#pragma once

#include "state.h"

// Initialize hardware watchdog timer
void safetyInitWatchdog();

// Feed watchdog – call every loop iteration
void safetyFeedWatchdog();

// Battery-voltage handling (currently disabled/no-op for USB bench operation)
void safetyUpdateVoltage();

// Get last filtered voltage reading
float safetyGetVoltage();

// Always false while voltage logic is disabled.
bool safetyIsCriticalLowVoltage();

// Clears voltage-related warning/error flags (no threshold logic).
void safetyCheckVoltage();

// Handle sidestand safety logic
void safetyHandleStand();

// Apply safety priorities (kill > ignition > everything else)
void safetyApplyPriorities();

// Resolve hazard state from emergency + manual
void resolveHazardState();

// Validate input plausibility (stuck switches, implausible combos/noise)
void safetyRunInputPlausibilityChecks();
