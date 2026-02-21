// ============================================================================
// MOTO32 FIRMWARE v0.1-alpha
// Open-Source Motorcycle Control Unit
// ESP32 based (default target: ESP32-S3 N16R8) – Motogadget M-Unit Blue alternative
//
// Lead Software Developer:
//   STEFAN WAHRENDORFF (0691 Kollektiv)
//   Stefan.Wahrendorff@gmail.com
//
// WARNING / DISCLAIMER:
//   This software directly controls safety-relevant vehicle functions.
//   Use, modification, and operation are entirely at your own risk.
//   No warranty is provided for damage, injury, or legal compliance.
//   Verify all wiring, logic, and safety behavior before road use.
//
// Changelog v2.2 (from v2.1):
//   NEW: Daytime running light source + dim level (25/50/75/100%)
//   NEW: AUX output modes (ignition / engine / manual / disabled)
//   NEW: Parking light modes (position / left blinker / right blinker)
//   NEW: Rear light / tail light modes (standard / dimmed / always-on)
//   NEW: Alarm system (vibration sensor, horn + flash, auto-arm/disarm)
//   NEW: OTA firmware update via web dashboard (/api/ota)
//   NEW: Version API endpoint (/api/version)
//   NEW: WebSocket AUX toggle + restart commands
//   FIX: Scan results now forwarded to web UI via keyless JSON
//   FIX: Watchdog API migrated to ESP-IDF v5.x (backward compatible)
//   FIX: AUX output state in web dashboard reflects actual pin state
//   FIX: Toast messages use i18n keys (language-neutral)
//   FIX: Turn signal handling simplified to standard flasher behavior
//
// Changelog v2.1 (from v2.0):
//   NEW: Web Dashboard (WiFi AP + AsyncWebServer + WebSocket)
//        – Real-time diagnostics (all inputs/outputs, voltage, errors)
//        – Settings editor (all parameters, persisted to NVS)
//        – BLE Keyless Ignition (phone proximity unlock)
//   NEW: BLE Keyless Ignition state machine
//        – Phone detected → ignition granted
//        – Engine ran + stopped → 10s grace period for restart
//        – Grace expired → re-detection or key required
//
// Changelog v2.0 (from v1.x):
//   FIX  #1: GPIO boot-state   FIX #2: Watchdog
//   FIX  #3: Starter logic     FIX #4: Kill switch reset
//   FIX  #5: Flasher phase     FIX #6: Sidestand safety
//   FIX  #7: PWM brake fade    FIX #8: BLE GATT
//   FIX  #9: OTA partitions    FIX #10: Voltage monitoring
//   ARCH: Modular code, state structs, logging, named constants
// ============================================================================

#include <Arduino.h>
#include <esp_system.h>
#include <esp_partition.h>
#include "config.h"
#include "state.h"
#include "outputs.h"
#include "inputs.h"
#include "settings_store.h"
#include "safety.h"
#include "bike_logic.h"
#include "setup_mode.h"
#include "ble_interface.h"
#include "web_server.h"

// ============================================================================
// GLOBAL STATE INSTANCES
// ============================================================================

Settings  settings;
BikeState bike;

ButtonEvent turnLeftEvent;
ButtonEvent turnRightEvent;
ButtonEvent lightEvent;
ButtonEvent startEvent;
ButtonEvent hornEvent;
ButtonEvent lockEvent;

// ============================================================================
// BOOT DIAGNOSTICS / SAFE MODE
// ============================================================================

RTC_DATA_ATTR static uint32_t rtcBootLoopCounter = 0;
RTC_DATA_ATTR static uint32_t rtcLastUptimeMs = 0;
RTC_DATA_ATTR static uint32_t rtcBootDiagInitMagic = 0;

static const uint32_t RTC_BOOT_DIAG_MAGIC = 0x4D6F7432;  // "Mot2"
static esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;
static unsigned long lastSafeModeHeartbeat = 0;

static const char* resetReasonToText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "POWERON_RESET";
    case ESP_RST_EXT:       return "EXT_RESET";
    case ESP_RST_SW:        return "SW_RESET";
    case ESP_RST_PANIC:     return "PANIC_RESET";
    case ESP_RST_INT_WDT:   return "INT_WDT_RESET";
    case ESP_RST_TASK_WDT:  return "TASK_WDT_RESET";
    case ESP_RST_WDT:       return "WDT_RESET";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP_RESET";
    case ESP_RST_BROWNOUT:  return "BROWNOUT_RESET";
    case ESP_RST_SDIO:      return "SDIO_RESET";
    default:                return "UNKNOWN_RESET";
  }
}

