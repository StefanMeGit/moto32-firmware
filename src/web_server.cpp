#include "web_server.h"
#include "inputs.h"
#include "settings_store.h"
#include "outputs.h"
#include "ble_interface.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Update.h>

static AsyncWebServer  server(80);
static AsyncWebSocket  ws("/ws");
static unsigned long   lastBroadcast = 0;
static const unsigned long BROADCAST_INTERVAL_MS = 150;

// WiFi AP credentials
static const char* AP_SSID = "Moto32";
static const char* AP_PASS = "moto3232";  // min 8 chars

// ============================================================================
// JSON STATE BUILDER
// ============================================================================

static void buildStateJson(JsonDocument& doc) {
  doc["type"] = "state";
  doc["voltage"] = round(bike.batteryVoltage * 10.0f) / 10.0f;
  doc["errorFlags"] = bike.errorFlags;
  doc["ignitionOn"] = bike.ignitionOn;
  doc["engineRunning"] = bike.engineRunning;
  doc["starterEngaged"] = bike.starterEngaged;
  doc["killActive"] = bike.killActive;

  // Inputs
  JsonObject ins = doc["inputs"].to<JsonObject>();
  ins["lock"]  = bike.ignitionOn;
  ins["turnL"] = turnLeftEvent.state;
  ins["turnR"] = turnRightEvent.state;
  ins["light"] = lightEvent.state;
  ins["start"] = startEvent.state;
  ins["horn"]  = hornEvent.state;
  ins["brake"] = bike.brakePressed;
  ins["kill"]  = bike.killActive;
  ins["stand"] = bike.standDown;
  ins["aux1"]  = inputActive(PIN_AUX1);
  ins["aux2"]  = inputActive(PIN_AUX2);
  ins["speed"] = false;
  ins["speed_info"] = String(bike.speedPulseCount) + " Pulse";

  // Outputs
  JsonObject outs = doc["outputs"].to<JsonObject>();
  outs["turnLOut"] = bike.leftTurnOn && bike.flasherState;
  outs["turnROut"] = bike.rightTurnOn && bike.flasherState;
  outs["lightOut"] = bike.lowBeamOn;
  outs["hibeam"]   = bike.highBeamOn;
  outs["brakeOut"] = bike.brakePressed;
  outs["hornOut"]  = bike.hornPressed && bike.ignitionOn;
  outs["start1"]   = bike.starterEngaged;
  outs["start2"]   = bike.starterEngaged;
  outs["ignOut"]   = bike.ignitionOn && !bike.killActive;
  // AUX outputs reflect actual state based on current mode
  outs["aux1Out"]  = digitalRead(PIN_AUX1_OUT) == HIGH;
  outs["aux2Out"]  = digitalRead(PIN_AUX2_OUT) == HIGH;

  // Hazard override
  if (bike.hazardLightsOn && bike.flasherState) {
    outs["turnLOut"] = true;
    outs["turnROut"] = true;
  }
}

static void buildSettingsJson(JsonDocument& doc) {
  doc["type"]      = "settings";
  doc["handlebar"] = settings.handlebarConfig;
  doc["rear"]      = settings.rearLightMode;
  doc["turn"]      = settings.turnSignalMode;
  doc["brake"]     = settings.brakeLightMode;
  doc["alarm"]     = settings.alarmMode;
  doc["pos"]       = settings.positionLight;
  doc["wave"]      = settings.moWaveEnabled ? 1 : 0;
  doc["low"]       = settings.lowBeamMode;
  doc["aux1"]      = settings.aux1Mode;
  doc["aux2"]      = settings.aux2Mode;
  doc["stand"]     = settings.standKillMode;
  doc["park"]      = settings.parkingLightMode;
  doc["tdist"]     = settings.turnDistancePulsesTarget;
}

// ============================================================================
// WEBSOCKET HANDLER
// ============================================================================

