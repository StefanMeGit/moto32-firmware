#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

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

static void updateButtonEventSim(
    ButtonEvent& event,
    bool state,
    unsigned long now,
    unsigned long longPressMs = 0,
    unsigned long doubleClickMs = 400) {
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

enum CalibrationState { CALIB_IDLE, CALIB_RUNNING, CALIB_DONE };

struct CalibrationMachine {
  CalibrationState state = CALIB_IDLE;
  int stepIndex = 0;
  unsigned long stepStart = 0;
  bool outputOn = false;
  static constexpr unsigned long kStepDuration = 500;
  static constexpr int kStepCount = 6;

  void start(unsigned long now) {
    state = CALIB_RUNNING;
    stepIndex = 0;
    stepStart = now;
    outputOn = true;
  }

  void process(unsigned long now) {
    if (state != CALIB_RUNNING) return;
    if (now - stepStart < kStepDuration) return;

    if (outputOn) {
      outputOn = false;
      stepStart = now;
      return;
    }

    stepIndex++;
    if (stepIndex >= kStepCount) {
      state = CALIB_DONE;
      return;
    }

    outputOn = true;
    stepStart = now;
  }
};

int main() {
  // Long press one-shot
  ButtonEvent e;
  updateButtonEventSim(e, true, 0, 2000);
  assert(e.pressed);
  updateButtonEventSim(e, true, 1999, 2000);
  assert(!e.longPress);
  updateButtonEventSim(e, true, 2001, 2000);
  assert(e.longPress);
  updateButtonEventSim(e, true, 2500, 2000);
  assert(!e.longPress);  // latched one-shot

  // Double click detection
  updateButtonEventSim(e, false, 2600);
  assert(e.released);
  updateButtonEventSim(e, true, 2850, 0, 400);
  assert(e.doubleClick);

  // Calibration sequencing should be non-blocking and deterministic
  CalibrationMachine cm;
  cm.start(1000);
  assert(cm.state == CALIB_RUNNING);
  assert(cm.outputOn);

  cm.process(1200);
  assert(cm.outputOn); // no transition before duration

  cm.process(1500);
  assert(!cm.outputOn); // OFF transition

  // run complete cycles for all steps
  unsigned long t = 1500;
  while (cm.state == CALIB_RUNNING) {
    t += 500;
    cm.process(t);
  }
  assert(cm.state == CALIB_DONE);

  std::cout << "roadmap logic tests passed\n";
  return 0;
}