static bool isUnstableReset(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_SW:
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
    case ESP_RST_BROWNOUT:
      return true;
    default:
      return false;
  }
}

static void evaluateBootHealthAndSafeMode() {
  if (rtcBootDiagInitMagic != RTC_BOOT_DIAG_MAGIC) {
    rtcBootDiagInitMagic = RTC_BOOT_DIAG_MAGIC;
    rtcBootLoopCounter = 0;
    rtcLastUptimeMs = 0;
  }

  bootResetReason = esp_reset_reason();
  bike.resetReasonCode = static_cast<uint8_t>(bootResetReason);

  const bool previousRunWasShort =
      rtcLastUptimeMs > 0 && rtcLastUptimeMs < SAFE_MODE_SHORT_UPTIME_MS;
  if (bootResetReason == ESP_RST_POWERON || bootResetReason == ESP_RST_EXT) {
    rtcBootLoopCounter = 0;
  } else if (previousRunWasShort && isUnstableReset(bootResetReason)) {
    if (rtcBootLoopCounter < 255) rtcBootLoopCounter++;
  } else {
    rtcBootLoopCounter = 0;
  }

  bike.bootLoopCounter = static_cast<uint8_t>(rtcBootLoopCounter);
  bike.safeModeActive = rtcBootLoopCounter >= SAFE_MODE_TRIGGER_RESTARTS;

  if (bootResetReason == ESP_RST_TASK_WDT
      || bootResetReason == ESP_RST_INT_WDT
      || bootResetReason == ESP_RST_WDT
      || bootResetReason == ESP_RST_PANIC) {
    bike.errorFlags |= ERR_WATCHDOG_RESET;
  }

  LOG_I("Reset reason: %s (%d), previous uptime=%lums, boot-loop counter=%u",
        resetReasonToText(bootResetReason), static_cast<int>(bootResetReason),
        rtcLastUptimeMs, rtcBootLoopCounter);
  if (bike.safeModeActive) {
    LOG_W("SAFE MODE active: repeated unstable restarts detected");
  }

  rtcLastUptimeMs = 1;  // heartbeat starts immediately for next boot-loop check
}

static void logCoreDumpPartitionStatus() {
  const esp_partition_t* core = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA,
      ESP_PARTITION_SUBTYPE_DATA_COREDUMP,
      nullptr);
  bike.coreDumpPartitionFound = (core != nullptr);
  if (core) {
    LOG_I("Core dump partition: offset=0x%06X size=%u",
          core->address, static_cast<unsigned>(core->size));
  } else {
    LOG_E("Core dump partition missing - flash partition table may be outdated");
  }
}

