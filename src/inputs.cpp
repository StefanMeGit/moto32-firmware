#include "inputs.h"

// ============================================================================
// DEBOUNCED INPUT
// ============================================================================

// ESP32-S3 max GPIO number is 48 → MAX_PIN defined in config.h

static bool db_initialized[MAX_PIN] = {};
static bool db_stableState[MAX_PIN] = {};
static bool db_lastReading[MAX_PIN] = {};
static unsigned long db_lastChangeTime[MAX_PIN] = {};

bool inputRawActive(int pin) {
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
  }
  lastSpeedState = speedState;
}
