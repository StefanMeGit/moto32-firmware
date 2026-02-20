#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <cmath>

// ============================================================================
// Simulated types from the firmware (standalone test, no Arduino deps)
// ============================================================================

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
    ButtonEvent& event, bool state, unsigned long now,
    unsigned long longPressMs = 0, unsigned long doubleClickMs = 400) {
  event.pressed = state && !event.state;
  event.released = !state && event.state;
  event.doubleClick = false;
  if (event.pressed) {
    if (now - event.lastReleaseTime <= doubleClickMs) event.doubleClick = true;
    event.pressStartTime = now;
    event.longPressLatched = false;
  }
  if (event.released) event.lastReleaseTime = now;
  event.longPress = false;
  if (state && longPressMs > 0 && !event.longPressLatched
      && now - event.pressStartTime >= longPressMs) {
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
    state = CALIB_RUNNING; stepIndex = 0; stepStart = now; outputOn = true;
  }
  void process(unsigned long now) {
    if (state != CALIB_RUNNING) return;
    if (now - stepStart < kStepDuration) return;
    if (outputOn) { outputOn = false; stepStart = now; return; }
    stepIndex++;
    if (stepIndex >= kStepCount) { state = CALIB_DONE; return; }
    outputOn = true; stepStart = now;
  }
};

// ============================================================================
// NEW: Starter state machine simulation (FIX #3 test)
// ============================================================================

struct StarterSim {
  bool starterEngaged = false;
  bool engineRunning = false;
  bool killActive = false;
  unsigned long starterStartTime = 0;
  static constexpr unsigned long ENGAGE_DELAY = 1500;
  static constexpr unsigned long MAX_DURATION = 5000;

  void pressStart(unsigned long now) {
    if (!killActive && !engineRunning) {
      starterEngaged = true;
      starterStartTime = now;
    }
  }

  void holdStart(unsigned long now) {
    if (!starterEngaged) return;
    unsigned long elapsed = now - starterStartTime;
    if (elapsed >= ENGAGE_DELAY && !engineRunning) {
      engineRunning = true;
      killActive = false;
    }
    if (elapsed >= MAX_DURATION) {
      starterEngaged = false;  // Timeout
    }
  }

  void releaseStart() {
    starterEngaged = false;
  }
};

// ============================================================================
// NEW: Kill switch simulation (FIX #4 test)
// ============================================================================

struct KillSwitchSim {
  bool killActive = false;
  void update(bool killState, uint8_t mode) {
    uint8_t cfg = mode % 3;
    switch (cfg) {
      case 0: killActive = killState;  break;  // N/O
      case 1: killActive = !killState; break;  // N/C
      case 2: killActive = false;      break;  // Disabled
    }
  }
};

// ============================================================================
// NEW: Voltage monitor simulation (FIX #10 test)
// ============================================================================

struct VoltageSim {
  float voltage = 12.6f;
  uint8_t errorFlags = 0;
  void check() {
    if (voltage < 10.0f && voltage > 1.0f)      errorFlags |= 0x01;
    else                                          errorFlags &= ~0x01;
    if (voltage > 15.5f)                          errorFlags |= 0x02;
    else                                          errorFlags &= ~0x02;
  }
};

// ============================================================================
// TESTS
// ============================================================================

int main() {
  std::cout << "=== Moto32 Logic Tests v2.0 ===" << std::endl;

  // --- Button event tests (existing) ---
  {
    ButtonEvent e;
    updateButtonEventSim(e, true, 0, 2000);
    assert(e.pressed);
    updateButtonEventSim(e, true, 1999, 2000);
    assert(!e.longPress);
    updateButtonEventSim(e, true, 2001, 2000);
    assert(e.longPress);
    updateButtonEventSim(e, true, 2500, 2000);
    assert(!e.longPress);  // latched

    updateButtonEventSim(e, false, 2600);
    assert(e.released);
    updateButtonEventSim(e, true, 2850, 0, 400);
    assert(e.doubleClick);
    std::cout << "[PASS] Button events" << std::endl;
  }

  // --- Calibration tests (existing) ---
  {
    CalibrationMachine cm;
    cm.start(1000);
    assert(cm.state == CALIB_RUNNING && cm.outputOn);
    cm.process(1200);
    assert(cm.outputOn);
    cm.process(1500);
    assert(!cm.outputOn);
    unsigned long t = 1500;
    while (cm.state == CALIB_RUNNING) { t += 500; cm.process(t); }
    assert(cm.state == CALIB_DONE);
    std::cout << "[PASS] Calibration sequence" << std::endl;
  }

  // --- FIX #3: Starter logic ---
  {
    StarterSim s;
    // Press start at t=0
    s.pressStart(0);
    assert(s.starterEngaged);
    assert(!s.engineRunning);

    // After 1s, still not running
    s.holdStart(1000);
    assert(s.starterEngaged);
    assert(!s.engineRunning);

    // After 1.5s, engine should start
    s.holdStart(1500);
    assert(s.starterEngaged);
    assert(s.engineRunning);

    // Release → starter disengaged
    s.releaseStart();
    assert(!s.starterEngaged);
    assert(s.engineRunning);

    // Test timeout
    StarterSim s2;
    s2.pressStart(0);
    s2.holdStart(5000);
    assert(!s2.starterEngaged);  // Timed out
    std::cout << "[PASS] Starter logic (FIX #3)" << std::endl;
  }

  // --- FIX #4: Kill switch reset ---
  {
    KillSwitchSim k;

    // N/O mode: active when signal active
    k.update(true, 0);
    assert(k.killActive);
    k.update(false, 0);
    assert(!k.killActive);  // Must reset!

    // N/C mode: active when signal inactive
    k.update(false, 1);
    assert(k.killActive);
    k.update(true, 1);
    assert(!k.killActive);

    // Disabled mode
    k.update(true, 2);
    assert(!k.killActive);
    std::cout << "[PASS] Kill switch reset (FIX #4)" << std::endl;
  }

  // --- FIX #10: Voltage monitoring ---
  {
    VoltageSim v;
    v.voltage = 12.6f;
    v.check();
    assert(v.errorFlags == 0);

    v.voltage = 9.5f;
    v.check();
    assert(v.errorFlags & 0x01);  // Low voltage flag

    v.voltage = 12.0f;
    v.check();
    assert(!(v.errorFlags & 0x01));  // Cleared

    v.voltage = 16.0f;
    v.check();
    assert(v.errorFlags & 0x02);  // High voltage flag

    v.voltage = 13.0f;
    v.check();
    assert(v.errorFlags == 0);  // All clear
    std::cout << "[PASS] Voltage monitoring (FIX #10)" << std::endl;
  }

  std::cout << "\n=== All tests passed ===" << std::endl;
  return 0;
}
