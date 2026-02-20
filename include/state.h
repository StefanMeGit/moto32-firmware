#pragma once

#include "config.h"

// ============================================================================
// SETTINGS (persisted to NVS)
// ============================================================================

struct Settings {
  HandlebarConfig handlebarConfig = CONFIG_C;
  uint8_t         rearLightMode   = 0;
  TurnSignalMode  turnSignalMode  = TURN_10S;
  BrakeLightMode  brakeLightMode  = BRAKE_CONTINUOUS;
  uint8_t         alarmMode       = 0;
  uint8_t         positionLight   = 0;       // 0-9 → 0-50% brightness
  bool            moWaveEnabled   = false;
  uint8_t         lowBeamMode     = 0;       // 0 = on after engine start
  uint8_t         aux1Mode        = 0;
  uint8_t         aux2Mode        = 0;
  uint8_t         standKillMode   = 0;       // %3: 0=N/O, 1=N/C, 2=disabled
  uint8_t         parkingLightMode = 0;
  uint16_t        turnDistancePulsesTarget = 50;
};

// ============================================================================
// BUTTON EVENT
// ============================================================================

struct ButtonEvent {
  bool          state            = false;
  bool          pressed          = false;   // Rising edge this cycle
  bool          released         = false;   // Falling edge this cycle
  bool          longPress        = false;   // One-shot on long press threshold
  bool          doubleClick      = false;   // Double-click detected
  unsigned long pressStartTime   = 0;
  unsigned long lastReleaseTime  = 0;
  bool          longPressLatched = false;
};

// ============================================================================
// BIKE STATE (runtime, not persisted)
// ============================================================================

struct BikeState {
  // Core state
  bool ignitionOn       = false;
  bool engineRunning    = false;
  bool starterEngaged   = false;   // NEW: starter physically running
  bool killActive       = false;

  // Lights
  bool lowBeamOn        = false;
  bool highBeamOn       = false;

  // Turn signals
  bool leftTurnOn       = false;
  bool rightTurnOn      = false;
  bool hazardLightsOn   = false;
  bool emergencyHazardActive  = false;
  bool manualHazardRequested  = false;

  // Inputs
  bool brakePressed     = false;
  bool hornPressed      = false;
  bool standDown        = false;   // NEW: sidestand state

  // Turn signal timing
  unsigned long leftTurnStartTime   = 0;
  unsigned long rightTurnStartTime  = 0;
  unsigned long leftTurnStartPulses = 0;
  unsigned long rightTurnStartPulses = 0;

  // Brake timing
  unsigned long brakePressTime  = 0;
  unsigned long lastBrakeFlash  = 0;
  bool          brakeFlashState = false;

  // Starter timing
  unsigned long starterStartTime = 0;

  // Flasher
  unsigned long lastFlasherToggle = 0;
  bool          flasherState      = false;

  // mo.wave sequential animation
  uint8_t       waveStep          = 0;    // Current step (0 = both off, 1-2 = sequence, 3 = both on)
  unsigned long waveStepStart     = 0;    // millis() when current step began
  bool          wavePhaseOn       = true; // Current blink phase (on/off)

  // Speed sensor
  unsigned long speedPulseCount = 0;

  // Alarm
  bool          alarmArmed        = false;  // Alarm is armed
  bool          alarmTriggered    = false;  // Alarm currently sounding
  unsigned long alarmTriggerTime  = 0;      // When alarm was triggered
  unsigned long lastAlarmCheck    = 0;      // Debounce for vibration sensor

  // Setup mode
  bool          inSetupMode     = false;
  unsigned long setupEnterTime  = 0;

  // Calibration
  CalibrationState calibrationState     = CALIB_IDLE;
  int              calibrationStepIndex = 0;
  unsigned long    calibrationStepStart = 0;
  bool             calibrationStepOutputOn = false;

  // Battery
  float   batteryVoltage    = 12.6f;
  uint8_t errorFlags        = ERR_NONE;
  bool    lowVoltageWarning = false;

  // BLE
  bool bleConnected = false;
};

// ============================================================================
// GLOBAL INSTANCES (defined in main.cpp)
// ============================================================================

extern Settings  settings;
extern BikeState bike;

extern ButtonEvent turnLeftEvent;
extern ButtonEvent turnRightEvent;
extern ButtonEvent lightEvent;
extern ButtonEvent startEvent;
extern ButtonEvent hornEvent;
extern ButtonEvent lockEvent;
