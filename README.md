# Moto32 Firmware

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-blue.svg)](https://www.espressif.com/)
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
| 46 | Ignition lock | Active HIGH (12V) |
| 47 | Turn left | Active LOW |
| 48 | Turn right | Active LOW |
| 21 | Light control | Active LOW |
| 22 | Starter button | Active LOW |
| 23 | Horn button | Active LOW |
| 1 | Brake switch | Active LOW |
| 2 | Kill switch | Active LOW |
| 3 | Sidestand switch | Active LOW |
| 4 | AUX input 1 | Active LOW |
| 5 | AUX input 2 | Active LOW |
| 6 | Speed sensor | Pulse input |
| 7 | Battery voltage | ADC (voltage divider) |

### Outputs

| Pin | Function | Notes |
|-----|----------|-------|
| 9 | Left turn | MOSFET, HIGH = on |
| 10 | Right turn | MOSFET, HIGH = on |
| 11 | Low beam | MOSFET + PWM capable |
| 12 | High beam | MOSFET |
| 13 | Brake light | MOSFET + PWM capable |
| 41 | Horn relay | MOSFET |
| 44 | Starter out 1 | MOSFET (2 pins for 30A) |
| 45 | Starter out 2 | MOSFET (⚠ strapping pin) |
| 42 | Ignition | MOSFET |
| 43 | AUX output 1 | MOSFET |
| 40 | AUX output 2 | MOSFET |
| 38 | Status LED | GPIO |

## Building

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB-C cable

### Build & Flash

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
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
│   └── esp32-s3-devkitc-1-4mb.json
├── partitions_ota.csv
└── platformio.ini
```

## License

MIT License – free to use, modify, and share.

## Contributing

Issues and PRs welcome! Please test on hardware before submitting.
