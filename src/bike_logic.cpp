#include "bike_logic.h"
#include "outputs.h"
#include "inputs.h"
#include "safety.h"

// ============================================================================
// INPUT HANDLERS
// ============================================================================

void handleLock() {
  if (lockEvent.pressed) {
    bike.ignitionOn = true;
    LOG_I("Ignition ON");
  } else if (lockEvent.released) {
    bike.ignitionOn = false;
    bike.engineRunning = false;
    bike.starterEngaged = false;
    bike.starterLightSnapshotValid = false;
    bike.manualHazardRequested = false;
    bike.lowBeamOn = false;
    bike.highBeamOn = false;
    LOG_I("Ignition OFF");
  }
}

// --------------------------------------------------------------------------

void handleTurnSignals() {
  bool leftState  = turnLeftEvent.state;
  bool rightState = turnRightEvent.state;

  // Simultaneous long-press → hazard toggle
  if (leftState && rightState
      && (turnLeftEvent.longPress || turnRightEvent.longPress)) {
    if (!bike.manualHazardRequested && !bike.emergencyHazardActive) {
      bike.manualHazardRequested = true;
      bike.leftTurnOn  = false;
      bike.rightTurnOn = false;
      LOG_I("Hazard lights ON");
    } else if (bike.manualHazardRequested && !bike.emergencyHazardActive) {
      bike.manualHazardRequested = false;
      LOG_I("Hazard lights OFF");
    }
  }

  // Left turn toggle
  if (turnLeftEvent.pressed && !rightState && !bike.hazardLightsOn) {
    bike.leftTurnOn  = !bike.leftTurnOn;
    bike.rightTurnOn = false;
    bike.leftTurnStartTime   = millis();
    bike.leftTurnStartPulses = bike.speedPulseCount;
    // FIX #5: Reset flasher on mode change for clean phase
    bike.flasherState       = false;
    bike.lastFlasherToggle  = millis();
    LOG_D("Left turn toggled %s", bike.leftTurnOn ? "ON" : "OFF");
  }

  // Right turn toggle
  if (turnRightEvent.pressed && !leftState && !bike.hazardLightsOn) {
    bike.rightTurnOn = !bike.rightTurnOn;
    bike.leftTurnOn  = false;
    bike.rightTurnStartTime   = millis();
    bike.rightTurnStartPulses = bike.speedPulseCount;
    bike.flasherState       = false;
    bike.lastFlasherToggle  = millis();
    LOG_D("Right turn toggled %s", bike.rightTurnOn ? "ON" : "OFF");
  }

  // Auto-turn-off based on time / distance
  if (settings.turnSignalMode != TURN_OFF) {
    unsigned long timeout = TURN_10S_TIMEOUT_MS;
    switch (settings.turnSignalMode) {
      case TURN_DISTANCE: timeout = TURN_DISTANCE_TIMEOUT_MS; break;
      case TURN_20S:      timeout = TURN_20S_TIMEOUT_MS;      break;
      case TURN_30S:      timeout = TURN_30S_TIMEOUT_MS;      break;
      default: break;
    }

    if (bike.leftTurnOn) {
      unsigned long dur = millis() - bike.leftTurnStartTime;
      bool distReached = (settings.turnSignalMode == TURN_DISTANCE)
        && (bike.speedPulseCount - bike.leftTurnStartPulses
            >= settings.turnDistancePulsesTarget);
      if (dur >= timeout || distReached) {
        bike.leftTurnOn = false;
        LOG_D("Left turn auto-off");
      }
    }

    if (bike.rightTurnOn) {
      unsigned long dur = millis() - bike.rightTurnStartTime;
      bool distReached = (settings.turnSignalMode == TURN_DISTANCE)
        && (bike.speedPulseCount - bike.rightTurnStartPulses
            >= settings.turnDistancePulsesTarget);
      if (dur >= timeout || distReached) {
        bike.rightTurnOn = false;
        LOG_D("Right turn auto-off");
      }
    }
  }
}

