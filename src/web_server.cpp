#include "web_server.h"
#include "inputs.h"
#include "settings_store.h"
#include "outputs.h"
#include "ble_interface.h"
#include "safety.h"
#include "web_ui_embedded.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <ctype.h>

static AsyncWebServer  server(80);
static AsyncWebSocket  ws("/ws");
static const unsigned long BROADCAST_INTERVAL_MS_DEFAULT = 200;
static const unsigned long BROADCAST_INTERVAL_MS_REDUCED = 500;
static const unsigned long STATE_KEEPALIVE_MS = 1000;
static const unsigned long KEYLESS_CHECK_INTERVAL_MS_DEFAULT = 1000;
static const unsigned long KEYLESS_CHECK_INTERVAL_MS_REDUCED = 2500;
static const unsigned long KEYLESS_KEEPALIVE_MS_DEFAULT = 5000;
static const unsigned long KEYLESS_KEEPALIVE_MS_REDUCED = 10000;
static const unsigned long WS_CLEANUP_INTERVAL_MS = 1000;
static const unsigned long RESTART_DEFER_MS = 100;
static unsigned long lastStateCheck = 0;
static unsigned long lastStateBroadcast = 0;
static unsigned long lastKeylessCheck = 0;
static unsigned long lastKeylessBroadcast = 0;
static unsigned long lastWsCleanup = 0;
static unsigned long stateBroadcastIntervalMs = BROADCAST_INTERVAL_MS_DEFAULT;
static unsigned long keylessCheckIntervalMs = KEYLESS_CHECK_INTERVAL_MS_DEFAULT;
static unsigned long keylessKeepaliveMs = KEYLESS_KEEPALIVE_MS_DEFAULT;
static String lastStatePayload;
static String lastKeylessPayload;
static bool restartPending = false;
static unsigned long restartRequestedAt = 0;
static unsigned long lastWsBackpressureLog = 0;

// WiFi AP credentials
static const char* AP_SSID = "Moto32";
static const char* AP_PASS = "moto3232";  // min 8 chars
static const uint8_t AP_CHANNEL = 6;
static const uint8_t AP_MAX_CONNECTIONS = 4;
static char activeApSsid[40] = {0};
static bool activeApOpenFallback = false;

struct ManualOutputOverride {
  const char* id;
  int         pin;
  bool        allowManualForce;
  bool        forcedOn;
};

struct ManualInputOverride {
  const char* id;
  int         pin;
  bool        enabled;
  bool        forcedActive;
};

static ManualOutputOverride manualOutputOverrides[] = {
  {"turnLOut", PIN_TURNL_OUT, true,  false},
  {"turnROut", PIN_TURNR_OUT, true,  false},
  {"lightOut", PIN_LIGHT_OUT, true,  false},
  {"hibeam",   PIN_HIBEAM_OUT, true,  false},
  {"brakeOut", PIN_BRAKE_OUT, true,  false},
  {"hornOut",  PIN_HORN_OUT, true,  false},
  {"start1",   PIN_START_OUT1, false, false},
  {"start2",   PIN_START_OUT2, false, false},
  {"ignOut",   PIN_IGN_OUT, false, false},
  {"aux1Out",  PIN_AUX1_OUT, true,  false},
  {"aux2Out",  PIN_AUX2_OUT, true,  false}
};
static const size_t MANUAL_OUTPUT_OVERRIDE_COUNT =
    sizeof(manualOutputOverrides) / sizeof(manualOutputOverrides[0]);

static ManualInputOverride manualInputOverrides[] = {
  {"lock",  PIN_LOCK,  false, false},
  {"turnL", PIN_TURNL, false, false},
  {"turnR", PIN_TURNR, false, false},
  {"light", PIN_LIGHT, false, false},
  {"start", PIN_START, false, false},
  {"horn",  PIN_HORN,  false, false},
  {"brake", PIN_BRAKE, false, false},
  {"kill",  PIN_KILL,  false, false},
  {"stand", PIN_STAND, false, false},
  {"aux1",  PIN_AUX1,  false, false},
  {"aux2",  PIN_AUX2,  false, false},
  {"speed", PIN_SPEED, false, false}
};
static const size_t MANUAL_INPUT_OVERRIDE_COUNT =
    sizeof(manualInputOverrides) / sizeof(manualInputOverrides[0]);
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
  CLEAR_INPUT_OVERRIDES,
  TOGGLE_INPUT,
  CLEAR_INPUT_OVERRIDE,
  TOGGLE_OUTPUT,
  CLEAR_OUTPUT_OVERRIDE,
  TOGGLE_AUX1,
  TOGGLE_AUX2,
  RESTART
};

