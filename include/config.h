#pragma once

#include <Arduino.h>

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_STRING "0.1-alpha"

// Project maintainer / software developer contact:
// STEFAN WAHRENDORFF - Stefan.Wahrendorff@gmail.com

// ============================================================================
// LOG SYSTEM
// ============================================================================
enum LogLevel : uint8_t {
  LOG_NONE  = 0,
  LOG_ERROR = 1,
  LOG_WARN  = 2,
  LOG_INFO  = 3,
  LOG_DEBUG = 4
};

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

#define LOG_E(fmt, ...) do { if (LOG_LEVEL >= LOG_ERROR) Serial.printf("[E] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_W(fmt, ...) do { if (LOG_LEVEL >= LOG_WARN)  Serial.printf("[W] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_I(fmt, ...) do { if (LOG_LEVEL >= LOG_INFO)  Serial.printf("[I] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_D(fmt, ...) do { if (LOG_LEVEL >= LOG_DEBUG) Serial.printf("[D] " fmt "\n", ##__VA_ARGS__); } while(0)

// ============================================================================
// PIN DEFINITIONS – INPUTS (active LOW unless noted)
// ============================================================================
#define PIN_LOCK       39    // Ignition lock (active HIGH – connects to +12V)
#define PIN_TURNL      36    // Turn left
#define PIN_TURNR      34    // Turn right
#define PIN_LIGHT      32    // Light control
#define PIN_START      33    // Starter button
#define PIN_HORN       25    // Horn button
#define PIN_BRAKE      26    // Brake switch (front/rear parallel)
#define PIN_KILL       27    // Kill switch
#define PIN_STAND      14    // Side stand switch
#define PIN_AUX1       12    // Auxiliary input 1
#define PIN_AUX2       13    // Auxiliary input 2
#define PIN_SPEED      23    // Speed sensor (pulse input)

// ============================================================================
// PIN DEFINITIONS – OUTPUTS (HIGH = on, MOSFET switches +12V)
// ============================================================================
#define PIN_TURNL_OUT   22   // Left turn indicator
#define PIN_TURNR_OUT   21   // Right turn indicator
#define PIN_LIGHT_OUT   19   // Low beam
#define PIN_HIBEAM_OUT  18   // High beam
#define PIN_BRAKE_OUT   17   // Brake light
#define PIN_HORN_OUT    16   // Horn relay
#define PIN_START_OUT1   5   // Starter output 1 (2 wires for 30A)
#define PIN_START_OUT2   4   // Starter output 2
#define PIN_IGN_OUT      2   // Ignition system
#define PIN_AUX1_OUT    15   // Auxiliary output 1
#define PIN_AUX2_OUT     0   // Auxiliary output 2

// Status LED
#define LED_STATUS       3

// ESP32 max GPIO index + 1 (GPIOs 0-39)
#define MAX_PIN         40

// Battery voltage ADC (needs external voltage divider 47k/10k)
#define PIN_VBAT_ADC    35

// Vibration/shock sensor for alarm (shares AUX2 input when alarm enabled)
#define PIN_VIBRATION    PIN_AUX2

// ============================================================================
// OUTPUT PIN TABLE (for iteration)
// ============================================================================
static const int OUTPUT_PINS[] = {
  PIN_TURNL_OUT, PIN_TURNR_OUT, PIN_LIGHT_OUT, PIN_HIBEAM_OUT,
  PIN_BRAKE_OUT, PIN_HORN_OUT, PIN_START_OUT1, PIN_START_OUT2,
  PIN_IGN_OUT, PIN_AUX1_OUT, PIN_AUX2_OUT
};
static const int OUTPUT_PIN_COUNT = sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]);

// Calibration output sequence
static const int CALIBRATION_PINS[] = {
  PIN_TURNL_OUT, PIN_TURNR_OUT, PIN_LIGHT_OUT,
  PIN_HIBEAM_OUT, PIN_BRAKE_OUT, PIN_AUX1_OUT
};
static const int CALIBRATION_PIN_COUNT = sizeof(CALIBRATION_PINS) / sizeof(CALIBRATION_PINS[0]);

// ============================================================================
// TIMING CONSTANTS (ms)
// ============================================================================
#define DEBOUNCE_DELAY_MS            50
#define FLASHER_PERIOD_MS           667     // 1.5 Hz = ~667ms
#define STARTER_MAX_DURATION_MS    5000     // Max starter run time
#define STARTER_ENGAGE_DELAY_MS    1500     // Delay before marking engine running
#define LIGHT_SHORT_PRESS_MS        500
#define LIGHT_LONG_PRESS_MS        2000
#define DOUBLE_CLICK_WINDOW_MS      400
#define LONG_PRESS_THRESHOLD_MS    2000
#define SETUP_FLASH_INTERVAL_MS     200
#define CALIBRATION_STEP_MS         500
#define STATUS_LED_ON_MS            100
#define STATUS_LED_PERIOD_MS       1000
#define BRAKE_FLASH_PERIOD_MS       200     // 5Hz flash
#define BRAKE_FADE_PERIOD_MS        333     // 3Hz fade cycle
#define BRAKE_8X_DURATION_MS       1600     // 8 flashes
#define BRAKE_2X_DURATION_MS        800     // 2 flashes
#define BRAKE_CONTINUOUS_DELAY_MS  3000     // 3s before flash

