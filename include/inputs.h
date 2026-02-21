#pragma once

#include "state.h"

// Debounced digital input read
bool inputActive(int pin);

// Raw read (no debounce) – for initial checks only
bool inputRawActive(int pin);

// Update a ButtonEvent from its pin
void updateButtonEvent(int pin, ButtonEvent& event,
                       unsigned long longPressMs = 0,
                       unsigned long doubleClickMs = DOUBLE_CLICK_WINDOW_MS);

// Refresh all button events in one call
void refreshInputEvents();

// Speed sensor pulse counting (call every loop)
void handleSpeedSensor();

// Manual input overrides (for ADVANCED diagnostics control)
void inputSetManualOverride(int pin, bool enabled, bool active);
bool inputIsManualOverrideEnabled(int pin);
bool inputGetManualOverrideState(int pin);
void inputClearAllManualOverrides();
