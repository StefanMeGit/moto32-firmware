#include "safety.h"
#include "outputs.h"
#include "inputs.h"
#include <esp_task_wdt.h>

// ============================================================================
// WATCHDOG (FIX #2) – uses new esp_task_wdt_config_t API (ESP-IDF ≥5.x)
// ============================================================================

void safetyInitWatchdog() {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // New ESP-IDF v5.x API
  esp_task_wdt_config_t wdtConfig = {
    .timeout_ms     = WATCHDOG_TIMEOUT_S * 1000,
    .idle_core_mask = 0,          // Don't watch idle tasks
    .trigger_panic  = true        // Panic on timeout → restart
  };
  esp_task_wdt_reconfigure(&wdtConfig);
  esp_task_wdt_add(NULL);         // Add current task (loopTask)
#else
  // Legacy ESP-IDF v4.x API
  esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
#endif
  LOG_I("Watchdog initialized (%ds)", WATCHDOG_TIMEOUT_S);
}

void safetyFeedWatchdog() {
  esp_task_wdt_reset();
}

// ============================================================================
// BATTERY VOLTAGE MONITORING (FIX #10)
// ============================================================================

static float voltageBuffer[VBAT_FILTER_SAMPLES] = {};
static int   voltageIndex = 0;
static bool  voltageBufferFull = false;
static unsigned long lastVoltageRead = 0;

void safetyUpdateVoltage() {
  unsigned long now = millis();
  if (now - lastVoltageRead < VBAT_SAMPLE_INTERVAL_MS) return;
  lastVoltageRead = now;

  int raw = analogRead(PIN_VBAT_ADC);
  float voltage = (raw / 4095.0f) * 3.3f * VBAT_DIVIDER_RATIO;

  voltageBuffer[voltageIndex] = voltage;
  voltageIndex = (voltageIndex + 1) % VBAT_FILTER_SAMPLES;
  if (voltageIndex == 0) voltageBufferFull = true;

  // Calculate moving average
  int count = voltageBufferFull ? VBAT_FILTER_SAMPLES : voltageIndex;
  if (count == 0) count = 1;
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += voltageBuffer[i];
  }
  bike.batteryVoltage = sum / count;
}

float safetyGetVoltage() {
  return bike.batteryVoltage;
}

void safetyCheckVoltage() {
  float v = bike.batteryVoltage;

  // Low voltage
  if (v < VBAT_CRITICAL_LOW && v > 1.0f) {  // > 1V to ignore disconnected ADC
    bike.errorFlags |= ERR_LOW_VOLTAGE;
    bike.lowVoltageWarning = true;
    LOG_W("CRITICAL: Battery voltage %.1fV – shutting down non-essential", v);
    // Turn off non-essential outputs
    outputOff(PIN_AUX1_OUT);
    outputOff(PIN_AUX2_OUT);
    outputOff(PIN_HORN_OUT);
  } else if (v < VBAT_WARNING_LOW && v > 1.0f) {
    bike.lowVoltageWarning = true;
    if (!(bike.errorFlags & ERR_LOW_VOLTAGE)) {
      LOG_W("Battery voltage low: %.1fV", v);
    }
  } else {
    bike.lowVoltageWarning = false;
    bike.errorFlags &= ~ERR_LOW_VOLTAGE;
  }

  // High voltage
  if (v > VBAT_CRITICAL_HIGH) {
    bike.errorFlags |= ERR_HIGH_VOLTAGE;
    LOG_E("CRITICAL: Overvoltage %.1fV!", v);
  } else if (v > VBAT_WARNING_HIGH) {
    if (!(bike.errorFlags & ERR_HIGH_VOLTAGE)) {
      LOG_W("Battery voltage high: %.1fV", v);
    }
  } else {
    bike.errorFlags &= ~ERR_HIGH_VOLTAGE;
  }
}

// ============================================================================
// SIDESTAND SAFETY (FIX #6)
// ============================================================================

void safetyHandleStand() {
  bike.standDown = inputActive(PIN_STAND);

  // If stand is down and engine running → kill engine
  // This is standard motorcycle safety behavior.
  // standKillMode / 3 determines stand behavior:
  //   0 = stand kills engine
  //   1 = stand kills engine (N/C sensor)
  //   2+ = stand safety disabled
  uint8_t standBehavior = settings.standKillMode / 3;

  if (standBehavior < 2 && bike.standDown && bike.engineRunning) {
    bike.engineRunning = false;
    bike.errorFlags |= ERR_STAND_KILL;
    LOG_W("Engine killed: sidestand down");
  }

  // Clear stand error when stand is up
  if (!bike.standDown) {
    bike.errorFlags &= ~ERR_STAND_KILL;
  }
}

// ============================================================================
// KILL SWITCH (FIX #4 – proper reset)
// ============================================================================

static void handleKillSwitch() {
  bool killState = inputActive(PIN_KILL);
  uint8_t killConfig = settings.standKillMode % 3;

  // FIX: Always update killActive based on current switch state
  switch (killConfig) {
    case 0:  // N/O: active when closed (signal LOW = active)
      bike.killActive = killState;
      break;
    case 1:  // N/C: active when open (signal HIGH = active)
      bike.killActive = !killState;
      break;
    case 2:  // Kill switch disabled
      bike.killActive = false;
      break;
  }

  if (bike.killActive) {
    bike.engineRunning = false;
    bike.starterEngaged = false;
  }
}

// ============================================================================
// HAZARD STATE RESOLUTION
// ============================================================================

void resolveHazardState() {
  bike.hazardLightsOn = bike.emergencyHazardActive || bike.manualHazardRequested;
}

// ============================================================================
// MASTER SAFETY PRIORITIES
// ============================================================================

void safetyApplyPriorities() {
  // Kill switch
  handleKillSwitch();

  // Priority 1: kill switch always stops engine and starter
  if (bike.killActive) {
    bike.engineRunning  = false;
    bike.starterEngaged = false;
  }

  // Priority 2: without ignition, disable everything dynamic
  if (!bike.ignitionOn) {
    bike.leftTurnOn           = false;
    bike.rightTurnOn          = false;
    bike.manualHazardRequested = false;
    bike.engineRunning         = false;
    bike.starterEngaged        = false;
  }

  // Priority 3: sidestand safety
  safetyHandleStand();

  // Resolve composite hazard state
  resolveHazardState();
}