static void handleWsMessage(AsyncWebSocketClient* client, const char* data, size_t len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err) {
    LOG_W("WS JSON parse error: %s", err.c_str());
    return;
  }

  const char* cmd = doc["cmd"];
  if (!cmd) return;

  // ---- Get Settings ----
  if (strcmp(cmd, "getSettings") == 0) {
    JsonDocument resp;
    buildSettingsJson(resp);
    String out;
    serializeJson(resp, out);
    client->text(out);
  }

  // ---- Set Settings ----
  else if (strcmp(cmd, "setSettings") == 0) {
    JsonObject d = doc["data"];
    if (d.isNull()) return;
    settings.handlebarConfig  = static_cast<HandlebarConfig>(
        constrain((int)d["handlebar"], CONFIG_A, CONFIG_E));
    settings.rearLightMode    = d["rear"] | 0;
    settings.turnSignalMode   = static_cast<TurnSignalMode>(
        constrain((int)d["turn"], TURN_OFF, TURN_30S));
    settings.brakeLightMode   = static_cast<BrakeLightMode>(
        constrain((int)d["brake"], BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
    settings.alarmMode        = d["alarm"] | 0;
    settings.positionLight    = constrain((int)d["pos"], 0, 9);
    settings.moWaveEnabled    = (int)d["wave"] != 0;
    settings.lowBeamMode      = d["low"] | 0;
    settings.aux1Mode         = d["aux1"] | 0;
    settings.aux2Mode         = d["aux2"] | 0;
    settings.standKillMode    = d["stand"] | 0;
    settings.parkingLightMode = d["park"] | 0;
    settings.turnDistancePulsesTarget = constrain(
        (int)d["tdist"], TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
    saveSettings();
    LOG_I("Settings updated via Web");

    // Send confirmation
    JsonDocument resp;
    resp["type"] = "toast";
    resp["text"] = "settings_saved";  // i18n key – frontend resolves to localized text
    resp["level"] = "success";
    String out;
    serializeJson(resp, out);
    client->text(out);
  }

  // ---- Factory Reset ----
  else if (strcmp(cmd, "factoryReset") == 0) {
    settings = Settings{};  // Reset to defaults
    saveSettings();
    LOG_I("Factory reset via Web");
    JsonDocument resp;
    buildSettingsJson(resp);
    String out;
    serializeJson(resp, out);
    client->text(out);
  }

  // ---- BLE Keyless commands ----
  else if (strcmp(cmd, "getKeyless") == 0) {
    JsonDocument resp;
    bleKeylessBuildJson(resp);
    String out;
    serializeJson(resp, out);
    client->text(out);
  }

  else if (strcmp(cmd, "setKeyless") == 0) {
    JsonObject d = doc["data"];
    if (d.isNull()) return;
    bleKeylessConfigure(
      d["enabled"] | false,
      d["rssiThreshold"] | -65,
      d["graceSeconds"] | 10
    );
  }

  else if (strcmp(cmd, "startScan") == 0) {
    bleStartScan();
  }

  else if (strcmp(cmd, "stopScan") == 0) {
    bleStopScan();
  }

  else if (strcmp(cmd, "pairDevice") == 0) {
    const char* mac = doc["mac"];
    if (mac) blePairDevice(mac);
  }

  else if (strcmp(cmd, "removePaired") == 0) {
    const char* mac = doc["mac"];
    if (mac) bleRemovePaired(mac);
  }

  // ---- AUX Manual Toggle ----
  else if (strcmp(cmd, "toggleAux1") == 0) {
    bike.aux1ManualOn = !bike.aux1ManualOn;
    LOG_D("AUX1 manual: %s", bike.aux1ManualOn ? "ON" : "OFF");
  }
  else if (strcmp(cmd, "toggleAux2") == 0) {
    bike.aux2ManualOn = !bike.aux2ManualOn;
    LOG_D("AUX2 manual: %s", bike.aux2ManualOn ? "ON" : "OFF");
  }

  // ---- Restart ----
  else if (strcmp(cmd, "restart") == 0) {
    LOG_I("Restart requested via Web");
    delay(100);
    esp_restart();
  }
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      LOG_I("WS client #%u connected from %s",
            client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      LOG_I("WS client #%u disconnected", client->id());
      break;
    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len
          && info->opcode == WS_TEXT) {
        handleWsMessage(client, (const char*)data, len);
      }
      break;
    }
    default: break;
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void webInit() {
  // Start LittleFS
  if (!LittleFS.begin(true)) {
    LOG_E("LittleFS mount failed!");
    return;
  }
  LOG_I("LittleFS mounted");

  // WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  LOG_I("WiFi AP started: %s (IP: %s)",
        AP_SSID, WiFi.softAPIP().toString().c_str());

  // mDNS – makes the dashboard accessible via http://moto32.local
  if (MDNS.begin("moto32")) {
    MDNS.addService("http", "tcp", 80);
    LOG_I("mDNS started: http://moto32.local");
  } else {
    LOG_W("mDNS failed to start");
  }

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // API fallback for REST-style access
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    buildStateJson(doc);
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    buildSettingsJson(doc);
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // ---- OTA Firmware Update ----
  server.on("/api/ota", HTTP_POST,
    // Response callback (called when upload finishes)
    [](AsyncWebServerRequest* req) {
      bool success = !Update.hasError();
      JsonDocument doc;
      doc["type"] = "toast";
      doc["text"] = success ? "ota_success" : "ota_failed";
      doc["level"] = success ? "success" : "error";
      String out;
      serializeJson(doc, out);
      // Broadcast OTA result to all WS clients
      ws.textAll(out);
      req->send(200, "application/json",
                success ? "{\"ok\":true}" : "{\"ok\":false}");
      if (success) {
        LOG_I("OTA update successful – restarting...");
        delay(500);
        esp_restart();
      }
    },
    // Upload handler (called for each chunk)
    [](AsyncWebServerRequest* req, const String& filename,
       size_t index, uint8_t* data, size_t len, bool final) {
      if (index == 0) {
        LOG_I("OTA: begin '%s' (%u bytes)", filename.c_str(),
              req->contentLength());
        // Determine if this is firmware or filesystem
        int cmd = (filename.indexOf("littlefs") >= 0 ||
                   filename.indexOf("spiffs") >= 0)
                      ? U_SPIFFS : U_FLASH;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
          LOG_E("OTA: begin failed: %s", Update.errorString());
        }
      }
      if (Update.isRunning()) {
        if (Update.write(data, len) != len) {
          LOG_E("OTA: write failed: %s", Update.errorString());
        }
      }
      if (final) {
        if (Update.end(true)) {
          LOG_I("OTA: upload complete (%u bytes)", index + len);
        } else {
          LOG_E("OTA: end failed: %s", Update.errorString());
        }
      }
    }
  );

  // ---- Firmware Version API ----
  server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["firmware"] = FIRMWARE_VERSION_STRING;
    doc["chip"]     = ESP.getChipModel();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"]   = millis() / 1000;
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // 404
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found");
  });

  server.begin();
  LOG_I("HTTP server started on port 80");
}

void webUpdate() {
  unsigned long now = millis();
  if (now - lastBroadcast < BROADCAST_INTERVAL_MS) return;
  lastBroadcast = now;

  // Cleanup dead connections
  ws.cleanupClients();

  if (ws.count() == 0) return;

  // Build and broadcast state
  JsonDocument doc;
  buildStateJson(doc);
  String out;
  serializeJson(doc, out);
  ws.textAll(out);

  // Also send keyless status periodically
  static unsigned long lastKeyless = 0;
  if (now - lastKeyless > 500) {
    lastKeyless = now;
    JsonDocument kdoc;
    bleKeylessBuildJson(kdoc);
    String kout;
    serializeJson(kdoc, kout);
    ws.textAll(kout);
  }
}

void webLog(const char* text) {
  if (ws.count() == 0) return;
  JsonDocument doc;
  doc["type"] = "log";
  doc["text"] = text;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}
