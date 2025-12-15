#include <Arduino.h>

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
#define LED_STATUS     2    // Built-in LED or status indicator

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
  .parkingLightMode = 0
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
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline void outputOn(int pin) {
  digitalWrite(pin, HIGH);  // Outputs switch +12V
}

inline void outputOff(int pin) {
  digitalWrite(pin, LOW);
}

inline bool inputActive(int pin) {
  if (pin == PIN_LOCK) {
    return digitalRead(pin) == HIGH;  // LOCK connects to +12V
  }
  return digitalRead(pin) == LOW;  // All other inputs switch to ground
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
      hazardLightsOn = true;
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
  static bool lastLockState = false;
  bool lockState = inputActive(PIN_LOCK);

  if (lockState != lastLockState) {
    lastLockState = lockState;
    if (lockState) {
      // Cold start: initialize system
      ignitionOn = true;
      Serial.println("Ignition ON");
    } else {
      ignitionOn = false;
      engineRunning = false;
      Serial.println("Ignition OFF");
    }
  }
}

void handleTurnSignals() {
  static bool lastLeftState = false;
  static bool lastRightState = false;
  static unsigned long leftPressTime = 0;
  static unsigned long rightPressTime = 0;
  static unsigned long hazardStartTime = 0;

  bool leftState = inputActive(PIN_TURNL);
  bool rightState = inputActive(PIN_TURNR);

  // Detect simultaneous press for hazard lights
  if (leftState && rightState && !hazardLightsOn) {
    unsigned long pressDuration = millis() - (leftPressTime > rightPressTime ? leftPressTime : rightPressTime);
    if (pressDuration >= 2000) {  // Hold for 2 seconds
      hazardLightsOn = true;
      leftTurnOn = false;
      rightTurnOn = false;
      Serial.println("Hazard lights ON");
    }
  } else if (!leftState && !rightState && hazardLightsOn) {
    // Check if hazard should turn off
    // In real implementation, this would need more logic
  }

  // Left turn signal
  if (leftState && !rightState && !hazardLightsOn) {
    if (!lastLeftState) {
      leftPressTime = millis();
      leftTurnOn = !leftTurnOn;  // Toggle
      rightTurnOn = false;
      leftTurnStartTime = millis();
      Serial.println("Left turn toggled");
    }
  }
  lastLeftState = leftState;

  // Right turn signal
  if (rightState && !leftState && !hazardLightsOn) {
    if (!lastRightState) {
      rightPressTime = millis();
      rightTurnOn = !rightTurnOn;  // Toggle
      leftTurnOn = false;
      rightTurnStartTime = millis();
      Serial.println("Right turn toggled");
    }
  }
  lastRightState = rightState;

  // Auto-turn-off based on time/distance
  if (leftTurnOn && settings.turnSignalMode != TURN_OFF) {
    unsigned long duration = millis() - leftTurnStartTime;
    unsigned long timeout = 10000;  // Default 10s
    if (settings.turnSignalMode == TURN_20S) timeout = 20000;
    if (settings.turnSignalMode == TURN_30S) timeout = 30000;
    
    if (duration >= timeout) {
      leftTurnOn = false;
      Serial.println("Left turn auto-off");
    }
  }

  if (rightTurnOn && settings.turnSignalMode != TURN_OFF) {
    unsigned long duration = millis() - rightTurnStartTime;
    unsigned long timeout = 10000;  // Default 10s
    if (settings.turnSignalMode == TURN_20S) timeout = 20000;
    if (settings.turnSignalMode == TURN_30S) timeout = 30000;
    
    if (duration >= timeout) {
      rightTurnOn = false;
      Serial.println("Right turn auto-off");
    }
  }
}

