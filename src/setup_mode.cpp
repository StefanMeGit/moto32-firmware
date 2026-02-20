#include "setup_mode.h"
#include "outputs.h"
#include "inputs.h"

void setupModeCheck() {
  refreshInputEvents();
  if (hornEvent.state) {
    bike.inSetupMode   = true;
    bike.setupEnterTime = millis();
    LOG_I("Entering SETUP mode");
  }
}

void startCalibrationSequence() {
  bike.calibrationState       = CALIB_RUNNING;
  bike.calibrationStepIndex   = 0;
  bike.calibrationStepStart   = millis();
  bike.calibrationStepOutputOn = true;
  LOG_I("Calibration started");
  outputOn(CALIBRATION_PINS[0]);
}

void processCalibrationSequence() {
  if (bike.calibrationState != CALIB_RUNNING) return;

  unsigned long now = millis();
  if (now - bike.calibrationStepStart < CALIBRATION_STEP_MS) return;

  if (bike.calibrationStepOutputOn) {
    outputOff(CALIBRATION_PINS[bike.calibrationStepIndex]);
    bike.calibrationStepOutputOn = false;
    bike.calibrationStepStart = now;
    return;
  }

  bike.calibrationStepIndex++;
  if (bike.calibrationStepIndex >= CALIBRATION_PIN_COUNT) {
    bike.calibrationState = CALIB_DONE;
    LOG_I("Calibration completed");
    return;
  }

  outputOn(CALIBRATION_PINS[bike.calibrationStepIndex]);
  bike.calibrationStepOutputOn = true;
  bike.calibrationStepStart = now;
}

bool setupModeHandleExit() {
  if (bike.inSetupMode && hornEvent.state
      && millis() - bike.setupEnterTime > LONG_PRESS_THRESHOLD_MS) {
    bike.inSetupMode = false;
    LOG_I("Exiting SETUP mode – calibration starting");
    startCalibrationSequence();
    return true;
  }
  return false;
}
