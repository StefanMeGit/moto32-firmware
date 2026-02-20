#include "web_server.h"
#include "inputs.h"
#include "settings_store.h"
#include "outputs.h"
#include "ble_interface.h"
#include "web_ui_embedded.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static AsyncWebServer  server(80);
static AsyncWebSocket  ws("/ws");
static const unsigned long BROADCAST_INTERVAL_MS = 150;
static const unsigned long STATE_KEEPALIVE_MS = 1000;
static const unsigned long KEYLESS_CHECK_INTERVAL_MS = 1000;
static const unsigned long KEYLESS_KEEPALIVE_MS = 5000;
static const unsigned long RESTART_DEFER_MS = 100;
static unsigned long lastStateCheck = 0;
static unsigned long lastStateBroadcast = 0;
static unsigned long lastKeylessCheck = 0;
static unsigned long lastKeylessBroadcast = 0;
static String lastStatePayload;
static String lastKeylessPayload;
static bool restartPending = false;
static unsigned long restartRequestedAt = 0;

// WiFi AP credentials
static const char* AP_SSID = "Moto32";
static const char* AP_PASS = "moto3232";  // min 8 chars

struct ManualOutputOverride {
  const char* id;
  int         pin;
  bool        forcedOn;
};

static ManualOutputOverride manualOutputOverrides[] = {
  {"turnLOut", PIN_TURNL_OUT, false},
  {"turnROut", PIN_TURNR_OUT, false},
  {"lightOut", PIN_LIGHT_OUT, false},
  {"hibeam",   PIN_HIBEAM_OUT, false},
  {"brakeOut", PIN_BRAKE_OUT, false},
  {"hornOut",  PIN_HORN_OUT, false},
  {"start1",   PIN_START_OUT1, false},
  {"start2",   PIN_START_OUT2, false},
  {"ignOut",   PIN_IGN_OUT, false},
  {"aux1Out",  PIN_AUX1_OUT, false},
  {"aux2Out",  PIN_AUX2_OUT, false}
};
static const size_t MANUAL_OUTPUT_OVERRIDE_COUNT =
    sizeof(manualOutputOverrides) / sizeof(manualOutputOverrides[0]);
static bool turnDistanceCalibrationActive = false;
static unsigned long turnDistanceCalibrationStartPulses = 0;

enum class WebActionType : uint8_t {
  SET_SETTINGS,
  START_TURN_CAL,
  FINISH_TURN_CAL,
  FACTORY_RESET,
  SET_KEYLESS,
  START_SCAN,
  STOP_SCAN,
  PAIR_DEVICE,
  REMOVE_PAIRED,
  TOGGLE_OUTPUT,
  TOGGLE_AUX1,
  TOGGLE_AUX2,
  RESTART
};

struct WebAction {
  WebActionType type;
  Settings settings;
  bool keylessEnabled;
  int keylessRssiThreshold;
  int keylessGraceSeconds;
  char mac[18];
  char outputId[16];
};

static QueueHandle_t wsActionQueue = nullptr;
static const uint8_t WS_ACTION_QUEUE_LEN = 16;

static ManualOutputOverride* findManualOverrideById(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
    if (strcmp(manualOutputOverrides[i].id, id) == 0) {
      return &manualOutputOverrides[i];
    }
  }
  return nullptr;
}

static bool isManualOverrideForcedOn(const char* id) {
  ManualOutputOverride* ovr = findManualOverrideById(id);
  return ovr ? ovr->forcedOn : false;
}