void handleLight() {
  static bool lastLightState = false;
  static unsigned long lightPressTime = 0;

  bool lightState = inputActive(PIN_LIGHT);

  if (lightState != lastLightState) {
    lastLightState = lightState;
    if (lightState) {
      lightPressTime = millis();
    } else {
      unsigned long pressDuration = millis() - lightPressTime;
      
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
}

void handleStart() {
  static bool lastStartState = false;
  static unsigned long lastStartPress = 0;
  static int doubleClickCount = 0;

  bool startState = inputActive(PIN_START);

  if (startState && !lastStartState) {
    unsigned long timeSinceLastPress = millis() - lastStartPress;
    
    if (timeSinceLastPress < 500) {
      // Double click detected
      doubleClickCount++;
      if (doubleClickCount == 2) {
        // Engine kill
        engineRunning = false;
        killActive = true;
        Serial.println("Engine KILL");
        doubleClickCount = 0;
      }
    } else {
      doubleClickCount = 1;
    }
    
    lastStartPress = millis();
    startPressed = true;
    startPressTime = millis();
    
    if (ignitionOn && !killActive) {
      // Start engine
      engineRunning = true;
      lowBeamOn = true;  // Low beam on after engine start (default)
      killActive = false;
      Serial.println("Engine START");
    }
  }

  if (!startState) {
    startPressed = false;
  }

  lastStartState = startState;
}

void handleHorn() {
  static bool lastHornState = false;
  static unsigned long hornHoldStart = 0;

  bool hornState = inputActive(PIN_HORN);
  
  if (hornState && !lastHornState) {
    hornHoldStart = millis();
  }

  // Setup mode entry: hold horn while switching ignition on
  if (hornState && !ignitionOn) {
    // Will be handled in setup entry check
  }

  hornPressed = hornState;
  lastHornState = hornState;
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

// ============================================================================
// SETUP MODE
// ============================================================================

void enterSetupMode() {
  if (inputActive(PIN_HORN) && !ignitionOn) {
    // Hold horn while switching ignition on
    // This is detected in setup()
    inSetupMode = true;
    setupEnterTime = millis();
    Serial.println("Entering SETUP mode");
    // Flash indicators briefly
    for (int i = 0; i < 3; i++) {
      outputOn(PIN_TURNL_OUT);
      outputOn(PIN_TURNR_OUT);
      delay(200);
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
      delay(200);
    }
  }
}

void exitSetupMode() {
  if (inSetupMode) {
    inSetupMode = false;
    Serial.println("Exiting SETUP mode - calibration starting");
    // Calibration sequence
    calibrateOutputs();
  }
}

void calibrateOutputs() {
  Serial.println("Calibration started");
  // Calibrate each output by switching on briefly
  outputOn(PIN_TURNL_OUT);
  delay(500);
  outputOff(PIN_TURNL_OUT);
  
  outputOn(PIN_TURNR_OUT);
  delay(500);
  outputOff(PIN_TURNR_OUT);
  
  outputOn(PIN_LIGHT_OUT);
  delay(500);
  outputOff(PIN_LIGHT_OUT);
  
  outputOn(PIN_HIBEAM_OUT);
  delay(500);
  outputOff(PIN_HIBEAM_OUT);
  
  outputOn(PIN_BRAKE_OUT);
  delay(500);
  outputOff(PIN_BRAKE_OUT);
  
  outputOn(PIN_AUX1_OUT);
  delay(500);
  outputOff(PIN_AUX1_OUT);
  
  Serial.println("Calibration completed");
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("mo.unit emulator starting...");

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
    delay(200);
    digitalWrite(LED_STATUS, LOW);
    delay(200);
  }

  // Check for setup mode entry
  if (inputActive(PIN_HORN)) {
    enterSetupMode();
  }

  Serial.println("mo.unit ready.");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle setup mode exit
  if (inSetupMode && inputActive(PIN_HORN) && millis() - setupEnterTime > 2000) {
    // Hold horn for 2s to exit setup
    exitSetupMode();
    delay(500);
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
    if (inputActive(PIN_HORN) && millis() - setupEnterTime > 2000) {
      exitSetupMode();
    }
  }

  delay(10);  // Small delay to prevent excessive CPU usage
}
