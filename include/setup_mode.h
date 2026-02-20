#pragma once

#include "state.h"

// Check if setup mode should be entered (call once after input init)
void setupModeCheck();

// Start calibration output sequence
void startCalibrationSequence();

// Process calibration (non-blocking, call every loop)
void processCalibrationSequence();

// Handle setup mode exit (long horn press)
bool setupModeHandleExit();
