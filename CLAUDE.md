# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

BrewMate is a compact (80×80×18mm) ESP32-based coffee scale, derived from [WeighMyBru](https://github.com/031devstudios/weighmybru2) but redesigned from scratch for a smaller form factor and custom UIs. The repo holds three things:

- **Firmware** (`src/`) — Arduino/PlatformIO C++ for the scale itself: weight, battery, display, touch input, buzzer, WiFi web server, and Bluetooth.
- **Web UI** (`web/`) — a Preact SPA served off the device over WiFi for configuration and live status.
- **Hardware** (`printed-parts/`, `assets/`, `README.md`) — 3D-printable case parts and the bill-of-materials differences from WeighMyBru.

⚠️ The firmware is a **work in progress** and not ready for daily use (see `README.md`). The case and hardware mods are usable with stock WeighMyBru software.

## Commands

Firmware uses **PlatformIO** (`pio`); the web UI uses **pnpm**.

| Command | Purpose |
| ------- | ------- |
| `pio run` | Build the firmware for the default env (`seeed_xiao_esp32c6`) |
| `pio run -t upload` | Flash the firmware over USB |
| `pio run -t uploadfs` | Build-less upload of the LittleFS image (the contents of `data/`) |
| `pio run -t monitor` | Serial monitor at 115200 baud |
| `cd web && pnpm install` | Install web UI dependencies |
| `cd web && pnpm dev` | Vite dev server (set `VITE_API_URL` to the device IP to hit a real scale) |
| `cd web && pnpm build` | Build the SPA — **outputs to `../data/`**, which is what `uploadfs` flashes |

Typical deploy: `cd web && pnpm build` → `pio run -t uploadfs` (web assets) → `pio run -t upload` (firmware).

There is no automated test suite. Verification is done on-device over serial and the web UI.

## Stack

**Firmware** (`platformio.ini`):

- **Arduino framework on ESP32** via `espressif32@6.12.0`. Default board is the **Seeed XIAO ESP32-C6** (`seeed_xiao_esp32c6` env, selected by the `BOARD_XIAO_ESP32C6` build flag).
- Display: **Adafruit SSD1306 / GFX** (128×32 OLED over I²C).
- Load cell: **RobTillaart/HX711**.
- Web server: **ESPAsyncWebServer + AsyncTCP** (ESP32Async forks), JSON via **ArduinoJson 7**.
- Bluetooth: **NimBLE-Arduino** (low-memory BLE stack).
- Filesystem: **LittleFS** (partition table in `partitions.csv`; web assets + `device_config.json` live here).

**Web UI** (`web/package.json`):

- **Preact** + **preact-iso** (`LocationProvider`/`Router`) for routing, built with **Vite**.
- **Tailwind v4** (via `@tailwindcss/vite`), `class-variance-authority` + `tailwind-merge` for variants, `lucide-preact` for icons.
- TypeScript throughout.

## Architecture

### Firmware

`src/main.cpp` is the orchestrator. It instantiates each subsystem as a global object, wires them together in `setup()`, and drives everything from a single cooperative `loop()` — **no RTOS tasks or blocking waits in the main loop** (the HX711 stability check in `Scale::begin()` is the one startup exception). Timing is done with `millis()` deltas against interval constants (`MIN_UPDATE_INTERVAL`, `ANIMATION_INTERVAL`, etc.).

Each subsystem is a **single self-contained class**, mostly **header-only** (the implementation lives inline in the `.h`); only `api_handler` and `bluetooth_scale` have separate `.cpp` files. The subsystems:

- `Scale` (`scale.h`) — HX711 wrapper; tare, calibration factor, filtered weight reads.
- `Battery` (`battery.h`) — ADC voltage reads through a divider, percentage/bar curve, USB-charging detection (VBUS pin) and battery-disconnect detection (switch pin). Calibration factors are persisted.
- `Display` (`display.h`) — all SSD1306 rendering; `showBootScreen()`, `showMainScreen(...)` take primitive state, not subsystem pointers.
- `TouchSensor` (`touchsensor.h`) — TTP223 inputs with debounce; exposes edge-triggered `isTarePressed()`/`isPowerPressed()`.
- `Buzzer` (`buzzer.h`) — LEDC hardware-PWM tones.
- `WiFiManager` (`wifi_manager.h`) — owns the `AsyncWebServer`; connects to saved STA credentials or falls back to the **`BrewMate` / `brewmate123` AP at `192.168.4.1`**; serves the SPA from LittleFS and delegates `/api/*` to `ApiHandler`.
- `ApiHandler` (`api_handler.{h,cpp}`) — the HTTP API (see below). Holds non-owning pointers to `Battery`/`Scale`/`WiFiManager`, injected via setters.
- `BluetoothScale` (`bluetooth_scale.{h,cpp}`) — NimBLE server emulating the WeighMyBru BLE protocol (GaggiMate + Bean Conqueror weight characteristics, command characteristic for tare).
- `ConfigManager` (`config_manager.h`) — static load/save of `DeviceConfig` (WiFi + battery calibration) as JSON in LittleFS at `/device_config.json`.
- `Logger` (`logger.h`) — leveled `printf`-style serial logging; set the level once in `setup()`.

**Pin assignments are centralized in `board_config.h`**, switched per-board by the `BOARD_*` build flag. Never hardcode a pin in a subsystem — add it to `board_config.h` (the `BOARD_ESP32S3_SUPERMINI` block shows the second-board pattern). New boards need a new `#elif` block there and a matching `[env:...]` in `platformio.ini`.

**Wiring pattern:** construct subsystems as globals, call `.begin()` in `setup()`, then inject cross-references via setters (`wifiManager.setBattery(&battery)`) — this avoids constructor-ordering and circular-include problems. Use forward declarations + pointers for cross-subsystem references (see `ApiHandler`), not includes.

### HTTP API

Served by `ApiHandler` (`api_handler.cpp`). All responses are JSON with permissive CORS; all routes have an `HTTP_OPTIONS` preflight handler.

- `GET  /api/status` — weight, battery %, voltage, USB/disconnect flags, WiFi state.
- `POST /api/tare` — tare the scale.
- `GET  /api/wifi/status` — connection / AP-mode state.
- `GET  /api/wifi/config` — current SSID (never returns the password).
- `POST /api/wifi/config` — save SSID/password (applied on next restart).
- `GET  /api/battery/calibration` — battery + VBUS calibration factors.
- `POST /api/battery/calibration` — update calibration factors (applied live + persisted).

Use the JSON helpers (`sendJsonResponse`/`sendJsonError`/`sendJsonSuccess`) rather than building responses by hand — they attach CORS headers and set the status code consistently. Guard handlers that need a subsystem with a null-pointer check returning `503`.

### Web UI

Standard Preact SPA: `web/src/index.tsx` mounts `App`, which wraps a `Router` (`/` → `Home`, `/settings` → `Settings`) in a persistent `Header`. Components live in `web/src/components/`, pages in `web/src/pages/`. All device calls go through `apiUrl()` in `web/src/utils/api.ts`, which prepends `VITE_API_URL` in dev and uses relative paths when served from the device. The build emits into `../data/` so the firmware can flash it to LittleFS.

## Development workflow

- **Don't `pio run` / `pnpm build` after every small edit** — batch related changes, then build once before flashing. The firmware build is slow.
- Verify on real hardware: flash, watch the serial monitor (115200), and exercise the web UI / BLE client. There are no unit tests to lean on.
- Do not commit or push without explicit approval from the user.
- When changing the web API, update **both** sides — the `ApiHandler` route and the web UI consumer — and keep the JSON shape in sync.

## Conventions

- **C++ subsystems are self-contained, header-first classes.** Keep a subsystem's logic inside its own class; expose a small public surface (`begin()`, queries, a few setters). Split to a `.cpp` only when the implementation is large or pulls in heavy includes (as `api_handler`/`bluetooth_scale` do).
- **All pin/hardware constants live in `board_config.h`**, gated by `BOARD_*` flags. Tuning constants (intervals, thresholds, calibration defaults) are `static constexpr` members on the owning class.
- **Cross-subsystem references are non-owning pointers injected after construction**, with forward declarations to avoid include cycles. The injector checks for null before use.
- **Header guards:** existing files use `#ifndef`/`#define`; newer ones (`bluetooth_scale.h`) use `#pragma once`. Match the file you're editing; prefer `#pragma once` for new files.
- **Log through `Logger`**, never raw `Serial.print` for status (raw serial is fine for transient debug like the WiFi connect dots). Pick the right level — `error`/`warn`/`info`/`debug`.
- **Persisted state goes through `ConfigManager`** as part of `DeviceConfig`; don't write ad-hoc files to LittleFS. Add a field to the relevant config struct and extend `load`/`save`.
- **Web file names:** components and pages are PascalCase (`WeightCard.tsx`, `Home/index.tsx`); utilities are camelCase (`api.ts`). Always reach the device through `apiUrl()`, never a hardcoded host.
- Indentation is **4 spaces** in C++ files, **2 spaces** in the web project.
