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

// Suspend or resume automatic keyless background scans.
void bleKeylessSetAutoScanSuspended(bool suspended);

// Returns true if keyless grants ignition permission
bool bleKeylessIgnitionAllowed();

// Returns true once when keyless requests ignition ON (button trigger)
bool bleKeylessTakeIgnitionOnRequest();

// Returns true once when keyless requests ignition OFF (session timeout)
bool bleKeylessTakeIgnitionOffRequest();

// ─── Keyless Configuration ───
void bleKeylessConfigure(bool enabled,
                         int rssiLevelOrLegacyDbm,
                         int activeMinutesOrLegacySeconds,
                         int activationMode,
                         int activationButton);

// ─── Pairing ───
void bleStartScan();
void bleStopScan();
void blePairDevice(const char* macStr);
void bleRemovePaired(const char* macStr);

// ─── JSON builder for web UI ───
void bleKeylessBuildJson(JsonDocument& doc);