// --------------------------------------------------------------------------

void handleLight() {
  if (lightEvent.released) {
    unsigned long dur = millis() - lightEvent.pressStartTime;

    if (dur < LIGHT_SHORT_PRESS_MS) {
      if (bike.lowBeamOn) {
        bike.highBeamOn = !bike.highBeamOn;
      } else {
        bike.lowBeamOn = true;
      }
      LOG_D("Light toggle: low=%d high=%d", bike.lowBeamOn, bike.highBeamOn);
    } else if (dur >= LIGHT_LONG_PRESS_MS) {
      bike.lowBeamOn  = false;
      bike.highBeamOn = false;
      LOG_D("Lights OFF");
    }
  }
}

// --------------------------------------------------------------------------
// FIX #3: Starter logic completely reworked
// The starter now runs while button is held, and engineRunning is only set
// after the starter has been engaged for STARTER_ENGAGE_DELAY_MS.
// --------------------------------------------------------------------------

void handleStart() {
  if (startEvent.pressed) {
    // Double-click = engine kill (FIX #9: clear killActive properly)
    if (startEvent.doubleClick) {
      bike.engineRunning  = false;
      bike.starterEngaged = false;
      bike.killActive     = true;
      LOG_I("Engine KILL (double-click)");
      return;
    }

    // Single press: begin starter engagement
    if (bike.ignitionOn && !bike.killActive && !bike.engineRunning) {
      bike.starterLowBeamBeforeStart = bike.lowBeamOn;
      bike.starterHighBeamBeforeStart = bike.highBeamOn;
      bike.starterLightSnapshotValid = true;
      bike.starterEngaged  = true;
      bike.starterStartTime = millis();
      LOG_I("Starter ENGAGED");
    }
  }

  // While button is held, keep starter running
  if (startEvent.state && bike.starterEngaged) {
    unsigned long elapsed = millis() - bike.starterStartTime;

    // After engage delay, consider engine running
    if (elapsed >= STARTER_ENGAGE_DELAY_MS && !bike.engineRunning) {
      bike.engineRunning = true;
      bike.killActive = false;
      LOG_I("Engine RUNNING");
    }

    // Safety timeout: max starter duration
    if (elapsed >= STARTER_MAX_DURATION_MS) {
      bike.starterEngaged = false;
      bike.errorFlags |= ERR_STARTER_TIMEOUT;
      LOG_W("Starter timeout – disengaged");
    }
  }

  // Button released: stop starter
  if (startEvent.released) {
    if (bike.starterEngaged) {
      bike.starterEngaged = false;
      LOG_D("Starter released");
    }
  }
}

void handleStarterLightSuppression() {
  static bool prevStarterEngaged = false;

  if (bike.starterEngaged && !prevStarterEngaged) {
    bike.starterLowBeamBeforeStart = bike.lowBeamOn;
    bike.starterHighBeamBeforeStart = bike.highBeamOn;
    bike.starterLightSnapshotValid = true;
  }

  if (bike.starterEngaged) {
    bike.lowBeamOn = false;
    bike.highBeamOn = false;
  } else if (prevStarterEngaged && bike.starterLightSnapshotValid) {
    bike.lowBeamOn = bike.starterLowBeamBeforeStart;
    bike.highBeamOn = bike.starterHighBeamBeforeStart;
    bike.starterLightSnapshotValid = false;
  }

  prevStarterEngaged = bike.starterEngaged;
}

// --------------------------------------------------------------------------

void handleHorn() {
  bike.hornPressed = hornEvent.state;
}

// --------------------------------------------------------------------------

void handleBrake() {
  bool brakeState = inputActive(PIN_BRAKE);

  if (brakeState && !bike.brakePressed) {
    bike.brakePressTime = millis();
    bike.lastBrakeFlash = millis();
  }

  bike.brakePressed = brakeState;
}

