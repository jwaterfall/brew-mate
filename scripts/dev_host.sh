#!/usr/bin/env bash
# Live dev server for the host runner: builds it, runs it against the scale, and
# rebuilds + restarts on every source save. No flashing -- edit scale_app.cpp /
# display_render.h and just save. Ctrl-C to quit.
#
# Usage:  ./scripts/dev_host.sh [serial_port]   (default /dev/ttyACM0)
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${1:-/dev/ttyACM0}"
BIN="$ROOT/host_runner"
CACHE="$ROOT/.host_build"

WATCH=(
    "$ROOT/src/core/scale_app.cpp"
    "$ROOT/src/core/scale_app.h"
    "$ROOT/src/core/serial_protocol.h"
    "$ROOT/src/core/display_render.h"
    "$ROOT/src/host/host_runner.cpp"
)

CXX=""
for c in g++ clang++ c++; do
    command -v "$c" >/dev/null 2>&1 && { CXX="$c"; break; }
done
[ -z "$CXX" ] && { echo "No C++ compiler found; try: sudo apt install g++" >&2; exit 1; }

# Adafruit GFX is fetched by PlatformIO into .pio/libdeps when the firmware is
# built; the host renders with the same library for pixel parity.
GFX_DIR="$(find "$ROOT/.pio/libdeps" -maxdepth 2 -iname "Adafruit GFX Library" -type d 2>/dev/null | head -1)"
[ -z "$GFX_DIR" ] && { echo "Adafruit GFX not found; build the firmware once: pio run -e seeed_xiao_esp32c6" >&2; exit 1; }

# -DARDUINO selects the Arduino.h include path inside Adafruit_GFX.h.
FLAGS=(-std=gnu++17 -O2 -DARDUINO=10805 -I "$ROOT/src/host/compat" -I "$ROOT/src/core" -I "$GFX_DIR")

build() {
    mkdir -p "$CACHE"
    local gfx_obj="$CACHE/Adafruit_GFX.o"
    # Compile the large, unchanging GFX core once (-w: silence third-party warnings).
    if [ ! -f "$gfx_obj" ] || [ "$GFX_DIR/Adafruit_GFX.cpp" -nt "$gfx_obj" ]; then
        "$CXX" "${FLAGS[@]}" -w -c "$GFX_DIR/Adafruit_GFX.cpp" -o "$gfx_obj" || return 1
    fi
    "$CXX" "${FLAGS[@]}" -Wall "$ROOT/src/core/scale_app.cpp" "$ROOT/src/host/host_runner.cpp" "$gfx_obj" -o "$BIN"
}

snapshot() { stat -c '%Y' "${WATCH[@]}" 2>/dev/null | tr '\n' ' '; }

RUNNER_PID=""
stop_runner() {
    [ -n "$RUNNER_PID" ] && kill "$RUNNER_PID" 2>/dev/null && wait "$RUNNER_PID" 2>/dev/null
    RUNNER_PID=""
}
cleanup() { stop_runner; echo; echo "dev server stopped."; exit 0; }
trap cleanup INT TERM

echo "BrewMate dev server -- port $PORT, watching for changes"
echo
if build; then "$BIN" "$PORT" & RUNNER_PID=$!; else echo "Build failed -- fix and save to retry."; fi

prev="$(snapshot)"
while true; do
    sleep 0.5
    cur="$(snapshot)"
    [ "$cur" = "$prev" ] && continue
    prev="$cur"
    echo; echo "--- rebuilding ---"
    stop_runner
    if build; then "$BIN" "$PORT" & RUNNER_PID=$!; else echo "Build failed -- waiting for next change..."; fi
done
