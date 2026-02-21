#pragma once

#include "state.h"

// Initialize hardware watchdog timer
void safetyInitWatchdog();

// Feed watchdog – call every loop iteration
void safetyFeedWatchdog();

// Read battery voltage via ADC (filtered)
void safetyUpdateVoltage();

// Get last filtered voltage reading
float safetyGetVoltage();

// True when battery voltage is critically low (and VBAT input is valid/present).
bool safetyIsCriticalLowVoltage();

// Check voltage thresholds, set error flags
void safetyCheckVoltage();

// Handle sidestand safety logic
void safetyHandleStand();

// Apply safety priorities (kill > ignition > everything else)
void safetyApplyPriorities();

// Resolve hazard state from emergency + manual
void resolveHazardState();
