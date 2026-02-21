#include "safety.h"
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
// BATTERY / VOLTAGE LOGIC
// Disabled by request for USB-only operation (no shutdowns/warnings).
// ============================================================================

void safetyUpdateVoltage() {
  bike.batteryVoltage = 0.0f;
  bike.batteryVoltageAvailable = false;
}

float safetyGetVoltage() {
  return bike.batteryVoltage;
}

bool safetyIsCriticalLowVoltage() {
  return false;
}

void safetyCheckVoltage() {
  bike.batteryVoltageAvailable = false;
  bike.lowVoltageWarning = false;
  bike.errorFlags &= ~ERR_LOW_VOLTAGE;
  bike.errorFlags &= ~ERR_HIGH_VOLTAGE;
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
// INPUT PLAUSIBILITY CHECKS
// ============================================================================

static bool heldLongerThan(bool active,
                           unsigned long& activeSince,
                           unsigned long now,
                           unsigned long thresholdMs) {
  if (!active) {
    activeSince = 0;
    return false;
  }
  if (activeSince == 0) activeSince = now;
  return (now - activeSince) >= thresholdMs;
}

void safetyRunInputPlausibilityChecks() {
  static unsigned long startHeldSince = 0;
  static unsigned long lightHeldSince = 0;
  static unsigned long hornHeldSince = 0;
  static unsigned long turnLeftHeldSince = 0;
  static unsigned long turnRightHeldSince = 0;
  static unsigned long dualTurnHeldSince = 0;
  static unsigned long speedOffWindowStart = 0;
  static unsigned long speedOffPulseStart = 0;
  static unsigned long lastSpeedPulseCount = 0;
  static uint8_t activeFaultMask = 0;

  const unsigned long now = millis();
  uint8_t faultMask = 0;

  const bool stuckStart = heldLongerThan(
      startEvent.state, startHeldSince, now, INPUT_STUCK_ACTIVE_MS);
  const bool stuckLight = heldLongerThan(
      lightEvent.state, lightHeldSince, now, INPUT_STUCK_ACTIVE_MS);
  const bool stuckHorn = heldLongerThan(
      hornEvent.state, hornHeldSince, now, INPUT_STUCK_ACTIVE_MS);
  const bool stuckTurnL = heldLongerThan(
      turnLeftEvent.state, turnLeftHeldSince, now, INPUT_STUCK_ACTIVE_MS);
  const bool stuckTurnR = heldLongerThan(
      turnRightEvent.state, turnRightHeldSince, now, INPUT_STUCK_ACTIVE_MS);

  if (turnLeftEvent.state && turnRightEvent.state && !bike.hazardLightsOn) {
    if (dualTurnHeldSince == 0) dualTurnHeldSince = now;
  } else {
    dualTurnHeldSince = 0;
  }
  const bool dualTurnStuck = dualTurnHeldSince != 0
      && (now - dualTurnHeldSince >= INPUT_DUAL_TURN_STUCK_MS);

  bool speedPulsesWhileIgnOff = false;
  if (!bike.ignitionOn) {
    if (bike.speedPulseCount != lastSpeedPulseCount) {
      if (speedOffWindowStart == 0) {
        speedOffWindowStart = now;
        speedOffPulseStart = lastSpeedPulseCount;
      }
    }
    if (speedOffWindowStart != 0) {
      const unsigned long pulseDelta = bike.speedPulseCount - speedOffPulseStart;
      if (pulseDelta >= INPUT_SPEED_OFF_PULSE_THRESHOLD
          && (now - speedOffWindowStart) <= INPUT_SPEED_OFF_PULSE_WINDOW_MS) {
        speedPulsesWhileIgnOff = true;
      } else if ((now - speedOffWindowStart) > INPUT_SPEED_OFF_PULSE_WINDOW_MS) {
        speedOffWindowStart = 0;
        speedOffPulseStart = bike.speedPulseCount;
      }
    }
  } else {
    speedOffWindowStart = 0;
    speedOffPulseStart = bike.speedPulseCount;
  }
  lastSpeedPulseCount = bike.speedPulseCount;

  if (stuckStart) faultMask |= 0x01;
  if (stuckLight) faultMask |= 0x02;
  if (stuckHorn) faultMask |= 0x04;
  if (stuckTurnL) faultMask |= 0x08;
  if (stuckTurnR) faultMask |= 0x10;
  if (dualTurnStuck) faultMask |= 0x20;
  if (speedPulsesWhileIgnOff) faultMask |= 0x40;

  const uint8_t newFaults = faultMask & ~activeFaultMask;
  if (newFaults) {
    LOG_W("Input plausibility warning (mask=0x%02X)", newFaults);
  }
  if (activeFaultMask && !faultMask) {
    LOG_I("Input plausibility restored");
  }
  activeFaultMask = faultMask;

  if (faultMask) {
    bike.errorFlags |= ERR_INPUT_IMPLAUSIBLE;
    if (stuckStart && bike.starterEngaged) {
      bike.starterEngaged = false;
      LOG_W("Starter disengaged due to stuck START input");
    }
  } else {
    bike.errorFlags &= ~ERR_INPUT_IMPLAUSIBLE;
  }
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
