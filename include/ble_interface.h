#pragma once

#include "state.h"
#include <ArduinoJson.h>

// ─── Core BLE (GATT server for diagnostics) ───
void bleInit();
void bleUpdate();
bool bleHasNewSettings();
Settings bleGetNewSettings();

// ─── Keyless Ignition ───

// Run keyless proximity scan & state machine (call every loop)
void bleKeylessUpdate();

// Returns true if keyless grants ignition permission
bool bleKeylessIgnitionAllowed();

// Notify keyless that the engine has stopped (start grace timer)
void bleKeylessEngineOff();

// ─── Keyless Configuration ───
void bleKeylessConfigure(bool enabled, int rssiThreshold, int graceSeconds);

// ─── Pairing ───
void bleStartScan();
void bleStopScan();
void blePairDevice(const char* macStr);
void bleRemovePaired(const char* macStr);

// ─── JSON builder for web UI ───
void bleKeylessBuildJson(JsonDocument& doc);