struct WebAction {
  WebActionType type;
  Settings settings;
  bool keylessEnabled;
  int keylessRssiLevel;
  int keylessActiveMinutes;
  int keylessActivationMode;
  int keylessActivationButton;
  char mac[18];
  char inputId[16];
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

static ManualInputOverride* findManualInputOverrideById(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < MANUAL_INPUT_OVERRIDE_COUNT; i++) {
    if (strcmp(manualInputOverrides[i].id, id) == 0) {
      return &manualInputOverrides[i];
    }
  }
  return nullptr;
}

static bool canKeepManualOverrideOn(const ManualOutputOverride& ovr, bool logReason) {
  if (!ovr.allowManualForce) {
    if (logReason) {
      LOG_W("Manual override blocked for safety-critical output: %s", ovr.id);
    }
    return false;
  }
  if (!bike.ignitionOn) return false;
  return true;
}

static bool isManualOverrideForcedOn(const char* id) {
  ManualOutputOverride* ovr = findManualOverrideById(id);
  return ovr ? (ovr->forcedOn && canKeepManualOverrideOn(*ovr, false)) : false;
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

static bool wsReadyForBroadcast() {
  if (ws.count() == 0) return false;
  if (ws.availableForWriteAll()) return true;

  const unsigned long now = millis();
  if (now - lastWsBackpressureLog >= 2000) {
    lastWsBackpressureLog = now;
    LOG_W("WS backpressure: outbound queue is full, dropping frame");
  }
  return false;
}

static bool wsBroadcastText(const String& payload) {
  if (!wsReadyForBroadcast()) return false;
  const AsyncWebSocket::SendStatus status = ws.textAll(payload);
  if (status == AsyncWebSocket::DISCARDED) {
    return false;
  }
  return true;
}

static void copySettingString(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, dstSize, "%s", src);
}

static bool hasVisibleChars(const char* text) {
  if (!text) return false;
  while (*text) {
    if (!isspace((unsigned char)*text)) return true;
    text++;
  }
  return false;
}

static bool profileReady(const Settings& s) {
  return hasVisibleChars(s.bikeBrand)
      && hasVisibleChars(s.bikeModel)
      && hasVisibleChars(s.driverName);
}

static const char* resetReasonCodeToText(uint8_t code) {
  switch (code) {
    case ESP_RST_POWERON:   return "POWERON_RESET";
    case ESP_RST_EXT:       return "EXT_RESET";
    case ESP_RST_SW:        return "SW_RESET";
    case ESP_RST_PANIC:     return "PANIC_RESET";
    case ESP_RST_INT_WDT:   return "INT_WDT_RESET";
    case ESP_RST_TASK_WDT:  return "TASK_WDT_RESET";
    case ESP_RST_WDT:       return "WDT_RESET";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP_RESET";
    case ESP_RST_BROWNOUT:  return "BROWNOUT_RESET";
    case ESP_RST_SDIO:      return "SDIO_RESET";
    default:                return "UNKNOWN_RESET";
  }
}

static void makeApSsid(char* out, size_t outLen, bool openFallback) {
  if (!out || outLen == 0) return;
  uint16_t suffix = (uint16_t)(ESP.getEfuseMac() & 0xFFFF);
  if (openFallback) {
    snprintf(out, outLen, "%s-OPEN-%04X", AP_SSID, suffix);
  } else {
    snprintf(out, outLen, "%s-%04X", AP_SSID, suffix);
  }
}

// ============================================================================
// JSON STATE BUILDER
// ============================================================================