static void updateBootHeartbeat() {
  const unsigned long now = millis();
  if (now - lastSafeModeHeartbeat < SAFE_MODE_HEARTBEAT_MS) return;
  lastSafeModeHeartbeat = now;
  rtcLastUptimeMs = now;
  if (now >= SAFE_MODE_RECOVERY_UPTIME_MS && rtcBootLoopCounter != 0) {
    rtcBootLoopCounter = 0;
    bike.bootLoopCounter = 0;
    LOG_I("Boot-loop counter cleared after stable uptime");
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // ---------------------------------------------------------------
  // FIX #1: Initialize outputs FIRST – before Serial, before anything.
  // This prevents MOSFET gates from floating during ESP32/ESP32-S3 boot.
  // Strapping pins must be handled carefully to avoid boot glitches.
  // ---------------------------------------------------------------
  outputsInitEarly();

  // Now safe to start Serial
  Serial.begin(115200);
  LOG_I("Moto32 Firmware v" FIRMWARE_VERSION_STRING " starting...");

  evaluateBootHealthAndSafeMode();
  logCoreDumpPartitionStatus();

  // Load persistent settings from NVS
  loadSettings();
  safetyUpdateVoltage();
  safetyCheckVoltage();

  // Initialize PWM channels
  outputsInitPWM();

  // FIX #2: Initialize hardware watchdog
  safetyInitWatchdog();

  // Configure input pins
  pinMode(PIN_LOCK,  INPUT_PULLDOWN);  // Active HIGH (12V)
  pinMode(PIN_TURNL, INPUT_PULLUP);
  pinMode(PIN_TURNR, INPUT_PULLUP);
  pinMode(PIN_LIGHT, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_HORN,  INPUT_PULLUP);
  pinMode(PIN_BRAKE, INPUT_PULLUP);
  pinMode(PIN_KILL,  INPUT_PULLUP);
  pinMode(PIN_STAND, INPUT_PULLUP);
  pinMode(PIN_AUX1,  INPUT_PULLUP);
  pinMode(PIN_AUX2,  INPUT_PULLUP);
  pinMode(PIN_SPEED, INPUT_PULLUP);

  // Cold start LED test
  LOG_D("Cold start – LED test");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(SETUP_FLASH_INTERVAL_MS);
    digitalWrite(LED_STATUS, LOW);
    delay(SETUP_FLASH_INTERVAL_MS);
  }

  // Check for setup mode (horn held during power-on)
  setupModeCheck();

  // FIX #8: Initialize BLE (GATT + Keyless)
  bleInit();

  // Web Dashboard (WiFi AP + HTTP + WebSocket)
  webInit();
  webSetReducedLoadMode(bike.safeModeActive);
  bleKeylessSetAutoScanSuspended(bike.safeModeActive);

  if (bike.batteryVoltageAvailable) {
    LOG_I("Moto32 ready. Battery: %.1fV", bike.batteryVoltage);
  } else {
    LOG_I("Moto32 ready. Battery: N/A");
  }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Feed watchdog first thing
  safetyFeedWatchdog();
  updateBootHeartbeat();

  // Read all button inputs
  refreshInputEvents();

  // Process calibration if running
  processCalibrationSequence();
  if (bike.calibrationState == CALIB_DONE) {
    bike.calibrationState = CALIB_IDLE;
    saveSettings();
  }

  // Setup mode exit check
  if (bike.inSetupMode) {
    setupModeHandleExit();
    if (bike.inSetupMode) {
      bleUpdate();
      webUpdate();
      return;
    }
  }

  // ---- Normal operation ----

  // Read inputs & update state
  handleLock();
  handleTurnSignals();
  handleLight();
  handleStart();
  handleHorn();
  handleBrake();
  handleSpeedSensor();
  safetyRunInputPlausibilityChecks();

  // BLE Keyless: update proximity scan + state machine
  if (!bike.safeModeActive) {
    bleKeylessUpdate();
  }

  // Keyless ignition: ON only on configured button trigger while session is active.
  if (bleKeylessTakeIgnitionOnRequest() && bleKeylessIgnitionAllowed() && !bike.ignitionOn) {
    bike.ignitionOn = true;
    LOG_I("Keyless: ignition ON (button trigger)");
  }

  if (bleKeylessTakeIgnitionOffRequest() && bike.ignitionOn) {
    bike.ignitionOn = false;
    bike.engineRunning = false;
    bike.starterEngaged = false;
    bike.starterLightSnapshotValid = false;
    bike.manualHazardRequested = false;
    bike.lowBeamOn = false;
    bike.highBeamOn = false;
    LOG_I("Keyless: ignition OFF (session timeout)");
  }

  // FIX #4 + #6: Safety priorities (kill switch, sidestand, ignition)
  safetyApplyPriorities();
  handleStarterLightSuppression();

  // Write outputs
  updateIgnition();
  updateTurnSignals();
  updateLights();
  updateBrakeLight();
  updateHorn();
  updateStarter();
  updateAuxOutputs();
  updateParkingLight();
  updateAlarm();
  webApplyOutputOverrides();

  // BLE GATT + Web Dashboard updates
  bleUpdate();
  webUpdate();

  // Check for BLE settings update
  if (bleHasNewSettings()) {
    settings = bleGetNewSettings();
    saveSettings();
    LOG_I("Settings updated via BLE");
  }

  // Status LED: blink when ignition on, fast blink on error
  if (bike.errorFlags != ERR_NONE) {
    // Fast blink on error (5Hz)
    digitalWrite(LED_STATUS, (millis() % 200) < 100 ? HIGH : LOW);
  } else if (bike.ignitionOn) {
    // Slow blink when ignition on
    digitalWrite(LED_STATUS,
        (millis() % STATUS_LED_PERIOD_MS) < STATUS_LED_ON_MS ? HIGH : LOW);
  } else {
    digitalWrite(LED_STATUS, LOW);
  }

  // Yield to FreeRTOS scheduler (non-blocking, replaces delay(10))
  vTaskDelay(pdMS_TO_TICKS(1));
}
