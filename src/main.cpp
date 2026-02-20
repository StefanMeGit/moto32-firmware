// ============================================================================
// MOTO32 FIRMWARE v2.1.0
// Open-Source Motorcycle Control Unit
// ESP32-S3 based – Motogadget M-Unit Blue alternative
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
// SETUP
// ============================================================================

void setup() {
  // ---------------------------------------------------------------
  // FIX #1: Initialize outputs FIRST – before Serial, before anything.
  // This prevents MOSFET gates from floating during ESP32-S3 boot.
  // GPIO45 (starter) is a strapping pin and can glitch HIGH!
  // ---------------------------------------------------------------
  outputsInitEarly();

  // Now safe to start Serial
  Serial.begin(115200);
  LOG_I("Moto32 Firmware v" FIRMWARE_VERSION_STRING " starting...");

  // Load persistent settings from NVS
  loadSettings();

  // Initialize PWM channels
  outputsInitPWM();

  // FIX #2: Initialize hardware watchdog
  safetyInitWatchdog();

  // Configure ADC for battery voltage monitoring
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(PIN_VBAT_ADC, INPUT);

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

  // Initial voltage reading
  safetyUpdateVoltage();

  LOG_I("Moto32 ready. Battery: %.1fV", bike.batteryVoltage);
}

// Track previous engine state for keyless grace trigger
static bool prevEngineRunning = false;

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Feed watchdog first thing
  safetyFeedWatchdog();

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

  // BLE Keyless: update proximity scan + state machine
  bleKeylessUpdate();

  // Keyless ignition: if phone detected, grant ignition even without key
  if (bleKeylessIgnitionAllowed() && !bike.ignitionOn) {
    bike.ignitionOn = true;
    LOG_I("Keyless: ignition ON (BLE proximity)");
  }

  // Detect engine-off transition → trigger keyless grace period
  if (prevEngineRunning && !bike.engineRunning) {
    bleKeylessEngineOff();
  }
  prevEngineRunning = bike.engineRunning;

  // FIX #10: Battery monitoring
  safetyUpdateVoltage();
  safetyCheckVoltage();

  // FIX #4 + #6: Safety priorities (kill switch, sidestand, ignition)
  safetyApplyPriorities();

  // Write outputs
  updateIgnition();
  updateTurnSignals();
  updateLights();
  updateBrakeLight();
  updateHorn();
  updateStarter();
  updateAuxOutputs();

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
