# Moto32 Firmware

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform: ESP32--WROOM--32D](https://img.shields.io/badge/Platform-ESP32--WROOM--32D-blue.svg)](https://www.espressif.com/)
[![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

**Open-source motorcycle control unit firmware** for the [Moto32 hardware](https://github.com/moto32/moto32-hardware). A complete alternative to the Motogadget M-Unit Blue.

📖 **Documentation:** [moto32.vercel.app](https://moto32.vercel.app)

---

## Features

- **8 protected MOSFET outputs** – lights, horn, starter, ignition, aux
- **12 debounced inputs** – switches, brake, kill, sidestand, speed sensor
- **7 brake light modes** – continuous, PWM fade, flash patterns, emergency
- **Auto turn signal cancel** – time or distance based
- **Hazard lights** – manual (long-press both turns) + emergency braking
- **Sidestand safety** – engine kill when stand is down
- **BLE interface** – wireless configuration & live diagnostics via NimBLE
- **OTA-ready** – dual-partition layout for wireless firmware updates
- **Battery voltage monitoring** – ADC with filtered readings, low/high alerts
- **Hardware watchdog** – 3s timeout, automatic restart on firmware hang
- **Persistent settings** – NVS storage, survives power cycles
- **Setup/calibration mode** – output verification sequence

## Pin Mapping

### Inputs

| Pin | Function | Type |
|-----|----------|------|
| 39 | Ignition lock | Active HIGH (12V) |
| 36 | Turn left | Active LOW |
| 34 | Turn right | Active LOW |
| 32 | Light control | Active LOW |
| 33 | Starter button | Active LOW |
| 25 | Horn button | Active LOW |
| 26 | Brake switch | Active LOW |
| 27 | Kill switch | Active LOW |
| 14 | Sidestand switch | Active LOW |
| 12 | AUX input 1 | Active LOW |
| 13 | AUX input 2 | Active LOW |
| 23 | Speed sensor | Pulse input |
| 35 | Battery voltage | ADC1 (voltage divider) |

### Outputs

| Pin | Function | Notes |
|-----|----------|-------|
| 22 | Left turn | MOSFET, HIGH = on |
| 21 | Right turn | MOSFET, HIGH = on |
| 19 | Low beam | MOSFET + PWM capable |
| 18 | High beam | MOSFET |
| 17 | Brake light | MOSFET + PWM capable |
| 16 | Horn relay | MOSFET |
| 5 | Starter out 1 | MOSFET (2 pins for 30A) |
| 4 | Starter out 2 | MOSFET |
| 2 | Ignition | MOSFET |
| 15 | AUX output 1 | MOSFET |
| 0 | AUX output 2 | MOSFET |
| 3 | Status LED | GPIO |

## Building

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB-C cable

### Build & Flash

```bash
# Build
pio run

# Upload firmware
pio run --target upload

# Monitor serial output
pio device monitor
```

The web dashboard is embedded in firmware (`src/web_ui_embedded.h`), so no
`uploadfs` step is required.

If you change `data/index.html`, regenerate the embedded header before build:

```bash
cat > src/web_ui_embedded.h <<'EOF'
#pragma once

#include <Arduino.h>

// Generated from data/index.html. Keep this file in sync after UI changes.
static const char WEB_UI_HTML[] PROGMEM = R"WEBUI(
EOF
cat data/index.html >> src/web_ui_embedded.h
cat >> src/web_ui_embedded.h <<'EOF'
)WEBUI";

static constexpr size_t WEB_UI_HTML_LEN = sizeof(WEB_UI_HTML) - 1;
EOF
```

### Debug Build

Uncomment the `[env:debug]` section in `platformio.ini` for verbose logging.

## Setup Mode

1. **Hold horn button** while turning ignition on
2. Unit enters setup mode (status LED blinks rapidly)
3. **Hold horn for 2s** to exit → calibration sequence runs
4. Each output activates briefly to verify wiring

## BLE Interface

Connect with any BLE scanner app (e.g., nRF Connect) to `Moto32`.

| Characteristic | UUID | Access | Description |
|---------------|------|--------|-------------|
| State | `...0001` | Read/Notify | Packed bike state (8 bytes) |
| Voltage | `...0002` | Read/Notify | Battery voltage as string |
| Settings | `...0003` | Read/Write | All settings (14 bytes) |
| Errors | `...0004` | Read/Notify | Error flags (1 byte) |
| Command | `...0005` | Write | `0x01` = restart, `0x02` = clear errors |

## Project Structure

```
├── include/
│   ├── config.h          # Pin definitions, constants, enums
│   ├── state.h           # BikeState + Settings structs
│   ├── inputs.h          # Debouncing, button events
│   ├── outputs.h         # MOSFET control, PWM
│   ├── settings_store.h  # NVS persistence
│   ├── safety.h          # Watchdog, voltage, sidestand
│   ├── bike_logic.h      # All input/output handlers
│   ├── setup_mode.h      # Setup & calibration
│   └── ble_interface.h   # BLE GATT service
├── src/
│   ├── main.cpp          # setup() + loop() only
│   ├── inputs.cpp
│   ├── outputs.cpp
│   ├── settings_store.cpp
│   ├── safety.cpp
│   ├── bike_logic.cpp
│   ├── setup_mode.cpp
│   └── ble_interface.cpp
├── test/
│   └── roadmap_logic_tests.cpp
├── boards/
│   └── esp32-s3-devkitc-1-4mb.json   # legacy S3 board profile (unused)
├── partitions_ota.csv
└── platformio.ini
```

## License

MIT License – free to use, modify, and share.

## Contributing

Issues and PRs welcome! Please test on hardware before submitting.
