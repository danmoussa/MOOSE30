#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
command -v pio >/dev/null 2>&1 || { echo 'ERROR: pio is not on PATH. Run: export PATH="$HOME/.local/bin:$PATH"'; exit 1; }
echo '=== Cleaning and building classic ESP32 ==='
pio run -d "$ROOT/classic_esp32" -t clean
pio run -d "$ROOT/classic_esp32"
echo '=== Cleaning and building Waveshare ESP32-C6 display ==='
pio run -d "$ROOT/waveshare_c6_display" -t clean
pio run -d "$ROOT/waveshare_c6_display"
echo '=== BOTH BUILDS SUCCEEDED ==='
