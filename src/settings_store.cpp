#include "settings_store.h"
#include <Preferences.h>
#include <nvs_flash.h>

static Preferences preferences;
static const char* NAMESPACE = "moto32";

static uint8_t normalizeDrlDimPercent(uint8_t percent) {
  switch (percent) {
    case 25:
    case 50:
    case 75:
    case 100:
      return percent;
    default:
      return 50;
  }
}

static void sanitizeProfileStrings() {
  settings.bikeBrand[sizeof(settings.bikeBrand) - 1] = '\0';
  settings.bikeModel[sizeof(settings.bikeModel) - 1] = '\0';
  settings.driverName[sizeof(settings.driverName) - 1] = '\0';
}

static bool beginPreferencesWithRecovery(Preferences& pref,
                                         const char* nameSpace,
                                         bool readOnly) {
  if (pref.begin(nameSpace, readOnly)) return true;

  LOG_E("NVS open failed for '%s' (ro=%d)", nameSpace, readOnly ? 1 : 0);

  esp_err_t nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES
      || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    LOG_W("NVS requires erase (%s), reinitializing", esp_err_to_name(nvsErr));
    esp_err_t eraseErr = nvs_flash_erase();
    if (eraseErr != ESP_OK) {
      LOG_E("NVS erase failed: %s", esp_err_to_name(eraseErr));
      return false;
    }
    nvsErr = nvs_flash_init();
  }

  if (nvsErr != ESP_OK) {
    LOG_E("NVS init failed: %s", esp_err_to_name(nvsErr));
    return false;
  }

  if (!pref.begin(nameSpace, readOnly)) {
    LOG_E("NVS reopen failed for '%s' after recovery", nameSpace);
    return false;
  }

  LOG_W("NVS recovered for '%s'", nameSpace);
  return true;
}

void saveSettings() {
  if (!beginPreferencesWithRecovery(preferences, NAMESPACE, false)) {
    LOG_E("Settings save skipped: NVS unavailable");
    return;
  }
  preferences.putUChar ("handlebar", settings.handlebarConfig);
  preferences.putUChar ("rear",      settings.rearLightMode);
  preferences.putUChar ("turn",      settings.turnSignalMode);
  preferences.putUChar ("brake",     settings.brakeLightMode);
  preferences.putUChar ("alarm",     settings.alarmMode);
  preferences.putUChar ("pos",       settings.positionLight);
  preferences.putBool  ("wave",      false);
  preferences.putUChar ("low",       settings.lowBeamMode);
  preferences.putUChar ("aux1",      settings.aux1Mode);
  preferences.putUChar ("aux2",      settings.aux2Mode);
  preferences.putUChar ("stand",     settings.standKillMode);
  preferences.putUChar ("park",      settings.parkingLightMode);
  preferences.putUChar ("drlsrc",    settings.daytimeLightSource);
  preferences.putUChar ("drldim",    normalizeDrlDimPercent(settings.daytimeLightDimPercent));
  preferences.putUShort("tdist",     settings.turnDistancePulsesTarget);
  preferences.putString("brand",     settings.bikeBrand);
  preferences.putString("model",     settings.bikeModel);
  preferences.putString("driver",    settings.driverName);
  preferences.putBool  ("proskip",   settings.profileSetupSkipped);
  preferences.end();
  LOG_I("Settings saved");
}

void loadSettings() {
  if (!beginPreferencesWithRecovery(preferences, NAMESPACE, false)) {
    LOG_W("Settings load fallback: using in-memory defaults");
    settings = Settings{};
    return;
  }
  settings.handlebarConfig  = static_cast<HandlebarConfig>(
      preferences.getUChar("handlebar", settings.handlebarConfig));
  settings.rearLightMode    = preferences.getUChar ("rear",  settings.rearLightMode);
  settings.turnSignalMode   = static_cast<TurnSignalMode>(
      preferences.getUChar("turn", settings.turnSignalMode));
  settings.brakeLightMode   = static_cast<BrakeLightMode>(
      preferences.getUChar("brake", settings.brakeLightMode));
  settings.alarmMode        = preferences.getUChar ("alarm", settings.alarmMode);
  settings.positionLight    = preferences.getUChar ("pos",   settings.positionLight);
  settings.moWaveEnabled    = false;
  settings.lowBeamMode      = preferences.getUChar ("low",   settings.lowBeamMode);
  settings.aux1Mode         = preferences.getUChar ("aux1",  settings.aux1Mode);
  settings.aux2Mode         = preferences.getUChar ("aux2",  settings.aux2Mode);
  settings.standKillMode    = preferences.getUChar ("stand", settings.standKillMode);
  settings.parkingLightMode = preferences.getUChar ("park",  settings.parkingLightMode);
  settings.daytimeLightSource = preferences.getUChar("drlsrc", settings.daytimeLightSource);
  settings.daytimeLightDimPercent =
      preferences.getUChar("drldim", settings.daytimeLightDimPercent);
  settings.turnDistancePulsesTarget =
      preferences.getUShort("tdist", settings.turnDistancePulsesTarget);
  settings.bikeBrand[0] = '\0';
  settings.bikeModel[0] = '\0';
  settings.driverName[0] = '\0';
  preferences.getString("brand", settings.bikeBrand, sizeof(settings.bikeBrand));
  preferences.getString("model", settings.bikeModel, sizeof(settings.bikeModel));
  preferences.getString("driver", settings.driverName, sizeof(settings.driverName));
  settings.profileSetupSkipped =
      preferences.getBool("proskip", settings.profileSetupSkipped);
  preferences.end();

  // Validate / clamp loaded values
  settings.handlebarConfig = static_cast<HandlebarConfig>(
      constrain(settings.handlebarConfig, CONFIG_A, CONFIG_E));
  settings.turnSignalMode = static_cast<TurnSignalMode>(
      constrain(settings.turnSignalMode, TURN_OFF, TURN_30S));
  settings.brakeLightMode = static_cast<BrakeLightMode>(
      constrain(settings.brakeLightMode, BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
  settings.positionLight = constrain(settings.positionLight, 0, 9);
  settings.daytimeLightSource = constrain(settings.daytimeLightSource, 0, 4);
  settings.daytimeLightDimPercent =
      normalizeDrlDimPercent(settings.daytimeLightDimPercent);
  settings.turnDistancePulsesTarget = constrain(
      settings.turnDistancePulsesTarget,
      TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
  sanitizeProfileStrings();

  LOG_I("Settings loaded");
}