static bool enqueueWsAction(const WebAction& action, const char* cmd) {
  if (!wsActionQueue) {
    LOG_W("WS action queue unavailable, dropped cmd=%s", cmd ? cmd : "?");
    return false;
  }
  if (xQueueSend(wsActionQueue, &action, 0) != pdTRUE) {
    LOG_W("WS action queue full, dropped cmd=%s", cmd ? cmd : "?");
    return false;
  }
  return true;
}

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
  doc["bleConnected"] = bike.bleConnected;
  doc["speedPulses"] = bike.speedPulseCount;
  doc["turnCalActive"] = turnDistanceCalibrationActive;
  doc["turnCalPulses"] = turnDistanceCalibrationActive
      ? (bike.speedPulseCount - turnDistanceCalibrationStartPulses)
      : 0;

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

  // Manual ADVANCED-mode overrides from web UI.
  if (isManualOverrideForcedOn("turnLOut")) outs["turnLOut"] = true;
  if (isManualOverrideForcedOn("turnROut")) outs["turnROut"] = true;
  if (isManualOverrideForcedOn("lightOut")) outs["lightOut"] = true;
  if (isManualOverrideForcedOn("hibeam"))   outs["hibeam"] = true;
  if (isManualOverrideForcedOn("brakeOut")) outs["brakeOut"] = true;
  if (isManualOverrideForcedOn("hornOut"))  outs["hornOut"] = true;
  if (isManualOverrideForcedOn("start1"))   outs["start1"] = true;
  if (isManualOverrideForcedOn("start2"))   outs["start2"] = true;
  if (isManualOverrideForcedOn("ignOut"))   outs["ignOut"] = true;
  if (isManualOverrideForcedOn("aux1Out"))  outs["aux1Out"] = true;
  if (isManualOverrideForcedOn("aux2Out"))  outs["aux2Out"] = true;
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

