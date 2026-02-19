#include <Arduino.h>
#include <Preferences.h>

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Inputs (12 total) - switch to ground
#define PIN_LOCK     46    // Ignition lock (connects to +12V)
#define PIN_TURNL    47    // Turn left
#define PIN_TURNR    48    // Turn right
#define PIN_LIGHT    21    // Light control
#define PIN_START    22    // Starter button
#define PIN_HORN     23    // Horn button
#define PIN_BRAKE    1     // Brake switch (front/rear parallel)
#define PIN_KILL     2     // Kill switch
#define PIN_STAND    3     // Side stand switch
#define PIN_AUX1     4     // Auxiliary input 1
#define PIN_AUX2     5     // Auxiliary input 2
#define PIN_SPEED    6     // Speed sensor (optional, on AUX2 when used)

// Outputs (10 total) - switch +12V, loads connected to ground
#define PIN_TURNL_OUT    9   // Left turn indicator
#define PIN_TURNR_OUT   10   // Right turn indicator
#define PIN_LIGHT_OUT   11   // Low beam
#define PIN_HIBEAM_OUT  12   // High beam
#define PIN_BRAKE_OUT   13   // Brake light
#define PIN_HORN_OUT    41   // Horn relay
#define PIN_START_OUT1  44   // Starter output 1 (2 wires for 30A)
#define PIN_START_OUT2  45   // Starter output 2
#define PIN_IGN_OUT     42   // Ignition system
#define PIN_AUX1_OUT    43   // Auxiliary output 1
#define PIN_AUX2_OUT    40   // Auxiliary output 2

// Status LED pins (if available)
#define LED_STATUS     38   // Built-in LED or status indicator

// ============================================================================
// CONFIGURATION STRUCTURES
// ============================================================================

enum HandlebarConfig {
  CONFIG_A = 0,  // 5 pushbuttons
  CONFIG_B = 1,  // Harley/BMW
  CONFIG_C = 2,  // Japanese/European
  CONFIG_D = 3,  // Ducati
  CONFIG_E = 4   // 4 pushbuttons
};

enum TurnSignalMode {
  TURN_OFF = 0,
  TURN_DISTANCE = 1,  // 50m or 10s at >10km/h
  TURN_10S = 2,
  TURN_20S = 3,
  TURN_30S = 4
};

enum BrakeLightMode {
  BRAKE_CONTINUOUS = 0,
  BRAKE_FADE = 1,      // Fade in/out 3Hz
  BRAKE_FLASH_5HZ = 2,
  BRAKE_FLASH_8X = 3,  // 8x flash then continuous
  BRAKE_FLASH_2X = 4,  // 2x flash then 1s continuous
  BRAKE_3S_FLASH = 5,  // 3s continuous then flash
  BRAKE_EMERGENCY = 6  // Emergency braking mode
};

struct Settings {
  HandlebarConfig handlebarConfig;
  uint8_t rearLightMode;      // Menu 2
  TurnSignalMode turnSignalMode;
  BrakeLightMode brakeLightMode;
  uint8_t alarmMode;          // Menu 5
  uint8_t positionLight;      // Menu 6 (0-9, brightness 0-50%)
  bool moWaveEnabled;         // Menu 7
  uint8_t lowBeamMode;        // Menu 8
  uint8_t aux1Mode;           // Menu 9
  uint8_t aux2Mode;           // Menu 10
  uint8_t standKillMode;      // Menu 11
  uint8_t parkingLightMode;   // Menu 12
  uint16_t turnDistancePulsesTarget; // Menu 13 (pulse target for TURN_DISTANCE)
};

// ============================================================================
// STATE VARIABLES
// ============================================================================

Settings settings = {
  .handlebarConfig = CONFIG_C,
  .rearLightMode = 0,
  .turnSignalMode = TURN_10S,
  .brakeLightMode = BRAKE_CONTINUOUS,
  .alarmMode = 0,  // Alarm deactivated
  .positionLight = 0,
  .moWaveEnabled = false,
  .lowBeamMode = 0,  // Low beam on after engine start
  .aux1Mode = 0,
  .aux2Mode = 0,
  .standKillMode = 0,
  .parkingLightMode = 0,
  .turnDistancePulsesTarget = 50
};

