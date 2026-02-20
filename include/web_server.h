#pragma once

#include "state.h"

// Initialize WiFi AP + AsyncWebServer + WebSocket
void webInit();

// Send state update to all connected WebSocket clients (call ~100ms)
void webUpdate();

// Broadcast a log message to clients
void webLog(const char* text);
