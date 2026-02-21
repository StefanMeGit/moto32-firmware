#include "inputs.h"

// ============================================================================
// DEBOUNCED INPUT
// ============================================================================

// ESP32 max GPIO number is 39 → MAX_PIN defined in config.h

static bool db_initialized[MAX_PIN] = {};
static bool db_stableState[MAX_PIN] = {};
static bool db_lastReading[MAX_PIN] = {};
static unsigned long db_lastChangeTime[MAX_PIN] = {};
static bool manual_override_enabled[MAX_PIN] = {};
static bool manual_override_state[MAX_PIN] = {};

void inputSetManualOverride(int pin, bool enabled, bool active) {
  if (pin < 0 || pin >= MAX_PIN) return;
  manual_override_enabled[pin] = enabled;
  manual_override_state[pin] = active;
}

bool inputIsManualOverrideEnabled(int pin) {
  if (pin < 0 || pin >= MAX_PIN) return false;
  return manual_override_enabled[pin];
}

bool inputGetManualOverrideState(int pin) {
  if (pin < 0 || pin >= MAX_PIN) return false;
  return manual_override_state[pin];
}

void inputClearAllManualOverrides() {
  for (int pin = 0; pin < MAX_PIN; pin++) {
    manual_override_enabled[pin] = false;
    manual_override_state[pin] = false;
  }
}

bool inputRawActive(int pin) {
  if (pin >= 0 && pin < MAX_PIN && manual_override_enabled[pin]) {
    return manual_override_state[pin];
  }
  if (pin == PIN_LOCK) {
    return digitalRead(pin) == HIGH;   // LOCK connects to +12V
  }
  return digitalRead(pin) == LOW;      // All other inputs switch to ground
}

bool inputActive(int pin) {
  if (pin < 0 || pin >= MAX_PIN) {
    LOG_E("inputActive: invalid pin %d", pin);
    return false;   // FIX: return safe default, not raw read
  }

  if (manual_override_enabled[pin]) {
    bool forced = manual_override_state[pin];
    db_initialized[pin] = true;
    db_stableState[pin] = forced;
    db_lastReading[pin] = forced;
    db_lastChangeTime[pin] = millis();
    return forced;
  }

  bool reading = inputRawActive(pin);

  if (!db_initialized[pin]) {
    db_initialized[pin]    = true;
    db_stableState[pin]    = reading;
    db_lastReading[pin]    = reading;
    db_lastChangeTime[pin] = millis();
    return db_stableState[pin];
  }

  if (reading != db_lastReading[pin]) {
    db_lastReading[pin]    = reading;
    db_lastChangeTime[pin] = millis();
  }

  if ((millis() - db_lastChangeTime[pin]) >= DEBOUNCE_DELAY_MS
      && db_stableState[pin] != reading) {
    db_stableState[pin] = reading;
  }

  return db_stableState[pin];
}

// ============================================================================
// BUTTON EVENT PROCESSING
// ============================================================================

void updateButtonEvent(int pin, ButtonEvent& event,
                       unsigned long longPressMs,
                       unsigned long doubleClickMs) {
  bool state = inputActive(pin);
  unsigned long now = millis();

  event.pressed     = state && !event.state;
  event.released    = !state && event.state;
  event.doubleClick = false;

  if (event.pressed) {
    if (now - event.lastReleaseTime <= doubleClickMs) {
      event.doubleClick = true;
    }
    event.pressStartTime   = now;
    event.longPressLatched = false;
  }

  if (event.released) {
    event.lastReleaseTime = now;
  }

  event.longPress = false;
  if (state && longPressMs > 0 && !event.longPressLatched
      && (now - event.pressStartTime >= longPressMs)) {
    event.longPress        = true;
    event.longPressLatched = true;
  }

  event.state = state;
}

void refreshInputEvents() {
  updateButtonEvent(PIN_LOCK,  lockEvent);
  updateButtonEvent(PIN_TURNL, turnLeftEvent,  LONG_PRESS_THRESHOLD_MS);
  updateButtonEvent(PIN_TURNR, turnRightEvent, LONG_PRESS_THRESHOLD_MS);
  updateButtonEvent(PIN_LIGHT, lightEvent,     LONG_PRESS_THRESHOLD_MS);
  updateButtonEvent(PIN_START, startEvent,     0, 500);
  updateButtonEvent(PIN_HORN,  hornEvent,      LONG_PRESS_THRESHOLD_MS);
}

// ============================================================================
// SPEED SENSOR
// ============================================================================

void handleSpeedSensor() {
  static bool lastSpeedState = false;
  bool speedState = inputActive(PIN_SPEED);

  if (speedState && !lastSpeedState) {
    bike.speedPulseCount++;
    // Engine/RPM latch: once pulses are present with ignition on, consider
    // engine running until a higher-priority safety path (kill/ignition off)
    // clears it.
    if (bike.ignitionOn && !bike.killActive && !bike.engineRunning) {
      bike.engineRunning = true;
      LOG_I("Engine RUNNING (RPM pulse detected)");
    }
  }
  lastSpeedState = speedState;
}