static void buildStateJson(JsonDocument& doc) {
  doc["type"] = "state";
  doc["voltage"] = round(bike.batteryVoltage * 10.0f) / 10.0f;
  doc["voltageAvailable"] = bike.batteryVoltageAvailable;
  doc["errorFlags"] = bike.errorFlags;
  doc["ignitionOn"] = bike.ignitionOn;
  doc["engineRunning"] = bike.engineRunning;
  doc["starterEngaged"] = bike.starterEngaged;
  doc["killActive"] = bike.killActive;
  doc["bleConnected"] = bike.bleConnected;
  doc["speedPulses"] = bike.speedPulseCount;
  doc["safeMode"] = bike.safeModeActive;
  doc["bootLoopCounter"] = bike.bootLoopCounter;
  doc["resetReasonCode"] = bike.resetReasonCode;
  doc["coreDumpPartitionFound"] = bike.coreDumpPartitionFound;
  doc["turnCalActive"] = turnDistanceCalibrationActive;
  doc["turnCalPulses"] = turnDistanceCalibrationActive
      ? (bike.speedPulseCount - turnDistanceCalibrationStartPulses)
      : 0;

  // Inputs
  JsonObject ins = doc["inputs"].to<JsonObject>();
  ins["lock"]  = lockEvent.state;
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
  ins["speed"] = inputActive(PIN_SPEED);
  ins["speed_info"] = String(bike.speedPulseCount) + " Pulse";
  for (size_t i = 0; i < MANUAL_INPUT_OVERRIDE_COUNT; i++) {
    char key[24];
    snprintf(key, sizeof(key), "%s_manual", manualInputOverrides[i].id);
    ins[key] = manualInputOverrides[i].enabled;
    if (manualInputOverrides[i].enabled) {
      char stateKey[30];
      snprintf(stateKey, sizeof(stateKey), "%s_manual_state", manualInputOverrides[i].id);
      ins[stateKey] = manualInputOverrides[i].forcedActive;
    }
  }

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
  for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
    char key[28];
    snprintf(key, sizeof(key), "%s_manual", manualOutputOverrides[i].id);
    outs[key] = manualOutputOverrides[i].forcedOn;
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
  doc["low"]       = settings.lowBeamMode;
  doc["aux1"]      = settings.aux1Mode;
  doc["aux2"]      = settings.aux2Mode;
  doc["stand"]     = settings.standKillMode;
  doc["park"]      = settings.parkingLightMode;
  doc["drlsrc"]    = settings.daytimeLightSource;
  doc["drldim"]    = settings.daytimeLightDimPercent;
  doc["tdist"]     = settings.turnDistancePulsesTarget;
  doc["bikeBrand"] = settings.bikeBrand;
  doc["bikeModel"] = settings.bikeModel;
  doc["driverName"] = settings.driverName;
  doc["profileReady"] = profileReady(settings);
  doc["profileSkip"] = settings.profileSetupSkipped;
  doc["firmware"] = FIRMWARE_VERSION_STRING;
}