// ============================================================================
// OUTPUT UPDATERS
// ============================================================================

void updateIgnition() {
  if (bike.ignitionOn && !bike.killActive) {
    outputOn(PIN_IGN_OUT);
  } else {
    outputOff(PIN_IGN_OUT);
  }
}

// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// mo.wave sequential turn signal animation
// Creates a sweep effect: segments light up sequentially (1→2→3→all),
// then all off, then repeat. Gives a modern "running light" look.
// --------------------------------------------------------------------------

static void updateWaveFlasher(int pin, bool isActive, unsigned long now) {
  if (!isActive) {
    outputOff(pin);
    return;
  }

  // Total cycle: MOWAVE_STEPS × MOWAVE_STEP_MS (on-sweep) + MOWAVE_OFF_MS (off)
  unsigned long totalOn  = (unsigned long)MOWAVE_STEPS * MOWAVE_STEP_MS;
  unsigned long totalCycle = totalOn + MOWAVE_OFF_MS;
  unsigned long phase = (now - bike.waveStepStart) % totalCycle;

  if (phase < totalOn) {
    // During on-sweep: PWM brightness ramps up in steps
    uint8_t step = phase / MOWAVE_STEP_MS;  // 0, 1, 2
    // Brightness: step 0 = 33%, step 1 = 66%, step 2 = 100%
    uint8_t duty = (uint8_t)(((step + 1) * 255) / MOWAVE_STEPS);
    outputPWM(pin, duty);
  } else {
    // Off phase
    outputOff(pin);
  }
}

