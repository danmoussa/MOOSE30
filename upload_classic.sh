#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
command -v pio >/dev/null 2>&1 || { echo 'ERROR: pio is not on PATH. Run: export PATH="$HOME/.local/bin:$PATH"'; exit 1; }
pio run -d "$ROOT/classic_esp32" -t upload
