#pragma once

#include "state.h"

// Initialize WiFi AP + AsyncWebServer + WebSocket
void webInit();

// Send state update to all connected WebSocket clients (call ~100ms)
void webUpdate();

// Apply manual output overrides from Web UI (call each main loop cycle).
void webApplyOutputOverrides();

// Broadcast a log message to clients
void webLog(const char* text);
