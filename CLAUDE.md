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

**All signal processing lives in `ScaleApp` (`core/scale_app.{h,cpp}`)** — a hardware-agnostic core that turns a `RawInputs` snapshot (raw HX711 counts, ADC millivolts, raw pin levels) into a `ScaleOutputs` (processed weight, battery state, timer, flow rate, the `DisplayState` to render, buzzer command). The subsystem classes are thin: they do the literal pin/bus reads and actuation, nothing else. Weight calibration/tare/deadband/smoothing (`WeightProcessor`), the battery voltage→% curve (`BatteryProcessor`), and touch debounce/edge detection (`TouchProcessor`) all live in `ScaleApp`. This same core runs on-device in standalone mode **and** on the host during live development (see below), so there is one implementation. All `ScaleApp` timing is injected via `nowMs`, never read from a clock.

The processed values the web API and BLE need are published to **`DeviceState` (`core/device_state.h`)** — a small shared struct the main loop fills from each `ScaleApp` tick. The API/BLE read live weight + battery from it and raise tare requests through it, so the driver classes stay pure raw readers with no caching.

`src/` is grouped by role: **`core/`** (portable, zero hardware deps — `scale_app`, `serial_protocol`, `display_render`, `device_state`), **`drivers/`** (thin hardware wrappers), **`net/`** (connectivity + persistence), **`host/`** (host dev runner + Arduino shim), and `main.cpp` / `board_config.h` / `logger.h` at the root. Subsystems are header-only classes; the policy is to split to a `.h`+`.cpp` only when the implementation pulls heavy framework includes (`net/api_handler`, `net/bluetooth_scale`, `net/wifi_manager`) or is shared logic compiled into multiple targets (`core/scale_app`). The subsystems:

- `Scale` (`drivers/scale.h`) — HX711 raw reader (`readRaw()`); stores the calibration factor (applied in `WeightProcessor`).
- `Battery` (`drivers/battery.h`) — raw ADC/pin reads (`readBatteryMv`/`readVbusMv`/`readSwitchRaw`) + calibration factors; interpretation is in `BatteryProcessor`.
- `Display` (`drivers/display.h`) — SSD1306 glue: `begin()`, `showMainScreen(DisplayState)` (standalone), `drawFrame(buffer)` (blits a host-rendered framebuffer in proxy mode). The actual drawing is in `core/display_render.h`.
- `core/display_render.h` — shared screen rendering into any `Adafruit_GFX&`, so the SSD1306 (device) and a `GFXcanvas1` (host) produce pixel-identical output.
- `TouchSensor` (`drivers/touch_sensor.h`) — raw TTP223 level reads (`readTareRaw`/`readPowerRaw`); debounce/edges are in `TouchProcessor`.
- `Buzzer` (`drivers/buzzer.h`) — LEDC hardware-PWM tones.
- `WiFiManager` (`net/wifi_manager.{h,cpp}`) — owns the `AsyncWebServer`; connects to saved STA credentials or falls back to the **`BrewMate` / `brewmate123` AP at `192.168.4.1`**; serves the SPA from LittleFS and delegates `/api/*` to `ApiHandler`.
- `ApiHandler` (`net/api_handler.{h,cpp}`) — the HTTP API (see below). Holds non-owning pointers to `DeviceState`/`Battery`/`WiFiManager`, injected via setters.
- `BluetoothScale` (`net/bluetooth_scale.{h,cpp}`) — NimBLE server emulating the WeighMyBru BLE protocol (GaggiMate + Bean Conqueror weight characteristics, command characteristic for tare); reads weight from `DeviceState`.
- `ConfigManager` (`net/config_manager.h`) — static load/save of `DeviceConfig` (WiFi + battery calibration) as JSON in LittleFS at `/device_config.json`.
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

### Live development (host dev server)

Because all logic and rendering live in `ScaleApp` + `display_render.h` (shared, hardware-agnostic code), you can develop them on your laptop with no flashing:

- Flash the firmware once, then run **`./scripts/dev_host.sh [port]`** (default `/dev/ttyACM0`). It builds `src/host/host_runner.cpp` and watches the shared sources, rebuilding + restarting on save.
- The scale detects the running dev server (a `BMTAP` handshake over serial) and switches into **proxy mode**: it streams raw sensor reads up, the host runs `ScaleApp` and renders the OLED framebuffer, and streams it back for the device to blit. Stop the server (or unplug) and the scale reverts to a normal standalone device after ~1.5s. There is no separate firmware — it's one image with a runtime switch.
- The serial protocol is in `serial_protocol.h`. The host build needs a C++ compiler (`g++`) and compiles the real Adafruit GFX library against a tiny Arduino shim in `src/host/compat/` for pixel-identical rendering (`host_runner` and `.host_build/` are gitignored build artifacts).
- Edit `core/scale_app.cpp` for logic, `core/display_render.h` for the screen — both hot-reload. Editing the thin subsystem `.h` files or `main.cpp` still needs a reflash.

Holding the tare touch pad for ~2s shows the device IP on screen (to find it for the web UI); the `INFO_HOLD_MS` threshold is in `ScaleApp`.

## Conventions

- **`src/` is grouped by role** — `core/` (portable, no hardware deps), `drivers/` (hardware wrappers), `net/` (connectivity), `host/` (host dev runner). Include paths are set in `platformio.ini`, so headers are included by basename (`#include "scale_app.h"`), not relative path.
- **C++ subsystems are self-contained, header-only classes by default.** Keep a subsystem's logic inside its own class; expose a small public surface (`begin()`, queries, a few setters). Split to a `.h`+`.cpp` only when the implementation pulls heavy framework includes (`net/*`) or is shared logic compiled into multiple targets (`core/scale_app`).
- **File names are `snake_case`; classes/structs are `PascalCase`** (`touch_sensor.h` → `class TouchSensor`).
- **All pin/hardware constants live in `board_config.h`**, gated by `BOARD_*` flags. Tuning constants (intervals, thresholds, calibration defaults) are `static constexpr` members on the owning class.
- **Cross-subsystem references are non-owning pointers injected after construction**, with forward declarations to avoid include cycles. The injector checks for null before use. Processed values shared with the API/BLE go through `DeviceState`, not caches on the driver classes.
- **Header guards: use `#pragma once`.**
- **Log through `Logger`**, never raw `Serial.print` for status (raw serial is fine for transient debug like the WiFi connect dots). Pick the right level — `error`/`warn`/`info`/`debug`.
- **Persisted state goes through `ConfigManager`** as part of `DeviceConfig`; don't write ad-hoc files to LittleFS. Add a field to the relevant config struct and extend `load`/`save`.
- **Web file names:** components and pages are PascalCase (`WeightCard.tsx`, `Home/index.tsx`); utilities are camelCase (`api.ts`). Always reach the device through `apiUrl()`, never a hardcoded host.
- Indentation is **4 spaces** in C++ files, **2 spaces** in the web project.
- **Don't add comments unless they're necessary** — keep a comment only when removing it would genuinely hinder understanding of the code (a non-obvious constraint, a hardware quirk, why something is done a surprising way). Don't add comments that restate what the code already says.
