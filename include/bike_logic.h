#pragma once

#include "state.h"

// ---- Input handlers (read buttons, update state) ----
void handleLock();
void handleTurnSignals();
void handleLight();
void handleStart();
void handleHorn();
void handleBrake();

// ---- Output updaters (write to pins based on state) ----
void updateIgnition();
void updateTurnSignals();
void updateLights();
void updateBrakeLight();
void updateHorn();
void updateStarter();
void updateAuxOutputs();