static void broadcastSettingsJson() {
  if (ws.count() == 0) return;
  JsonDocument doc;
  buildSettingsJson(doc);
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

static void processQueuedWsActions() {
  if (!wsActionQueue) return;

  WebAction action;
  while (xQueueReceive(wsActionQueue, &action, 0) == pdTRUE) {
    switch (action.type) {
      case WebActionType::SET_SETTINGS:
        settings = action.settings;
        saveSettings();
        LOG_I("Settings updated via Web (deferred)");
        broadcastSettingsJson();
        break;

      case WebActionType::START_TURN_CAL:
        turnDistanceCalibrationActive = true;
        turnDistanceCalibrationStartPulses = bike.speedPulseCount;
        LOG_I("Turn distance calibration started at pulse=%lu",
              turnDistanceCalibrationStartPulses);
        break;

      case WebActionType::FINISH_TURN_CAL:
        if (turnDistanceCalibrationActive) {
          const unsigned long delta =
              bike.speedPulseCount - turnDistanceCalibrationStartPulses;
          turnDistanceCalibrationActive = false;
          if (delta > 0) {
            settings.turnDistancePulsesTarget = constrain(
                (int)(delta * 5UL),
                TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
            saveSettings();
            LOG_I("Turn distance calibrated: 10m=%lu pulses, target=%u",
                  delta, settings.turnDistancePulsesTarget);
          } else {
            LOG_W("Turn distance calibration failed: no pulses captured");
          }
        }
        broadcastSettingsJson();
        break;

      case WebActionType::FACTORY_RESET:
        settings = Settings{};
        saveSettings();
        turnDistanceCalibrationActive = false;
        LOG_I("Factory reset via Web (deferred)");
        broadcastSettingsJson();
        break;

      case WebActionType::SET_KEYLESS:
        bleKeylessConfigure(action.keylessEnabled,
                            action.keylessRssiThreshold,
                            action.keylessGraceSeconds);
        break;

      case WebActionType::START_SCAN:
        bleStartScan();
        break;

      case WebActionType::STOP_SCAN:
        bleStopScan();
        break;

      case WebActionType::PAIR_DEVICE:
        blePairDevice(action.mac);
        break;

      case WebActionType::REMOVE_PAIRED:
        bleRemovePaired(action.mac);
        break;

      case WebActionType::TOGGLE_OUTPUT: {
        ManualOutputOverride* ovr = findManualOverrideById(action.outputId);
        if (ovr) {
          ovr->forcedOn = !ovr->forcedOn;
          LOG_W("Manual output override %s: %s",
                action.outputId, ovr->forcedOn ? "ON" : "OFF");
        }
        break;
      }

      case WebActionType::TOGGLE_AUX1:
        bike.aux1ManualOn = !bike.aux1ManualOn;
        LOG_D("AUX1 manual: %s", bike.aux1ManualOn ? "ON" : "OFF");
        break;

      case WebActionType::TOGGLE_AUX2:
        bike.aux2ManualOn = !bike.aux2ManualOn;
        LOG_D("AUX2 manual: %s", bike.aux2ManualOn ? "ON" : "OFF");
        break;

      case WebActionType::RESTART:
        restartPending = true;
        restartRequestedAt = millis();
        LOG_I("Restart requested via Web (deferred)");
        break;
    }
  }

  if (restartPending && (millis() - restartRequestedAt >= RESTART_DEFER_MS)) {
    LOG_I("Restarting system...");
    esp_restart();
  }
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

    WebAction action = {};
    action.type = WebActionType::SET_SETTINGS;
    action.settings = settings;
    action.settings.handlebarConfig  = static_cast<HandlebarConfig>(
        constrain((int)d["handlebar"], CONFIG_A, CONFIG_E));
    action.settings.rearLightMode    = d["rear"] | 0;
    action.settings.turnSignalMode   = static_cast<TurnSignalMode>(
        constrain((int)d["turn"], TURN_OFF, TURN_30S));
    action.settings.brakeLightMode   = static_cast<BrakeLightMode>(
        constrain((int)d["brake"], BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
    action.settings.alarmMode        = d["alarm"] | 0;
    action.settings.positionLight    = constrain((int)d["pos"], 0, 9);
    action.settings.moWaveEnabled    = (int)d["wave"] != 0;
    action.settings.lowBeamMode      = d["low"] | 0;
    action.settings.aux1Mode         = d["aux1"] | 0;
    action.settings.aux2Mode         = d["aux2"] | 0;
    action.settings.standKillMode    = d["stand"] | 0;
    action.settings.parkingLightMode = d["park"] | 0;
    action.settings.turnDistancePulsesTarget = constrain(
        (int)d["tdist"], TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
    enqueueWsAction(action, cmd);
  }

  // ---- Turn Distance Calibration (push bike 10m) ----
  else if (strcmp(cmd, "startTurnDistanceCal") == 0) {
    WebAction action = {};
    action.type = WebActionType::START_TURN_CAL;
    enqueueWsAction(action, cmd);
  }

  else if (strcmp(cmd, "finishTurnDistanceCal") == 0) {
    WebAction action = {};
    action.type = WebActionType::FINISH_TURN_CAL;
    enqueueWsAction(action, cmd);
  }

  // ---- Factory Reset ----
  else if (strcmp(cmd, "factoryReset") == 0) {
    WebAction action = {};
    action.type = WebActionType::FACTORY_RESET;
    enqueueWsAction(action, cmd);
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
    WebAction action = {};
    action.type = WebActionType::SET_KEYLESS;
    action.keylessEnabled = d["enabled"] | false;
    action.keylessRssiThreshold = d["rssiThreshold"] | -65;
    action.keylessGraceSeconds = d["graceSeconds"] | 10;
    enqueueWsAction(action, cmd);
  }

  else if (strcmp(cmd, "startScan") == 0) {
    LOG_I("Web: startScan requested by client #%u", client->id());
    WebAction action = {};
    action.type = WebActionType::START_SCAN;
    enqueueWsAction(action, cmd);
  }

  else if (strcmp(cmd, "stopScan") == 0) {
    LOG_I("Web: stopScan requested by client #%u", client->id());
    WebAction action = {};
    action.type = WebActionType::STOP_SCAN;
    enqueueWsAction(action, cmd);
  }

  else if (strcmp(cmd, "pairDevice") == 0) {
    const char* mac = doc["mac"];
    LOG_I("Web: pairDevice requested by client #%u (%s)",
          client->id(), mac ? mac : "null");
    if (mac) {
      WebAction action = {};
      action.type = WebActionType::PAIR_DEVICE;
      snprintf(action.mac, sizeof(action.mac), "%s", mac);
      enqueueWsAction(action, cmd);
    }
  }

  else if (strcmp(cmd, "removePaired") == 0) {
    const char* mac = doc["mac"];
    LOG_I("Web: removePaired requested by client #%u (%s)",
          client->id(), mac ? mac : "null");
    if (mac) {
      WebAction action = {};
      action.type = WebActionType::REMOVE_PAIRED;
      snprintf(action.mac, sizeof(action.mac), "%s", mac);
      enqueueWsAction(action, cmd);
    }
  }

  // ---- Direct output toggle (ADVANCED mode in UI) ----
  else if (strcmp(cmd, "toggleOutput") == 0) {
    const char* id = doc["id"];
    if (id) {
      WebAction action = {};
      action.type = WebActionType::TOGGLE_OUTPUT;
      snprintf(action.outputId, sizeof(action.outputId), "%s", id);
      enqueueWsAction(action, cmd);
    }
  }

  // ---- AUX Manual Toggle ----
  else if (strcmp(cmd, "toggleAux1") == 0) {
    WebAction action = {};
    action.type = WebActionType::TOGGLE_AUX1;
    enqueueWsAction(action, cmd);
  }
  else if (strcmp(cmd, "toggleAux2") == 0) {
    WebAction action = {};
    action.type = WebActionType::TOGGLE_AUX2;
    enqueueWsAction(action, cmd);
  }

  // ---- Restart ----
  else if (strcmp(cmd, "restart") == 0) {
    WebAction action = {};
    action.type = WebActionType::RESTART;
    enqueueWsAction(action, cmd);
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
  wsActionQueue = xQueueCreate(WS_ACTION_QUEUE_LEN, sizeof(WebAction));
  if (!wsActionQueue) {
    LOG_E("Failed to create WS action queue");
  } else {
    LOG_I("WS action queue ready (%u entries)", WS_ACTION_QUEUE_LEN);
  }
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve embedded Web UI directly from firmware image.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp = req->beginResponse_P(
      200, "text/html; charset=utf-8",
      reinterpret_cast<const uint8_t*>(WEB_UI_HTML), WEB_UI_HTML_LEN);
    req->send(resp);
  });

  // Captive portal probes on phones should land on dashboard root.
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("/");
  });

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

void webApplyOutputOverrides() {
  for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
    if (manualOutputOverrides[i].forcedOn) {
      outputOn(manualOutputOverrides[i].pin);
    }
  }
}

void webUpdate() {
  unsigned long now = millis();

  // Execute deferred operations on loop task (not async_tcp task).
  processQueuedWsActions();

  if (now - lastStateCheck < BROADCAST_INTERVAL_MS) return;
  lastStateCheck = now;

  // Cleanup dead connections
  ws.cleanupClients();

  if (ws.count() == 0) return;

  // Build state and broadcast only on change (or keepalive).
  JsonDocument doc;
  buildStateJson(doc);
  String stateOut;
  serializeJson(doc, stateOut);
  if (stateOut != lastStatePayload
      || (now - lastStateBroadcast) >= STATE_KEEPALIVE_MS) {
    ws.textAll(stateOut);
    lastStatePayload = stateOut;
    lastStateBroadcast = now;
  }

  // Keyless status is less dynamic than core state.
  if (now - lastKeylessCheck >= KEYLESS_CHECK_INTERVAL_MS) {
    lastKeylessCheck = now;
    JsonDocument kdoc;
    bleKeylessBuildJson(kdoc);
    String keylessOut;
    serializeJson(kdoc, keylessOut);
    if (keylessOut != lastKeylessPayload
        || (now - lastKeylessBroadcast) >= KEYLESS_KEEPALIVE_MS) {
      ws.textAll(keylessOut);
      lastKeylessPayload = keylessOut;
      lastKeylessBroadcast = now;
    }
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