// State flags
bool ignitionOn = false;
bool engineRunning = false;
bool lowBeamOn = false;
bool highBeamOn = false;
bool leftTurnOn = false;
bool rightTurnOn = false;
bool hazardLightsOn = false;
bool brakePressed = false;
bool hornPressed = false;
bool startPressed = false;
bool killActive = false;

// Timing variables
unsigned long leftTurnStartTime = 0;
unsigned long rightTurnStartTime = 0;
unsigned long brakePressTime = 0;
unsigned long lastBrakeFlash = 0;
bool brakeFlashState = false;
unsigned long hornPressTime = 0;
unsigned long startPressTime = 0;
bool inSetupMode = false;
unsigned long setupEnterTime = 0;

// Flasher timing (1.5Hz = ~667ms period)
const unsigned long FLASHER_PERIOD = 667;
unsigned long lastFlasherToggle = 0;
bool flasherState = false;

// Button debounce
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long TURN_DISTANCE_TIMEOUT = 10000;
const uint16_t TURN_DISTANCE_MIN_PULSES = 10;
const uint16_t TURN_DISTANCE_MAX_PULSES = 1000;

const unsigned long SETUP_FLASH_INTERVAL = 200;
const unsigned long CALIBRATION_STEP_DURATION = 500;

// Speed sensor pulse tracking (used for TURN_DISTANCE)
unsigned long speedPulseCount = 0;
unsigned long leftTurnStartPulses = 0;
unsigned long rightTurnStartPulses = 0;

// Persistent settings
Preferences preferences;
const char* SETTINGS_NAMESPACE = "moto32";

// Emergency hazard ownership flag
bool emergencyHazardActive = false;
bool manualHazardRequested = false;

struct ButtonEvent {
  bool state = false;
  bool pressed = false;
  bool released = false;
  bool longPress = false;
  bool doubleClick = false;
  unsigned long pressStartTime = 0;
  unsigned long lastReleaseTime = 0;
  bool longPressLatched = false;
};

ButtonEvent turnLeftEvent;
ButtonEvent turnRightEvent;
ButtonEvent lightEvent;
ButtonEvent startEvent;
ButtonEvent hornEvent;
ButtonEvent lockEvent;

enum CalibrationState {
  CALIB_IDLE,
  CALIB_RUNNING,
  CALIB_DONE
};

