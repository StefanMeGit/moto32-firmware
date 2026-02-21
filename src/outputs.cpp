#include "outputs.h"

// Track which pins have PWM attached
static bool pwmAttached[MAX_PIN] = {};
static uint8_t pinToChannel[MAX_PIN] = {};

// ============================================================================
// CRITICAL: Early init – called FIRST in setup(), before Serial, before anything
// This prevents MOSFET outputs from floating HIGH during ESP32 boot.
// Strapping pins can glitch at boot – keep outputs forced OFF during early init.
// ============================================================================

void outputsInitEarly() {
  for (int i = 0; i < OUTPUT_PIN_COUNT; i++) {
    // Set pin LOW immediately, then configure as output
    gpio_set_level((gpio_num_t)OUTPUT_PINS[i], 0);
    pinMode(OUTPUT_PINS[i], OUTPUT);
    digitalWrite(OUTPUT_PINS[i], LOW);
  }
  // Status LED off
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);
}

// ============================================================================
// PWM SETUP
// ============================================================================

void outputsInitPWM() {
  // Brake light PWM (for BRAKE_FADE mode + rear light dimming)
  ledcSetup(PWM_CHANNEL_BRAKE, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

  // Low beam / position light PWM
  ledcSetup(PWM_CHANNEL_POSITION, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

  // Additional dimmable light channels (DRL)
  ledcSetup(PWM_CHANNEL_HIBEAM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcSetup(PWM_CHANNEL_AUX1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcSetup(PWM_CHANNEL_AUX2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
}

// ============================================================================
// BASIC OUTPUT CONTROL
// ============================================================================

void outputOn(int pin) {
  // If PWM is attached to this pin, detach first
  if (pin >= 0 && pin < MAX_PIN && pwmAttached[pin]) {
    ledcDetachPin(pin);
    pwmAttached[pin] = false;
    pinMode(pin, OUTPUT);
  }
  digitalWrite(pin, HIGH);
}

void outputOff(int pin) {
  if (pin >= 0 && pin < MAX_PIN && pwmAttached[pin]) {
    ledcDetachPin(pin);
    pwmAttached[pin] = false;
    pinMode(pin, OUTPUT);
  }
  digitalWrite(pin, LOW);
}

void outputPWM(int pin, uint8_t duty) {
  if (pin < 0 || pin >= MAX_PIN) return;

  uint8_t channel = 0;
  if (pin == PIN_BRAKE_OUT) {
    channel = PWM_CHANNEL_BRAKE;
  } else if (pin == PIN_LIGHT_OUT) {
    channel = PWM_CHANNEL_POSITION;
  } else if (pin == PIN_HIBEAM_OUT) {
    channel = PWM_CHANNEL_HIBEAM;
  } else if (pin == PIN_AUX1_OUT) {
    channel = PWM_CHANNEL_AUX1;
  } else if (pin == PIN_AUX2_OUT) {
    channel = PWM_CHANNEL_AUX2;
  } else {
    // Unsupported pin for PWM – fall back to on/off
    if (duty > 127) outputOn(pin);
    else outputOff(pin);
    return;
  }

  if (!pwmAttached[pin]) {
    ledcAttachPin(pin, channel);
    pwmAttached[pin] = true;
    pinToChannel[pin] = channel;
  }
  ledcWrite(channel, duty);
}

void outputsAllOff() {
  for (int i = 0; i < OUTPUT_PIN_COUNT; i++) {
    outputOff(OUTPUT_PINS[i]);
  }
  digitalWrite(LED_STATUS, LOW);
}
