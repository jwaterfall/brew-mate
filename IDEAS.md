# BrewMate — Feature Ideas

A running list of potential features and improvements to keep track of for BrewMate.
These are candidate ideas, not a committed roadmap or a priority order — somewhere to
park things worth considering.

## Bluetooth (BLE)

- **Timer control over BLE** — handle start/stop/reset commands (e.g. Bean Conqueror
  command bytes) so a connected app can drive the brew timer, with confirmation packets
  sent back.
- **Accept GaggiMate-originated commands** (product ID `0x02`) in addition to the existing
  protocol, so GaggiMate command messages are honoured rather than ignored.
- **BLE connection-info summary** — expose connected/advertising state, RSSI, a
  human-readable signal-quality bucket, and the connection handle for the web UI / diagnostics.
- **Init robustness** — free-heap guard before starting BLE, release Classic-BT memory,
  wrap init so the scale keeps running if BLE fails, and tune TX power explicitly.

## Web UI & HTTP API

- **On-device load-cell calibration** — a guided flow (tare → place a known weight →
  calibrate) plus endpoints to read/set the calibration factor and report raw scale status.
  There's currently no way to calibrate the load cell from the device or UI.
- **Live flow-rate chart** and a session/average flow readout.
- **Brew timer controls** in the web UI (start / stop / reset).
- **WiFi management UI** — scan for networks, turn WiFi off to save power, clear stored
  credentials.
- **Display & filter settings** — configurable decimal places, and brewing-detection /
  stability / sample-count tuning.
- **Device & firmware info** — version, board, build date, chip, free heap; optionally an
  updates/info page.
- **Fast/minimal weight endpoints** for third-party integrations (low-latency weight + flow JSON).
- **Richer home-screen status** — Bluetooth, WiFi-signal, and scale-connected indicators
  alongside the battery.
- **Factory-reset endpoint** — clear stored settings and restart.

## Scale, flow rate & filtering

- **On-device calibration routine** (see Web UI above).
- **Adaptive filtering** — average filter when stable, median filter while brewing, with a
  transition/hysteresis phase and a fast-path that snaps to large weight changes instantly.
- **Auto-select the brewing threshold** from the calibration-factor magnitude (different
  load-cell sizes).
- **Flow-rate refinements** — reject tare transitions so taring doesn't spike the reading,
  fast-zero on weight removal, pause/resume around a tare, and a per-shot average flow tied
  to the timer.
- **HX711 presence detection** — detect a missing/disconnected load cell and degrade
  gracefully / report status.
- **Multi-sample tare** (average several reads) and NaN/not-ready guards on reads.

## Display & touch

- **Additional screens** — a status page (battery, HX711, BT, WiFi + IP) and transient
  message screens (taring, tared, sleeping, countdowns, WiFi on/off, low battery) with
  auto-timeout.
- **Richer main layout** — larger weight, stacked timer + flow rows.
- **Brightness / contrast control.**
- **Critical-battery flashing indicator.**
- **More touch gestures** — medium-hold and long-hold actions on the pads (e.g. status page,
  WiFi toggle, sleep) alongside the current short-press / hold behaviours.

## Power management & sleep

- **Deep sleep with wake-on-touch** — an on-screen countdown and touch-to-cancel before
  sleeping; wake when the touch pad is pressed.
- **Low-battery boot guard** — show a warning and sleep instead of running when the battery
  is critically low at boot.
- **CPU underclocking** for power saving.
- **Battery status strings** (Full / Good / Fair / Low / Critical) and low/critical flags
  driving warnings.

## WiFi & networking

- **mDNS / hostname** (e.g. `brewmate.local`).
- **Network scanning.**
- **Runtime reconnection & AP-fallback maintenance** — detect dropped connections and retry,
  falling back to AP mode if needed (current connect happens once at startup).
- **Runtime WiFi enable/disable**, persisted across boots, for battery saving.
- **AP power optimisation** / per-board antenna & TX-power tuning.
- **Clear stored credentials.**

## System, firmware & persistence

- **Firmware version & build metadata** — version, board, build date — shown at boot and via
  the API.
- **Move persisted settings into NVS/Preferences** (calibration, filter params, display
  options, WiFi-enabled state), beyond the current WiFi + battery-calibration JSON.
- **Factory reset** — a boot-time gesture and/or API endpoint.
- **Headless degrade** — keep running (web / BLE available) if the display fails to
  initialise, rather than halting startup as it does today.

## Worth considering (not present today)

- **Runtime OTA firmware updates** — the partition layout already supports A/B images.
- **Software watchdog.**
- **Captive portal** for first-time WiFi setup.
- **Multiple saved WiFi networks.**