CalibrationState calibrationState = CALIB_IDLE;
int calibrationStepIndex = 0;
unsigned long calibrationStepStart = 0;
bool calibrationStepOutputOn = false;
const int calibrationPins[] = {
  PIN_TURNL_OUT,
  PIN_TURNR_OUT,
  PIN_LIGHT_OUT,
  PIN_HIBEAM_OUT,
  PIN_BRAKE_OUT,
  PIN_AUX1_OUT
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline void outputOn(int pin) {
  digitalWrite(pin, HIGH);  // Outputs switch +12V
}

inline void outputOff(int pin) {
  digitalWrite(pin, LOW);
}

inline bool inputRawActive(int pin) {
  if (pin == PIN_LOCK) {
    return digitalRead(pin) == HIGH;  // LOCK connects to +12V
  }
  return digitalRead(pin) == LOW;  // All other inputs switch to ground
}

bool inputActive(int pin) {
  static bool initialized[49] = {false};
  static bool stableState[49] = {false};
  static bool lastReading[49] = {false};
  static unsigned long lastChangeTime[49] = {0};

  if (pin < 0 || pin > 48) {
    return inputRawActive(pin);
  }

  bool reading = inputRawActive(pin);
  if (!initialized[pin]) {
    initialized[pin] = true;
    stableState[pin] = reading;
    lastReading[pin] = reading;
    lastChangeTime[pin] = millis();
    return stableState[pin];
  }

  if (reading != lastReading[pin]) {
    lastReading[pin] = reading;
    lastChangeTime[pin] = millis();
  }

  if ((millis() - lastChangeTime[pin]) >= DEBOUNCE_DELAY && stableState[pin] != reading) {
    stableState[pin] = reading;
  }

  return stableState[pin];
}

void saveSettings() {
  preferences.begin(SETTINGS_NAMESPACE, false);
  preferences.putUChar("handlebar", settings.handlebarConfig);
  preferences.putUChar("rear", settings.rearLightMode);
  preferences.putUChar("turn", settings.turnSignalMode);
  preferences.putUChar("brake", settings.brakeLightMode);
  preferences.putUChar("alarm", settings.alarmMode);
  preferences.putUChar("pos", settings.positionLight);
  preferences.putBool("wave", settings.moWaveEnabled);
  preferences.putUChar("low", settings.lowBeamMode);
  preferences.putUChar("aux1", settings.aux1Mode);
  preferences.putUChar("aux2", settings.aux2Mode);
  preferences.putUChar("stand", settings.standKillMode);
  preferences.putUChar("park", settings.parkingLightMode);
  preferences.putUShort("tdist", settings.turnDistancePulsesTarget);
  preferences.end();
}

void loadSettings() {
  preferences.begin(SETTINGS_NAMESPACE, true);
  settings.handlebarConfig = static_cast<HandlebarConfig>(preferences.getUChar("handlebar", settings.handlebarConfig));
  settings.rearLightMode = preferences.getUChar("rear", settings.rearLightMode);
  settings.turnSignalMode = static_cast<TurnSignalMode>(preferences.getUChar("turn", settings.turnSignalMode));
  settings.brakeLightMode = static_cast<BrakeLightMode>(preferences.getUChar("brake", settings.brakeLightMode));
  settings.alarmMode = preferences.getUChar("alarm", settings.alarmMode);
  settings.positionLight = preferences.getUChar("pos", settings.positionLight);
  settings.moWaveEnabled = preferences.getBool("wave", settings.moWaveEnabled);
  settings.lowBeamMode = preferences.getUChar("low", settings.lowBeamMode);
  settings.aux1Mode = preferences.getUChar("aux1", settings.aux1Mode);
  settings.aux2Mode = preferences.getUChar("aux2", settings.aux2Mode);
  settings.standKillMode = preferences.getUChar("stand", settings.standKillMode);
  settings.parkingLightMode = preferences.getUChar("park", settings.parkingLightMode);
  settings.turnDistancePulsesTarget = preferences.getUShort("tdist", settings.turnDistancePulsesTarget);
  preferences.end();

  settings.handlebarConfig = static_cast<HandlebarConfig>(constrain(settings.handlebarConfig, CONFIG_A, CONFIG_E));
  settings.turnSignalMode = static_cast<TurnSignalMode>(constrain(settings.turnSignalMode, TURN_OFF, TURN_30S));
  settings.brakeLightMode = static_cast<BrakeLightMode>(constrain(settings.brakeLightMode, BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
  settings.positionLight = constrain(settings.positionLight, 0, 9);
  settings.turnDistancePulsesTarget = constrain(settings.turnDistancePulsesTarget, TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
}

void updateButtonEvent(int pin, ButtonEvent& event, unsigned long longPressMs = 0, unsigned long doubleClickMs = 400) {
  bool state = inputActive(pin);
  unsigned long now = millis();

  event.pressed = state && !event.state;
  event.released = !state && event.state;
  event.doubleClick = false;

  if (event.pressed) {
    if (now - event.lastReleaseTime <= doubleClickMs) {
      event.doubleClick = true;
    }
    event.pressStartTime = now;
    event.longPressLatched = false;
  }

  if (event.released) {
    event.lastReleaseTime = now;
  }

  event.longPress = false;
  if (state && longPressMs > 0 && !event.longPressLatched && now - event.pressStartTime >= longPressMs) {
    event.longPress = true;
    event.longPressLatched = true;
  }

  event.state = state;
}

void refreshInputEvents() {
  updateButtonEvent(PIN_LOCK, lockEvent);
  updateButtonEvent(PIN_TURNL, turnLeftEvent, 2000);
  updateButtonEvent(PIN_TURNR, turnRightEvent, 2000);
  updateButtonEvent(PIN_LIGHT, lightEvent, 2000);
  updateButtonEvent(PIN_START, startEvent, 0, 500);
  updateButtonEvent(PIN_HORN, hornEvent, 2000);
}

inline void resolveHazardState() {
  hazardLightsOn = emergencyHazardActive || manualHazardRequested;
}

void startCalibrationSequence() {
  calibrationState = CALIB_RUNNING;
  calibrationStepIndex = 0;
  calibrationStepStart = millis();
  calibrationStepOutputOn = true;
  Serial.println("Calibration started");
  outputOn(calibrationPins[calibrationStepIndex]);
}

void processCalibrationSequence() {
  if (calibrationState != CALIB_RUNNING) {
    return;
  }

  unsigned long now = millis();
  if (now - calibrationStepStart < CALIBRATION_STEP_DURATION) {
    return;
  }

  if (calibrationStepOutputOn) {
    outputOff(calibrationPins[calibrationStepIndex]);
    calibrationStepOutputOn = false;
    calibrationStepStart = now;
    return;
  }

  calibrationStepIndex++;
  if (calibrationStepIndex >= static_cast<int>(sizeof(calibrationPins) / sizeof(calibrationPins[0]))) {
    calibrationState = CALIB_DONE;
    Serial.println("Calibration completed");
    return;
  }

  outputOn(calibrationPins[calibrationStepIndex]);
  calibrationStepOutputOn = true;
  calibrationStepStart = now;
}

// Forward declarations
void calibrateOutputs();

// ============================================================================
// OUTPUT CONTROL FUNCTIONS
// ============================================================================

void updateTurnSignals() {
  if (hazardLightsOn) {
    // Hazard lights - both flash together
    if (millis() - lastFlasherToggle >= FLASHER_PERIOD) {
      flasherState = !flasherState;
      lastFlasherToggle = millis();
      outputOn(PIN_TURNL_OUT);
      outputOn(PIN_TURNR_OUT);
      if (!flasherState) {
        outputOff(PIN_TURNL_OUT);
        outputOff(PIN_TURNR_OUT);
      }
    }
  } else {
    // Normal turn signals
    if (leftTurnOn) {
      if (millis() - lastFlasherToggle >= FLASHER_PERIOD) {
        flasherState = !flasherState;
        lastFlasherToggle = millis();
      }
      outputOn(PIN_TURNL_OUT);
      if (!flasherState) {
        outputOff(PIN_TURNL_OUT);
      }
      outputOff(PIN_TURNR_OUT);
    } else if (rightTurnOn) {
      if (millis() - lastFlasherToggle >= FLASHER_PERIOD) {
        flasherState = !flasherState;
        lastFlasherToggle = millis();
      }
      outputOn(PIN_TURNR_OUT);
      if (!flasherState) {
        outputOff(PIN_TURNR_OUT);
      }
      outputOff(PIN_TURNL_OUT);
    } else {
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
    }
  }
}

void updateLights() {
  if (!ignitionOn) {
    outputOff(PIN_LIGHT_OUT);
    outputOff(PIN_HIBEAM_OUT);
    return;
  }

  // Low beam control
  if (lowBeamOn) {
    outputOn(PIN_LIGHT_OUT);
  } else {
    outputOff(PIN_LIGHT_OUT);
  }

  // High beam control
  if (highBeamOn) {
    outputOn(PIN_HIBEAM_OUT);
  } else {
    outputOff(PIN_HIBEAM_OUT);
  }
}

void updateBrakeLight() {
  if (!brakePressed) {
    outputOff(PIN_BRAKE_OUT);
    brakeFlashState = false;
    if (emergencyHazardActive) {
      emergencyHazardActive = false;
    }
    resolveHazardState();
    return;
  }

  switch (settings.brakeLightMode) {
    case BRAKE_CONTINUOUS:
      outputOn(PIN_BRAKE_OUT);
      break;

    case BRAKE_FADE:
      // Fade in/out with 3Hz (333ms period)
      if (millis() - lastBrakeFlash >= 333) {
        brakeFlashState = !brakeFlashState;
        lastBrakeFlash = millis();
      }
      if (brakeFlashState) {
        outputOn(PIN_BRAKE_OUT);
      } else {
        outputOff(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_FLASH_5HZ:
      // Flash with 5Hz (200ms period)
      if (millis() - lastBrakeFlash >= 200) {
        brakeFlashState = !brakeFlashState;
        lastBrakeFlash = millis();
      }
      if (brakeFlashState) {
        outputOn(PIN_BRAKE_OUT);
      } else {
        outputOff(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_FLASH_8X:
      // 8 flashes then continuous
      // Simplified - flash for 1.6s (8 * 200ms) then continuous
      if (millis() - brakePressTime < 1600) {
        if (millis() - lastBrakeFlash >= 200) {
          brakeFlashState = !brakeFlashState;
          lastBrakeFlash = millis();
        }
        if (brakeFlashState) {
          outputOn(PIN_BRAKE_OUT);
        } else {
          outputOff(PIN_BRAKE_OUT);
        }
      } else {
        outputOn(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_EMERGENCY:
      // Emergency braking - flash 5Hz + hazard lights
      emergencyHazardActive = true;
      resolveHazardState();
      if (millis() - lastBrakeFlash >= 200) {
        brakeFlashState = !brakeFlashState;
        lastBrakeFlash = millis();
      }
      if (brakeFlashState) {
        outputOn(PIN_BRAKE_OUT);
      } else {
        outputOff(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_FLASH_2X:
      // 2x flash then 1s continuous
      if (millis() - brakePressTime < 800) {
        if (millis() - lastBrakeFlash >= 200) {
          brakeFlashState = !brakeFlashState;
          lastBrakeFlash = millis();
        }
        if (brakeFlashState) {
          outputOn(PIN_BRAKE_OUT);
        } else {
          outputOff(PIN_BRAKE_OUT);
        }
      } else {
        outputOn(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_3S_FLASH:
      // 3s continuous then flashing
      if (millis() - brakePressTime < 3000) {
        outputOn(PIN_BRAKE_OUT);
      } else {
        if (millis() - lastBrakeFlash >= 200) {
          brakeFlashState = !brakeFlashState;
          lastBrakeFlash = millis();
        }
        if (brakeFlashState) {
          outputOn(PIN_BRAKE_OUT);
        } else {
          outputOff(PIN_BRAKE_OUT);
        }
      }
      break;

    default:
      outputOn(PIN_BRAKE_OUT);
      break;
  }
}

void updateHorn() {
  if (hornPressed && ignitionOn) {
    outputOn(PIN_HORN_OUT);
  } else {
    outputOff(PIN_HORN_OUT);
  }
}

void updateStarter() {
  if (startPressed && ignitionOn && !killActive && !engineRunning) {
    // Starter can run for max 5 seconds
    if (millis() - startPressTime < 5000) {
      outputOn(PIN_START_OUT1);
      outputOn(PIN_START_OUT2);
    } else {
      outputOff(PIN_START_OUT1);
      outputOff(PIN_START_OUT2);
    }
  } else {
    outputOff(PIN_START_OUT1);
    outputOff(PIN_START_OUT2);
  }
}

void updateIgnition() {
  if (ignitionOn && !killActive) {
    outputOn(PIN_IGN_OUT);
  } else {
    outputOff(PIN_IGN_OUT);
  }
}

void updateAuxOutputs() {
  // AUX1 - simplified: active with ignition
  if (settings.aux1Mode == 0 && ignitionOn) {
    outputOn(PIN_AUX1_OUT);
  } else {
    outputOff(PIN_AUX1_OUT);
  }

  // AUX2 - simplified: active with ignition
  if (settings.aux2Mode == 0 && ignitionOn) {
    outputOn(PIN_AUX2_OUT);
  } else {
    outputOff(PIN_AUX2_OUT);
  }
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void handleLock() {
  if (lockEvent.pressed) {
    ignitionOn = true;
    Serial.println("Ignition ON");
  } else if (lockEvent.released) {
    ignitionOn = false;
    engineRunning = false;
    manualHazardRequested = false;
    Serial.println("Ignition OFF");
  }
}

void handleTurnSignals() {
  bool leftState = turnLeftEvent.state;
  bool rightState = turnRightEvent.state;

  // Detect simultaneous long-press for hazard lights (one-shot per press cycle)
  if (leftState && rightState && (turnLeftEvent.longPress || turnRightEvent.longPress)) {
    if (!manualHazardRequested && !emergencyHazardActive) {
      manualHazardRequested = true;
      leftTurnOn = false;
      rightTurnOn = false;
      Serial.println("Hazard lights ON");
    } else if (manualHazardRequested && !emergencyHazardActive) {
      manualHazardRequested = false;
      Serial.println("Hazard lights OFF");
    }
  }

  // Left turn signal
  if (turnLeftEvent.pressed && !rightState && !hazardLightsOn) {
    leftTurnOn = !leftTurnOn;  // Toggle
    rightTurnOn = false;
    leftTurnStartTime = millis();
    leftTurnStartPulses = speedPulseCount;
    Serial.println("Left turn toggled");
  }

  // Right turn signal
  if (turnRightEvent.pressed && !leftState && !hazardLightsOn) {
    rightTurnOn = !rightTurnOn;  // Toggle
    leftTurnOn = false;
    rightTurnStartTime = millis();
    rightTurnStartPulses = speedPulseCount;
    Serial.println("Right turn toggled");
  }

  // Auto-turn-off based on time/distance
  if (leftTurnOn && settings.turnSignalMode != TURN_OFF) {
    unsigned long duration = millis() - leftTurnStartTime;
    unsigned long timeout = 10000;  // Default 10s
    if (settings.turnSignalMode == TURN_DISTANCE) timeout = TURN_DISTANCE_TIMEOUT;
    if (settings.turnSignalMode == TURN_20S) timeout = 20000;
    if (settings.turnSignalMode == TURN_30S) timeout = 30000;

    bool distanceReached = settings.turnSignalMode == TURN_DISTANCE &&
                           (speedPulseCount - leftTurnStartPulses >= settings.turnDistancePulsesTarget);
    if (duration >= timeout || distanceReached) {
      leftTurnOn = false;
      Serial.println("Left turn auto-off");
    }
  }

  if (rightTurnOn && settings.turnSignalMode != TURN_OFF) {
    unsigned long duration = millis() - rightTurnStartTime;
    unsigned long timeout = 10000;  // Default 10s
    if (settings.turnSignalMode == TURN_DISTANCE) timeout = TURN_DISTANCE_TIMEOUT;
    if (settings.turnSignalMode == TURN_20S) timeout = 20000;
    if (settings.turnSignalMode == TURN_30S) timeout = 30000;

    bool distanceReached = settings.turnSignalMode == TURN_DISTANCE &&
                           (speedPulseCount - rightTurnStartPulses >= settings.turnDistancePulsesTarget);
    if (duration >= timeout || distanceReached) {
      rightTurnOn = false;
      Serial.println("Right turn auto-off");
    }
  }
}

void handleSpeedSensor() {
  static bool lastSpeedState = false;
  bool speedState = inputActive(PIN_SPEED);

  if (speedState && !lastSpeedState) {
    speedPulseCount++;
  }
  lastSpeedState = speedState;
}

void handleLight() {
  if (lightEvent.released) {
    unsigned long pressDuration = millis() - lightEvent.pressStartTime;

    if (pressDuration < 500) {
      // Brief press: toggle high/low beam or flash high beam
      if (lowBeamOn) {
        highBeamOn = !highBeamOn;
      } else {
        lowBeamOn = true;
      }
      Serial.println("Light toggle");
    } else if (pressDuration >= 2000) {
      // Long press (2s): light off
      lowBeamOn = false;
      highBeamOn = false;
      Serial.println("Lights OFF");
    }
  }
}

void handleStart() {
  if (startEvent.pressed) {
    startPressed = true;
    startPressTime = millis();

    if (startEvent.doubleClick) {
      // Engine kill
      engineRunning = false;
      killActive = true;
      Serial.println("Engine KILL");
      return;
    }

    if (ignitionOn && !killActive) {
      // Start engine
      engineRunning = true;
      lowBeamOn = true;  // Low beam on after engine start (default)
      killActive = false;
      Serial.println("Engine START");
    }
  }

  if (startEvent.released) {
    startPressed = false;
  }
}

void handleHorn() {
  hornPressed = hornEvent.state;
}

void handleBrake() {
  bool brakeState = inputActive(PIN_BRAKE);
  
  if (brakeState && !brakePressed) {
    brakePressTime = millis();
    lastBrakeFlash = millis();
  }
  
  brakePressed = brakeState;
}

void handleKill() {
  bool killState = inputActive(PIN_KILL);
  
  if (killState && settings.standKillMode % 3 == 0) {
    // KILL N/O (normally open)
    killActive = killState;
  } else if (!killState && settings.standKillMode % 3 == 1) {
    // KILL N/C (normally closed)
    killActive = !killState;
  }
  
  if (killActive) {
    engineRunning = false;
  }
}

void applySafetyPriorities() {
  // Priority 1: kill switch always stops engine and starter request
  if (killActive) {
    engineRunning = false;
    startPressed = false;
  }

  // Priority 2: without ignition, disable non-essential dynamic outputs
  if (!ignitionOn) {
    leftTurnOn = false;
    rightTurnOn = false;
    manualHazardRequested = false;
  }

  resolveHazardState();
}

// ============================================================================
// SETUP MODE
// ============================================================================

void enterSetupMode() {
  if (hornEvent.state && !ignitionOn) {
    // Hold horn while switching ignition on
    // This is detected in setup()
    inSetupMode = true;
    setupEnterTime = millis();
    Serial.println("Entering SETUP mode");
  }
}

void exitSetupMode() {
  if (inSetupMode) {
    inSetupMode = false;
    Serial.println("Exiting SETUP mode - calibration starting");
    startCalibrationSequence();
  }
}

void calibrateOutputs() {
  // Kept for compatibility: starts the non-blocking calibration sequence.
  startCalibrationSequence();
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("mo.unit emulator starting...");
  loadSettings();

  // Configure input pins
  pinMode(PIN_LOCK, INPUT_PULLDOWN);
  pinMode(PIN_TURNL, INPUT_PULLUP);
  pinMode(PIN_TURNR, INPUT_PULLUP);
  pinMode(PIN_LIGHT, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_HORN, INPUT_PULLUP);
  pinMode(PIN_BRAKE, INPUT_PULLUP);
  pinMode(PIN_KILL, INPUT_PULLUP);
  pinMode(PIN_STAND, INPUT_PULLUP);
  pinMode(PIN_AUX1, INPUT_PULLUP);
  pinMode(PIN_AUX2, INPUT_PULLUP);
  pinMode(PIN_SPEED, INPUT_PULLUP);

  // Configure output pins
  pinMode(PIN_TURNL_OUT, OUTPUT);
  pinMode(PIN_TURNR_OUT, OUTPUT);
  pinMode(PIN_LIGHT_OUT, OUTPUT);
  pinMode(PIN_HIBEAM_OUT, OUTPUT);
  pinMode(PIN_BRAKE_OUT, OUTPUT);
  pinMode(PIN_HORN_OUT, OUTPUT);
  pinMode(PIN_START_OUT1, OUTPUT);
  pinMode(PIN_START_OUT2, OUTPUT);
  pinMode(PIN_IGN_OUT, OUTPUT);
  pinMode(PIN_AUX1_OUT, OUTPUT);
  pinMode(PIN_AUX2_OUT, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  // Initialize all outputs to OFF
  outputOff(PIN_TURNL_OUT);
  outputOff(PIN_TURNR_OUT);
  outputOff(PIN_LIGHT_OUT);
  outputOff(PIN_HIBEAM_OUT);
  outputOff(PIN_BRAKE_OUT);
  outputOff(PIN_HORN_OUT);
  outputOff(PIN_START_OUT1);
  outputOff(PIN_START_OUT2);
  outputOff(PIN_IGN_OUT);
  outputOff(PIN_AUX1_OUT);
  outputOff(PIN_AUX2_OUT);
  digitalWrite(LED_STATUS, LOW);

  // Cold start initialization - LED test sequence
  Serial.println("Cold start - LED test");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(SETUP_FLASH_INTERVAL);
    digitalWrite(LED_STATUS, LOW);
    delay(SETUP_FLASH_INTERVAL);
  }

  // Check for setup mode entry
  refreshInputEvents();
  if (hornEvent.state) {
    enterSetupMode();
  }

  Serial.println("mo.unit ready.");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  refreshInputEvents();
  processCalibrationSequence();

  if (calibrationState == CALIB_DONE) {
    calibrationState = CALIB_IDLE;
    saveSettings();
  }

  // Handle setup mode exit
  if (inSetupMode && hornEvent.state && millis() - setupEnterTime > 2000) {
    // Hold horn for 2s to exit setup
    exitSetupMode();
    return;
  }

  if (!inSetupMode) {
    // Read inputs
    handleLock();
    handleTurnSignals();
    handleLight();
    handleStart();
    handleHorn();
    handleBrake();
    handleKill();
    handleSpeedSensor();
    applySafetyPriorities();

    // Update outputs
    updateIgnition();
    updateTurnSignals();
    updateLights();
    updateBrakeLight();
    updateHorn();
    updateStarter();
    updateAuxOutputs();

    // Status LED blinks when ignition is on
    if (ignitionOn && (millis() % 1000 < 100)) {
      digitalWrite(LED_STATUS, HIGH);
    } else {
      digitalWrite(LED_STATUS, LOW);
    }
  } else {
    // Setup mode - simplified, just wait for exit
    if (hornEvent.state && millis() - setupEnterTime > 2000) {
      exitSetupMode();
    }
  }

  delay(10);  // Small delay to prevent excessive CPU usage
}
