# Moto32 Firmware

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform: ESP32--WROOM--32D](https://img.shields.io/badge/Platform-ESP32--WROOM--32D-blue.svg)](https://www.espressif.com/)
[![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

Open-source motorcycle control unit firmware for Moto32 hardware.  
Current code base: **Firmware v2.2.0** (with additional stability updates integrated in this repository).

---

## What Changed Recently

### Functional updates
- Web dashboard is served directly from firmware (`src/web_ui_embedded.h`), no filesystem upload required.
- Dashboard header/status was reworked:
  - top title shows `MOTO32`
  - separate `WEB` and `BLE` connection status indicators.
- `Switch Unit` default is now **A – 5 Pushbuttons** (`CONFIG_A`).
- New **ADVANCED mode** in dashboard:
  - warning confirmation required before entering
  - output tiles can latch outputs on/off in ADVANCED mode
  - mode indicator/banner and visual mode coloring.
- Turn-distance calibration UX added:
  - button flow: start calibration, push bike 10m, confirm.
- BLE connect acknowledge blink sequence:
  - indicators blink quickly left/right once on BLE GATT connect.
- Starter light behavior updated:
  - while starter is engaged, lighting outputs are suppressed
  - previous low/high beam state is restored after starter release.

### Stability updates (top-priority fixes)
- WebSocket actions moved to deferred processing queue (loop task):
  - avoids heavy work in `async_tcp` callback context
  - reduces risk of `task_wdt` panic on `async_tcp`.
- State/keyless WebSocket broadcasts are now change-driven with keepalive:
  - lower background traffic and fewer bursts under load.
- NVS recovery was hardened:
  - settings and keyless namespaces now attempt recovery/reinit on open failure.
- Keyless detection reworked:
  - no sticky `detected` flag
  - uses recent-seen timeout (`lastSeenMs`) + RSSI threshold logic.
- Watchdog timeout increased from `3s` to `8s` for safer headroom.

---

## Core Features

- 11 MOSFET outputs (lights, horn, starter, ignition, AUX).
- 12 debounced inputs (lock, turn, light, start, horn, brake, kill, stand, AUX, speed).
- 7 brake light modes (continuous, fade, flash patterns, emergency mode).
- Auto turn signal cancel (time-based or pulse-distance based).
- Hazard logic (manual + emergency).
- Sidestand + kill switch safety priorities.
- BLE GATT interface for diagnostics/settings.
- BLE keyless ignition with paired-device proximity and grace period.
- WiFi AP + AsyncWebServer + WebSocket dashboard.
- OTA firmware update endpoint (`/api/ota`) and version endpoint (`/api/version`).
- Voltage monitoring with filtering and low/high voltage protection.
- Setup/calibration mode for output verification.
- Persistent settings in NVS.

---

## Web Dashboard

The dashboard is embedded into firmware and served from `/`.

### Access
- Connect phone/laptop to WiFi AP:
  - SSID: `Moto32`
  - Password: `moto3232`
- Open:
  - `http://192.168.4.1`
  - or `http://moto32.local` (if mDNS resolves on your client).

### Captive portal helpers
Requests to these paths are redirected to `/`:
- `/generate_204`
- `/gen_204`
- `/hotspot-detect.html`
- `/ncsi.txt`
- `/connecttest.txt`

### API endpoints
- `GET /api/state`
- `GET /api/settings`
- `GET /api/version`
- `POST /api/ota`

### WebSocket
- Endpoint: `/ws`
- Real-time state + keyless status updates.
- Commands include settings updates, keyless config, scan/pair flow, output toggle, restart, and distance calibration control.

---

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
| 35 | Battery voltage | ADC1 (with divider) |

### Outputs

| Pin | Function | Notes |
|-----|----------|-------|
| 22 | Left turn | MOSFET, HIGH = on |
| 21 | Right turn | MOSFET, HIGH = on |
| 19 | Low beam | MOSFET + PWM |
| 18 | High beam | MOSFET |
| 17 | Brake light | MOSFET + PWM |
| 16 | Horn relay | MOSFET |
| 5 | Starter out 1 | MOSFET |
| 4 | Starter out 2 | MOSFET |
| 2 | Ignition out | MOSFET |
| 15 | AUX out 1 | MOSFET |
| 0 | AUX out 2 | MOSFET |
| 3 | Status LED | GPIO |

---

## Build, Flash, Monitor

### Requirements

- PlatformIO CLI or VS Code PlatformIO extension
- USB serial adapter/cable
- ESP32-WROOM-32D board profile (`esp32dev`)

### Install `pio` CLI (if missing)

If shell returns `zsh: command not found: pio`:

```bash
python3 -m pip install --user platformio
```

Ensure your user bin path is in `PATH` (often `~/.local/bin`).

### Build

```bash
pio run -e esp32-wroom-32d
```

### Flash

```bash
pio run -e esp32-wroom-32d -t upload --upload-port /dev/cu.usbserial-140
```

### Serial monitor

```bash
pio device monitor --port /dev/cu.usbserial-140 --baud 115200
```

Important:
- If upload fails with **"port is busy"**, close any running monitor first (`Ctrl+C` in monitor terminal).

---

## UI Embedding (No `uploadfs`)

Filesystem upload is **not** required for dashboard deployment.

- UI source: `data/index.html`
- Embedded output: `src/web_ui_embedded.h`
- Firmware serves embedded HTML directly.

If `data/index.html` changes, regenerate `src/web_ui_embedded.h` before build:

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

---

## BLE Interface

Device name: `Moto32`

### GATT characteristics

| Characteristic | UUID suffix | Access | Description |
|---------------|-------------|--------|-------------|
| State | `...0001` | Read/Notify | Packed bike state |
| Voltage | `...0002` | Read/Notify | Battery voltage |
| Settings | `...0003` | Read/Write | Settings payload |
| Errors | `...0004` | Read/Notify | Error flags |
| Command | `...0005` | Write | `0x01` restart, `0x02` clear errors |

### Keyless notes

- Up to 3 paired devices.
- iOS devices may appear as `Apple Device` during scan (no local name in advertisement is normal).
- Detection uses RSSI threshold + recent scan freshness timeout.

---

## Setup Mode

1. Hold horn button while turning ignition on.
2. Setup mode starts (status LED pattern).
3. Hold horn for ~2s to exit.
4. Calibration/output verification sequence runs.

---

## Partition Layout

`partitions_ota.csv` currently uses:
- `nvs` (settings/keyless storage)
- `otadata`
- `app0`, `app1` (OTA slots)
- `spiffs` (reserved; dashboard is embedded and does not rely on FS upload)

---

## Troubleshooting

### Browser shows `Not found`
- Use `http://192.168.4.1/` exactly.
- Confirm firmware with embedded UI is flashed.
- Reboot board after upload.

### `Web UI not available. Upload filesystem image...`
- This message belongs to older FS-based builds.
- Current firmware serves embedded UI; no `uploadfs` needed.
- Flash latest firmware from this repo.

### Serial logs show `Preferences begin(): NOT_FOUND`
- This repository includes NVS recovery fallback.
- If persistent issues continue, erase flash once and reflash:
  - `pio run -e esp32-wroom-32d -t erase`
  - then upload firmware again.

### Upload error: `Could not exclusively lock port`
- Close monitor or any serial tool (VS Code monitor, screen, minicom, etc.).
- Retry upload command.

### Watchdog resets under heavy web/BLE usage
- Recent deferred WS action queue and reduced broadcast rate address this.
- Build and flash latest code from this repository.

---

## Project Structure

```
├── include/
│   ├── config.h
│   ├── state.h
│   ├── inputs.h
│   ├── outputs.h
│   ├── settings_store.h
│   ├── safety.h
│   ├── bike_logic.h
│   ├── setup_mode.h
│   ├── ble_interface.h
│   └── web_server.h
├── src/
│   ├── main.cpp
│   ├── inputs.cpp
│   ├── outputs.cpp
│   ├── settings_store.cpp
│   ├── safety.cpp
│   ├── bike_logic.cpp
│   ├── setup_mode.cpp
│   ├── ble_interface.cpp
│   ├── web_server.cpp
│   └── web_ui_embedded.h
├── data/
│   └── index.html
├── boards/
│   └── esp32-s3-devkitc-1-4mb.json
├── partitions_ota.csv
├── platformio.ini
└── README.md
```

---

## License

MIT