// Turn signal auto-off
#define TURN_DISTANCE_TIMEOUT_MS  10000
#define TURN_10S_TIMEOUT_MS       10000
#define TURN_20S_TIMEOUT_MS       20000
#define TURN_30S_TIMEOUT_MS       30000
#define TURN_DISTANCE_MIN_PULSES     10
#define TURN_DISTANCE_MAX_PULSES   1000

// Alarm system
#define ALARM_ARM_DELAY_MS        30000     // 30s after ignition off to arm
#define ALARM_DURATION_MS         30000     // 30s alarm sounding duration
#define ALARM_VIBRATION_DEBOUNCE_MS 100     // Debounce vibration sensor
#define ALARM_TRIGGER_THRESHOLD       3     // Hits within window to trigger
#define ALARM_TRIGGER_WINDOW_MS    2000     // Window for counting vibration hits

// Watchdog
#define WATCHDOG_TIMEOUT_S            8

// Battery voltage thresholds
#define VBAT_DIVIDER_RATIO        5.7f     // 47k + 10k voltage divider
#define VBAT_PRESENT_MIN_VOLTAGE  6.0f     // Below this, treat VBAT input as disconnected/floating
#define VBAT_WARNING_LOW         11.0f     // Low voltage warning
#define VBAT_CRITICAL_LOW        10.0f     // Critical – shutdown non-essential
#define VBAT_WARNING_HIGH        15.0f     // Overvoltage warning
#define VBAT_CRITICAL_HIGH       15.5f     // Critical overvoltage
#define VBAT_SAMPLE_INTERVAL_MS   1000     // Read every 1s
#define VBAT_FILTER_SAMPLES          8     // Moving average window

// PWM configuration
#define PWM_FREQ_HZ              5000
#define PWM_RESOLUTION_BITS         8      // 0-255
#define PWM_CHANNEL_BRAKE           0
#define PWM_CHANNEL_POSITION        1
#define PWM_CHANNEL_HIBEAM          2
#define PWM_CHANNEL_AUX1            3
#define PWM_CHANNEL_AUX2            4

// BLE
#define BLE_DEVICE_NAME        "Moto32"
#define BLE_SERVICE_UUID       "4d6f7432-0001-0000-0000-000000000000"
#define BLE_CHAR_STATE_UUID    "4d6f7432-0001-0001-0000-000000000000"
#define BLE_CHAR_VOLTAGE_UUID  "4d6f7432-0001-0002-0000-000000000000"
#define BLE_CHAR_SETTINGS_UUID "4d6f7432-0001-0003-0000-000000000000"
#define BLE_CHAR_ERRORS_UUID   "4d6f7432-0001-0004-0000-000000000000"
#define BLE_CHAR_COMMAND_UUID  "4d6f7432-0001-0005-0000-000000000000"

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum HandlebarConfig : uint8_t {
  CONFIG_A = 0,  // 5 pushbuttons
  CONFIG_B = 1,  // Harley/BMW
  CONFIG_C = 2,  // Japanese/European
  CONFIG_D = 3,  // Ducati
  CONFIG_E = 4   // 4 pushbuttons
};

enum TurnSignalMode : uint8_t {
  TURN_OFF      = 0,
  TURN_DISTANCE = 1,  // 50m or 10s at >10km/h
  TURN_10S      = 2,
  TURN_20S      = 3,
  TURN_30S      = 4
};

enum BrakeLightMode : uint8_t {
  BRAKE_CONTINUOUS  = 0,
  BRAKE_FADE        = 1,  // Fade in/out 3Hz (PWM)
  BRAKE_FLASH_5HZ   = 2,
  BRAKE_FLASH_8X    = 3,  // 8x flash then continuous
  BRAKE_FLASH_2X    = 4,  // 2x flash then 1s continuous
  BRAKE_3S_FLASH    = 5,  // 3s continuous then flash
  BRAKE_EMERGENCY   = 6   // Emergency braking mode
};

enum CalibrationState : uint8_t {
  CALIB_IDLE,
  CALIB_RUNNING,
  CALIB_DONE
};

enum ErrorCode : uint8_t {
  ERR_NONE            = 0x00,
  ERR_LOW_VOLTAGE     = 0x01,
  ERR_HIGH_VOLTAGE    = 0x02,
  ERR_STARTER_TIMEOUT = 0x04,
  ERR_WATCHDOG_RESET  = 0x08,
  ERR_STAND_KILL      = 0x10
};
