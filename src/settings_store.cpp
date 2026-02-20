#include "settings_store.h"
#include <Preferences.h>

static Preferences preferences;
static const char* NAMESPACE = "moto32";

void saveSettings() {
  preferences.begin(NAMESPACE, false);
  preferences.putUChar ("handlebar", settings.handlebarConfig);
  preferences.putUChar ("rear",      settings.rearLightMode);
  preferences.putUChar ("turn",      settings.turnSignalMode);
  preferences.putUChar ("brake",     settings.brakeLightMode);
  preferences.putUChar ("alarm",     settings.alarmMode);
  preferences.putUChar ("pos",       settings.positionLight);
  preferences.putBool  ("wave",      settings.moWaveEnabled);
  preferences.putUChar ("low",       settings.lowBeamMode);
  preferences.putUChar ("aux1",      settings.aux1Mode);
  preferences.putUChar ("aux2",      settings.aux2Mode);
  preferences.putUChar ("stand",     settings.standKillMode);
  preferences.putUChar ("park",      settings.parkingLightMode);
  preferences.putUShort("tdist",     settings.turnDistancePulsesTarget);
  preferences.end();
  LOG_I("Settings saved");
}

void loadSettings() {
  preferences.begin(NAMESPACE, true);
  settings.handlebarConfig  = static_cast<HandlebarConfig>(
      preferences.getUChar("handlebar", settings.handlebarConfig));
  settings.rearLightMode    = preferences.getUChar ("rear",  settings.rearLightMode);
  settings.turnSignalMode   = static_cast<TurnSignalMode>(
      preferences.getUChar("turn", settings.turnSignalMode));
  settings.brakeLightMode   = static_cast<BrakeLightMode>(
      preferences.getUChar("brake", settings.brakeLightMode));
  settings.alarmMode        = preferences.getUChar ("alarm", settings.alarmMode);
  settings.positionLight    = preferences.getUChar ("pos",   settings.positionLight);
  settings.moWaveEnabled    = preferences.getBool  ("wave",  settings.moWaveEnabled);
  settings.lowBeamMode      = preferences.getUChar ("low",   settings.lowBeamMode);
  settings.aux1Mode         = preferences.getUChar ("aux1",  settings.aux1Mode);
  settings.aux2Mode         = preferences.getUChar ("aux2",  settings.aux2Mode);
  settings.standKillMode    = preferences.getUChar ("stand", settings.standKillMode);
  settings.parkingLightMode = preferences.getUChar ("park",  settings.parkingLightMode);
  settings.turnDistancePulsesTarget =
      preferences.getUShort("tdist", settings.turnDistancePulsesTarget);
  preferences.end();

  // Validate / clamp loaded values
  settings.handlebarConfig = static_cast<HandlebarConfig>(
      constrain(settings.handlebarConfig, CONFIG_A, CONFIG_E));
  settings.turnSignalMode = static_cast<TurnSignalMode>(
      constrain(settings.turnSignalMode, TURN_OFF, TURN_30S));
  settings.brakeLightMode = static_cast<BrakeLightMode>(
      constrain(settings.brakeLightMode, BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
  settings.positionLight = constrain(settings.positionLight, 0, 9);
  settings.turnDistancePulsesTarget = constrain(
      settings.turnDistancePulsesTarget,
      TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);

  LOG_I("Settings loaded");
}