void updateTurnSignals() {
  unsigned long now = millis();

  if (bike.starterEngaged) {
    outputOff(PIN_TURNL_OUT);
    outputOff(PIN_TURNR_OUT);
    return;
  }

  // BLE connect acknowledgement: blink left/right once quickly.
  if (bike.bleConnectBlinkActive) {
    unsigned long elapsed = now - bike.bleConnectBlinkStart;
    if (elapsed < 120) {
      outputOn(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
    } else if (elapsed < 180) {
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
    } else if (elapsed < 300) {
      outputOff(PIN_TURNL_OUT);
      outputOn(PIN_TURNR_OUT);
    } else if (elapsed < 360) {
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
    } else {
      bike.bleConnectBlinkActive = false;
    }
    return;
  }

  // Standard flasher logic for non-wave mode
  bool useWave = settings.moWaveEnabled && !bike.hazardLightsOn;

  if (bike.hazardLightsOn) {
    // Hazard: both flash together (always standard flash, never wave)
    if (now - bike.lastFlasherToggle >= FLASHER_PERIOD_MS) {
      bike.flasherState = !bike.flasherState;
      bike.lastFlasherToggle = now;
    }
    if (bike.flasherState) {
      outputOn(PIN_TURNL_OUT);
      outputOn(PIN_TURNR_OUT);
    } else {
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
    }
  } else if (bike.leftTurnOn) {
    if (useWave) {
      updateWaveFlasher(PIN_TURNL_OUT, true, now);
    } else {
      if (now - bike.lastFlasherToggle >= FLASHER_PERIOD_MS) {
        bike.flasherState = !bike.flasherState;
        bike.lastFlasherToggle = now;
      }
      if (bike.flasherState) outputOn(PIN_TURNL_OUT);
      else                   outputOff(PIN_TURNL_OUT);
    }
    outputOff(PIN_TURNR_OUT);
  } else if (bike.rightTurnOn) {
    if (useWave) {
      updateWaveFlasher(PIN_TURNR_OUT, true, now);
    } else {
      if (now - bike.lastFlasherToggle >= FLASHER_PERIOD_MS) {
        bike.flasherState = !bike.flasherState;
        bike.lastFlasherToggle = now;
      }
      if (bike.flasherState) outputOn(PIN_TURNR_OUT);
      else                   outputOff(PIN_TURNR_OUT);
    }
    outputOff(PIN_TURNL_OUT);
  } else {
    outputOff(PIN_TURNL_OUT);
    outputOff(PIN_TURNR_OUT);
  }
}

// --------------------------------------------------------------------------

void updateLights() {
  if (!bike.ignitionOn) {
    // Parking light is handled separately by updateParkingLight()
    // Only turn off if parking light is not active
    if (settings.parkingLightMode == 0 || bike.alarmTriggered) {
      outputOff(PIN_LIGHT_OUT);
    }
    outputOff(PIN_HIBEAM_OUT);
    return;
  }

  if (bike.starterEngaged) {
    outputOff(PIN_LIGHT_OUT);
    outputOff(PIN_HIBEAM_OUT);
    return;
  }

  // Low beam mode:
  //   0 = standard state-driven behavior
  //   1 = always on with ignition
  //   2 = manual only (user toggles)
  if (settings.lowBeamMode == 1 && !bike.lowBeamOn) {
    bike.lowBeamOn = true;
  }

  // Position light dimming (PWM)
  if (!bike.lowBeamOn && settings.positionLight > 0) {
    uint8_t duty = map(settings.positionLight, 1, 9, 13, 128);  // ~5-50%
    outputPWM(PIN_LIGHT_OUT, duty);
  } else if (bike.lowBeamOn) {
    outputOn(PIN_LIGHT_OUT);
  } else {
    outputOff(PIN_LIGHT_OUT);
  }

  if (bike.highBeamOn) outputOn(PIN_HIBEAM_OUT);
  else                 outputOff(PIN_HIBEAM_OUT);
}

// --------------------------------------------------------------------------
// FIX #7: Real PWM fade for BRAKE_FADE mode
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Rear light modes (rearLightMode):
//   0 = Standard (brake only)
//   1 = Dimmed tail light (PWM ~30%) + full brightness on brake
//   2 = Always on (full) + still brightens on brake (handled by wiring)
// --------------------------------------------------------------------------

void updateBrakeLight() {
  if (bike.starterEngaged) {
    outputOff(PIN_BRAKE_OUT);
    return;
  }

  if (!bike.brakePressed) {
    // When not braking, check rear light mode for tail light behavior
    if (bike.ignitionOn && settings.rearLightMode == 1) {
      // Dimmed tail light (~30% brightness)
      outputPWM(PIN_BRAKE_OUT, 77);  // ~30% of 255
    } else if (bike.ignitionOn && settings.rearLightMode == 2) {
      // Always on at full brightness
      outputOn(PIN_BRAKE_OUT);
    } else {
      outputOff(PIN_BRAKE_OUT);
    }
    bike.brakeFlashState = false;
    if (bike.emergencyHazardActive) {
      bike.emergencyHazardActive = false;
      resolveHazardState();
    }
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - bike.brakePressTime;

  switch (settings.brakeLightMode) {

    case BRAKE_CONTINUOUS:
      outputOn(PIN_BRAKE_OUT);
      break;

    // FIX #7: Real PWM fade using sine wave
    case BRAKE_FADE: {
      // 3Hz sine wave fade: period = 333ms
      float phase = (float)(now % BRAKE_FADE_PERIOD_MS) / BRAKE_FADE_PERIOD_MS;
      float sine = (sin(phase * 2.0f * PI) + 1.0f) / 2.0f;  // 0.0 – 1.0
      uint8_t duty = 80 + (uint8_t)(sine * 175);  // 80-255 (never fully off)
      outputPWM(PIN_BRAKE_OUT, duty);
      break;
    }

    case BRAKE_FLASH_5HZ:
      if (now - bike.lastBrakeFlash >= BRAKE_FLASH_PERIOD_MS) {
        bike.brakeFlashState = !bike.brakeFlashState;
        bike.lastBrakeFlash = now;
      }
      if (bike.brakeFlashState) outputOn(PIN_BRAKE_OUT);
      else                      outputOff(PIN_BRAKE_OUT);
      break;

    case BRAKE_FLASH_8X:
      if (elapsed < BRAKE_8X_DURATION_MS) {
        if (now - bike.lastBrakeFlash >= BRAKE_FLASH_PERIOD_MS) {
          bike.brakeFlashState = !bike.brakeFlashState;
          bike.lastBrakeFlash = now;
        }
        if (bike.brakeFlashState) outputOn(PIN_BRAKE_OUT);
        else                      outputOff(PIN_BRAKE_OUT);
      } else {
        outputOn(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_FLASH_2X:
      if (elapsed < BRAKE_2X_DURATION_MS) {
        if (now - bike.lastBrakeFlash >= BRAKE_FLASH_PERIOD_MS) {
          bike.brakeFlashState = !bike.brakeFlashState;
          bike.lastBrakeFlash = now;
        }
        if (bike.brakeFlashState) outputOn(PIN_BRAKE_OUT);
        else                      outputOff(PIN_BRAKE_OUT);
      } else {
        outputOn(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_3S_FLASH:
      if (elapsed < BRAKE_CONTINUOUS_DELAY_MS) {
        outputOn(PIN_BRAKE_OUT);
      } else {
        if (now - bike.lastBrakeFlash >= BRAKE_FLASH_PERIOD_MS) {
          bike.brakeFlashState = !bike.brakeFlashState;
          bike.lastBrakeFlash = now;
        }
        if (bike.brakeFlashState) outputOn(PIN_BRAKE_OUT);
        else                      outputOff(PIN_BRAKE_OUT);
      }
      break;

    case BRAKE_EMERGENCY:
      bike.emergencyHazardActive = true;
      resolveHazardState();
      if (now - bike.lastBrakeFlash >= BRAKE_FLASH_PERIOD_MS) {
        bike.brakeFlashState = !bike.brakeFlashState;
        bike.lastBrakeFlash = now;
      }
      if (bike.brakeFlashState) outputOn(PIN_BRAKE_OUT);
      else                      outputOff(PIN_BRAKE_OUT);
      break;

    default:
      outputOn(PIN_BRAKE_OUT);
      break;
  }
}

// --------------------------------------------------------------------------

void updateHorn() {
  if (bike.hornPressed && bike.ignitionOn) {
    outputOn(PIN_HORN_OUT);
  } else {
    outputOff(PIN_HORN_OUT);
  }
}

// --------------------------------------------------------------------------
// FIX #3: Starter only runs while starterEngaged is true
// --------------------------------------------------------------------------

void updateStarter() {
  if (bike.starterEngaged && bike.ignitionOn && !bike.killActive) {
    outputOn(PIN_START_OUT1);
    outputOn(PIN_START_OUT2);
  } else {
    outputOff(PIN_START_OUT1);
    outputOff(PIN_START_OUT2);
  }
}

// --------------------------------------------------------------------------
// AUX Output Modes:
//   0 = ON with ignition (default)
//   1 = ON with engine running
//   2 = Manual toggle (on/off via web UI)
//   3 = Disabled (always off)
// --------------------------------------------------------------------------

static bool auxShouldBeOn(uint8_t mode, bool manualOn) {
  switch (mode) {
    case 0:  return bike.ignitionOn;
    case 1:  return bike.engineRunning;
    case 2:  return manualOn && bike.ignitionOn;
    case 3:  return false;
    default: return false;
  }
}

void updateAuxOutputs() {
  if (auxShouldBeOn(settings.aux1Mode, bike.aux1ManualOn)) {
    outputOn(PIN_AUX1_OUT);
  } else {
    outputOff(PIN_AUX1_OUT);
  }

  if (auxShouldBeOn(settings.aux2Mode, bike.aux2ManualOn)) {
    outputOn(PIN_AUX2_OUT);
  } else {
    outputOff(PIN_AUX2_OUT);
  }
}

// --------------------------------------------------------------------------
// PARKING LIGHT
// When ignition is off, show parking light based on parkingLightMode:
//   0 = Off
//   1 = Position light (low beam dimmed)
//   2 = Left turn indicator steady
//   3 = Right turn indicator steady
// --------------------------------------------------------------------------

void updateParkingLight() {
  // Only active when ignition is OFF
  if (bike.ignitionOn) return;

  switch (settings.parkingLightMode) {
    case 1: {
      // Dim position light (~25%)
      uint8_t duty = 64;
      outputPWM(PIN_LIGHT_OUT, duty);
      break;
    }
    case 2:
      outputOn(PIN_TURNL_OUT);
      break;
    case 3:
      outputOn(PIN_TURNR_OUT);
      break;
    default:
      // Mode 0: nothing (outputs already off from safety)
      break;
  }
}

// --------------------------------------------------------------------------
// ALARM SYSTEM
// State machine:
//   DISARMED → (ignition off for 30s) → ARMED
//   ARMED → (vibration detected) → TRIGGERED (horn + lights for 30s)
//   TRIGGERED → (30s elapsed) → ARMED (re-arm)
//   Any state → (ignition on) → DISARMED
// --------------------------------------------------------------------------

void updateAlarm() {
  if (settings.alarmMode == 0) {
    bike.alarmArmed     = false;
    bike.alarmTriggered = false;
    return;
  }

  unsigned long now = millis();

  // Ignition turns off alarm
  if (bike.ignitionOn) {
    bike.alarmArmed     = false;
    bike.alarmTriggered = false;
    bike.alarmTriggerTime = 0;
    return;
  }

  // Alarm triggered → sound horn + flash lights for ALARM_DURATION_MS
  if (bike.alarmTriggered) {
    unsigned long elapsed = now - bike.alarmTriggerTime;

    if (elapsed >= ALARM_DURATION_MS) {
      // Stop alarm, re-arm
      bike.alarmTriggered = false;
      outputOff(PIN_HORN_OUT);
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
      LOG_I("Alarm: timeout – re-armed");
      return;
    }

    // Flash turn signals + horn during alarm
    bool flashState = ((now / 200) % 2) == 0;  // 5Hz flash
    if (flashState) {
      outputOn(PIN_TURNL_OUT);
      outputOn(PIN_TURNR_OUT);
      outputOn(PIN_HORN_OUT);
    } else {
      outputOff(PIN_TURNL_OUT);
      outputOff(PIN_TURNR_OUT);
      outputOff(PIN_HORN_OUT);
    }
    return;
  }

  // Not yet armed → wait for arm delay after ignition off
  if (!bike.alarmArmed) {
    // alarmTriggerTime doubles as arm-delay start when not triggered
    if (bike.alarmTriggerTime == 0) {
      bike.alarmTriggerTime = now;  // Start counting from now
    }
    if (now - bike.alarmTriggerTime >= ALARM_ARM_DELAY_MS) {
      bike.alarmArmed = true;
      bike.alarmTriggerTime = 0;
      LOG_I("Alarm: armed");
    }
    return;
  }

  // Armed → check vibration sensor
  if (now - bike.lastAlarmCheck >= ALARM_VIBRATION_DEBOUNCE_MS) {
    bike.lastAlarmCheck = now;

    bool vibration = inputActive(PIN_VIBRATION);
    if (vibration) {
      // Count hits within window using static counters
      static uint8_t hitCount = 0;
      static unsigned long firstHitTime = 0;

      if (hitCount == 0) {
        firstHitTime = now;
      }

      hitCount++;

      // Check if we exceeded the window
      if (now - firstHitTime > ALARM_TRIGGER_WINDOW_MS) {
        hitCount = 1;
        firstHitTime = now;
      }

      if (hitCount >= ALARM_TRIGGER_THRESHOLD) {
        bike.alarmTriggered  = true;
        bike.alarmTriggerTime = now;
        hitCount = 0;
        LOG_W("Alarm: TRIGGERED!");
      }
    }
  }
}
