#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
command -v pio >/dev/null 2>&1 || { echo 'ERROR: pio is not on PATH.'; exit 1; }
pio device monitor -d "$ROOT/classic_esp32" -b 115200