static void broadcastSettingsJson() {
  if (ws.count() == 0) return;
  JsonDocument doc;
  buildSettingsJson(doc);
  String out;
  serializeJson(doc, out);
  wsBroadcastText(out);
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
                            action.keylessRssiLevel,
                            action.keylessActiveMinutes,
                            action.keylessActivationMode,
                            action.keylessActivationButton);
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

      case WebActionType::CLEAR_INPUT_OVERRIDES:
        for (size_t i = 0; i < MANUAL_INPUT_OVERRIDE_COUNT; i++) {
          manualInputOverrides[i].enabled = false;
          manualInputOverrides[i].forcedActive = false;
        }
        inputClearAllManualOverrides();
        LOG_I("Manual input overrides cleared");
        break;

      case WebActionType::TOGGLE_INPUT: {
        if (bike.safeModeActive) {
          LOG_W("Manual input override blocked in SAFE MODE");
          break;
        }
        ManualInputOverride* ovr = findManualInputOverrideById(action.inputId);
        if (ovr) {
          if (!ovr->enabled) {
            ovr->enabled = true;
            ovr->forcedActive = true;
          } else if (ovr->forcedActive) {
            ovr->enabled = true;
            ovr->forcedActive = false;
          } else {
            ovr->enabled = false;
            ovr->forcedActive = false;
          }
          inputSetManualOverride(ovr->pin, ovr->enabled, ovr->forcedActive);
          LOG_W("Manual input override %s: %s",
                action.inputId,
                !ovr->enabled ? "LIVE"
                              : (ovr->forcedActive ? "MANUAL ON" : "MANUAL OFF"));
        }
        break;
      }

      case WebActionType::CLEAR_INPUT_OVERRIDE: {
        ManualInputOverride* ovr = findManualInputOverrideById(action.inputId);
        if (ovr) {
          ovr->enabled = false;
          ovr->forcedActive = false;
          inputSetManualOverride(ovr->pin, false, false);
          LOG_I("Manual input override cleared: %s", action.inputId);
        }
        break;
      }

      case WebActionType::TOGGLE_OUTPUT: {
        if (bike.safeModeActive) {
          LOG_W("Manual output override blocked in SAFE MODE");
          break;
        }
        ManualOutputOverride* ovr = findManualOverrideById(action.outputId);
        if (ovr) {
          const bool enable = !ovr->forcedOn;
          if (enable && !canKeepManualOverrideOn(*ovr, true)) {
            break;
          }
          ovr->forcedOn = enable;
          LOG_W("Manual output override %s: %s",
                action.outputId, ovr->forcedOn ? "ON" : "OFF");
        }
        break;
      }

      case WebActionType::CLEAR_OUTPUT_OVERRIDE: {
        ManualOutputOverride* ovr = findManualOverrideById(action.outputId);
        if (ovr && ovr->forcedOn) {
          ovr->forcedOn = false;
          LOG_I("Manual output override cleared: %s", action.outputId);
        }
        break;
      }

      case WebActionType::TOGGLE_AUX1:
        if (bike.safeModeActive) {
          LOG_W("AUX1 manual toggle blocked in SAFE MODE");
          break;
        }
        bike.aux1ManualOn = !bike.aux1ManualOn;
        LOG_D("AUX1 manual: %s", bike.aux1ManualOn ? "ON" : "OFF");
        break;

      case WebActionType::TOGGLE_AUX2:
        if (bike.safeModeActive) {
          LOG_W("AUX2 manual toggle blocked in SAFE MODE");
          break;
        }
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
    action.settings.moWaveEnabled    = false;
    action.settings.lowBeamMode      = d["low"] | 0;
    action.settings.aux1Mode         = d["aux1"] | 0;
    action.settings.aux2Mode         = d["aux2"] | 0;
    action.settings.standKillMode    = d["stand"] | 0;
    action.settings.parkingLightMode = d["park"] | 0;
    action.settings.daytimeLightSource = constrain((int)d["drlsrc"], 0, 4);
    {
      int drlDim = d["drldim"] | 50;
      if (drlDim != 25 && drlDim != 50 && drlDim != 75 && drlDim != 100) {
        drlDim = 50;
      }
      action.settings.daytimeLightDimPercent = drlDim;
    }
    action.settings.turnDistancePulsesTarget = constrain(
        (int)d["tdist"], TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
    if (d["bikeBrand"].is<const char*>()) {
      copySettingString(action.settings.bikeBrand,
                        sizeof(action.settings.bikeBrand),
                        d["bikeBrand"]);
    } else if (d["brand"].is<const char*>()) {
      copySettingString(action.settings.bikeBrand,
                        sizeof(action.settings.bikeBrand),
                        d["brand"]);
    }
    if (d["bikeModel"].is<const char*>()) {
      copySettingString(action.settings.bikeModel,
                        sizeof(action.settings.bikeModel),
                        d["bikeModel"]);
    } else if (d["model"].is<const char*>()) {
      copySettingString(action.settings.bikeModel,
                        sizeof(action.settings.bikeModel),
                        d["model"]);
    }
    if (d["driverName"].is<const char*>()) {
      copySettingString(action.settings.driverName,
                        sizeof(action.settings.driverName),
                        d["driverName"]);
    } else if (d["driver"].is<const char*>()) {
      copySettingString(action.settings.driverName,
                        sizeof(action.settings.driverName),
                        d["driver"]);
    }
    if (d["profileSkip"].is<bool>()) {
      action.settings.profileSetupSkipped = d["profileSkip"];
    }
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
    int requestedLevel = d["rssiLevel"] | 0;
    if (requestedLevel >= 1 && requestedLevel <= 6) {
      action.keylessRssiLevel = requestedLevel;
    } else {
      // Backward compatibility for older web clients that still send dBm.
      action.keylessRssiLevel = d["rssiThreshold"] | 4;
    }
    const int requestedMinutes = d["activeMinutes"] | 0;
    if (requestedMinutes > 0) {
      action.keylessActiveMinutes = requestedMinutes;
    } else {
      // Backward compatibility for older web clients that still send grace seconds.
      action.keylessActiveMinutes = d["graceSeconds"] | 300;
    }
    action.keylessActivationMode = d["activationMode"] | 0;
    action.keylessActivationButton = d["activationButton"] | 0;
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

  else if (strcmp(cmd, "clearInputOverrides") == 0) {
    WebAction action = {};
    action.type = WebActionType::CLEAR_INPUT_OVERRIDES;
    enqueueWsAction(action, cmd);
  }

  // ---- Direct input toggle (ADVANCED mode in UI) ----
  else if (strcmp(cmd, "toggleInput") == 0) {
    const char* id = doc["id"];
    if (id) {
      ManualInputOverride* ovr = findManualInputOverrideById(id);
      if (!ovr) {
        LOG_W("Web: toggleInput ignored, unknown id=%s", id);
        return;
      }
      WebAction action = {};
      action.type = WebActionType::TOGGLE_INPUT;
      snprintf(action.inputId, sizeof(action.inputId), "%s", id);
      enqueueWsAction(action, cmd);
    }
  }

  else if (strcmp(cmd, "clearInputOverride") == 0) {
    const char* id = doc["id"];
    if (id) {
      ManualInputOverride* ovr = findManualInputOverrideById(id);
      if (!ovr) {
        LOG_W("Web: clearInputOverride ignored, unknown id=%s", id);
        return;
      }
      WebAction action = {};
      action.type = WebActionType::CLEAR_INPUT_OVERRIDE;
      snprintf(action.inputId, sizeof(action.inputId), "%s", id);
      enqueueWsAction(action, cmd);
    }
  }

  // ---- Direct output toggle (ADVANCED mode in UI) ----
  else if (strcmp(cmd, "toggleOutput") == 0) {
    const char* id = doc["id"];
    if (id) {
      ManualOutputOverride* ovr = findManualOverrideById(id);
      if (!ovr) {
        LOG_W("Web: toggleOutput ignored, unknown id=%s", id);
        return;
      }
      if (!ovr->allowManualForce) {
        LOG_W("Web: toggleOutput blocked for critical output id=%s", id);
        return;
      }
      WebAction action = {};
      action.type = WebActionType::TOGGLE_OUTPUT;
      snprintf(action.outputId, sizeof(action.outputId), "%s", id);
      enqueueWsAction(action, cmd);
    }
  }

  else if (strcmp(cmd, "clearOutputOverride") == 0) {
    const char* id = doc["id"];
    if (id) {
      ManualOutputOverride* ovr = findManualOverrideById(id);
      if (!ovr) {
        LOG_W("Web: clearOutputOverride ignored, unknown id=%s", id);
        return;
      }
      WebAction action = {};
      action.type = WebActionType::CLEAR_OUTPUT_OVERRIDE;
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

void webSetReducedLoadMode(bool enabled) {
  stateBroadcastIntervalMs = enabled
      ? BROADCAST_INTERVAL_MS_REDUCED
      : BROADCAST_INTERVAL_MS_DEFAULT;
  keylessCheckIntervalMs = enabled
      ? KEYLESS_CHECK_INTERVAL_MS_REDUCED
      : KEYLESS_CHECK_INTERVAL_MS_DEFAULT;
  keylessKeepaliveMs = enabled
      ? KEYLESS_KEEPALIVE_MS_REDUCED
      : KEYLESS_KEEPALIVE_MS_DEFAULT;
  LOG_I("Web load mode: %s (state=%lums keyless=%lums)",
        enabled ? "REDUCED" : "NORMAL",
        stateBroadcastIntervalMs, keylessCheckIntervalMs);
}

void webInit() {
  inputClearAllManualOverrides();

  // WiFi Access Point
  WiFi.persistent(false);
  WiFi.softAPdisconnect(true);
  delay(80);
  WiFi.mode(WIFI_MODE_NULL);
  delay(30);
  WiFi.mode(WIFI_AP);
  makeApSsid(activeApSsid, sizeof(activeApSsid), false);
  bool apOk = WiFi.softAP(
      activeApSsid, AP_PASS, AP_CHANNEL, 0, AP_MAX_CONNECTIONS, false);
  activeApOpenFallback = false;
  if (!apOk) {
    LOG_E("WiFi AP start failed with WPA2 (ssid=%s, passLen=%u). "
          "Retrying OPEN fallback.",
          activeApSsid, (unsigned)strlen(AP_PASS));
    makeApSsid(activeApSsid, sizeof(activeApSsid), true);
    apOk = WiFi.softAP(
        activeApSsid, nullptr, AP_CHANNEL, 0, AP_MAX_CONNECTIONS, false);
    activeApOpenFallback = apOk;
  }
  if (!apOk) {
    LOG_E("WiFi AP failed to start");
  } else {
    LOG_I("WiFi AP started: %s (IP: %s, CH=%u, auth=%s)",
          activeApSsid, WiFi.softAPIP().toString().c_str(),
          AP_CHANNEL, activeApOpenFallback ? "OPEN" : "WPA2-PSK");
    if (!activeApOpenFallback) {
      LOG_I("WiFi AP password: %s", AP_PASS);
    }
  }

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
    AsyncWebServerResponse* resp = req->beginResponse(
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
      wsBroadcastText(out);
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
    doc["safeMode"] = bike.safeModeActive;
    doc["bootLoopCounter"] = bike.bootLoopCounter;
    doc["resetReasonCode"] = bike.resetReasonCode;
    doc["resetReason"] = resetReasonCodeToText(bike.resetReasonCode);
    doc["coreDumpPartitionFound"] = bike.coreDumpPartitionFound;
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
  if (bike.safeModeActive) {
    bool cleared = false;
    for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
      if (manualOutputOverrides[i].forcedOn) {
        manualOutputOverrides[i].forcedOn = false;
        cleared = true;
      }
    }
    if (cleared) {
      LOG_I("Manual output overrides cleared (SAFE MODE)");
    }
    return;
  }

  if (!bike.ignitionOn) {
    bool cleared = false;
    for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
      if (manualOutputOverrides[i].forcedOn) {
        manualOutputOverrides[i].forcedOn = false;
        cleared = true;
      }
    }
    if (cleared) {
      LOG_I("Manual output overrides cleared (ignition off)");
    }
    return;
  }

  for (size_t i = 0; i < MANUAL_OUTPUT_OVERRIDE_COUNT; i++) {
    ManualOutputOverride& ovr = manualOutputOverrides[i];
    if (!ovr.forcedOn) continue;
    if (!canKeepManualOverrideOn(ovr, true)) {
      ovr.forcedOn = false;
      continue;
    }
    outputOn(ovr.pin);
  }
}

void webUpdate() {
  unsigned long now = millis();

  // Execute deferred operations on loop task (not async_tcp task).
  processQueuedWsActions();

  if (now - lastStateCheck < stateBroadcastIntervalMs) return;
  lastStateCheck = now;

  // Cleanup dead connections
  if (now - lastWsCleanup >= WS_CLEANUP_INTERVAL_MS) {
    lastWsCleanup = now;
    ws.cleanupClients();
  }

  if (ws.count() == 0) return;
  if (!wsReadyForBroadcast()) return;

  // Build state and broadcast only on change (or keepalive).
  JsonDocument doc;
  buildStateJson(doc);
  String stateOut;
  serializeJson(doc, stateOut);
  if (stateOut != lastStatePayload
      || (now - lastStateBroadcast) >= STATE_KEEPALIVE_MS) {
    if (wsBroadcastText(stateOut)) {
      lastStatePayload = stateOut;
      lastStateBroadcast = now;
    }
  }

  // Keyless status is less dynamic than core state.
  if (now - lastKeylessCheck >= keylessCheckIntervalMs) {
    lastKeylessCheck = now;
    JsonDocument kdoc;
    bleKeylessBuildJson(kdoc);
    String keylessOut;
    serializeJson(kdoc, keylessOut);
    if (keylessOut != lastKeylessPayload
        || (now - lastKeylessBroadcast) >= keylessKeepaliveMs) {
      if (wsBroadcastText(keylessOut)) {
        lastKeylessPayload = keylessOut;
        lastKeylessBroadcast = now;
      }
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
  wsBroadcastText(out);
}
