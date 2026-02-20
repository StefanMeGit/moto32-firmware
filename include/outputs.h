#pragma once

#include "state.h"

// ============================================================================
// OUTPUT CONTROL
// ============================================================================

// Initialize all output pins to safe OFF state.
// MUST be called as first thing in setup() before anything else!
void outputsInitEarly();

// Set up PWM channels (call after outputsInitEarly)
void outputsInitPWM();

// Basic on/off
void outputOn(int pin);
void outputOff(int pin);

// PWM output (0-255)
void outputPWM(int pin, uint8_t duty);

// Force all outputs off (emergency / fail-safe)
void outputsAllOff();
