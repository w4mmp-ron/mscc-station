#!/bin/bash
# Burn the Black Pill bootloader (or bootloader + app) at 0x08000000 via ST-Link.
# Linux / Raspberry Pi version of flash-bootloader.ps1. Uses st-flash (stlink-tools).
#
# Usage:
#   ./flash-bootloader.sh <file.bin>   # image at 0x08000000, e.g.
#       boot-app-firmware-async.bin (bootloader + app, new board) or bootload-PA8.bin
# 1.0.5: no default image (the package carries none, Ron: users would flash an old one).
#
# Needs: sudo apt install stlink-tools
# Run with sudo if the ST-Link isn't accessible to your user (no udev rule).
# App-only updates over USB DFU (no ST-Link): ./flash-proficio.sh

set -euo pipefail
BL_ADDR="0x08000000"
[[ $# -eq 1 ]] || {
  echo "Usage: sudo proficio-flash-bootloader <file.bin>"
  echo "  Image for 0x08000000: boot-app-firmware-async.bin (bootloader + app) or bootload-PA8.bin."
  exit 1
}
FW="$1"

[[ -f "$FW" ]] || { echo "ERROR: missing $FW"; exit 1; }
command -v st-flash >/dev/null || {
  echo "ERROR: st-flash not found. sudo apt install stlink-tools"; exit 1; }

echo "Image   : $FW"
echo "Address : $BL_ADDR"
echo "Size    : $(stat -c %s "$FW") bytes, $(date -r "$FW" '+%Y-%m-%d %H:%M')"
echo

echo "=== Probe ST-Link / STM32F411 ==="
PROBE="$(st-info --probe 2>&1 || true)"
echo "$PROBE"
if echo "$PROBE" | grep -qi "Found 0 stlink"; then
  echo "ERROR: no ST-Link found. Check USB, or run with sudo."; exit 1
fi
if ! echo "$PROBE" | grep -qi "F4"; then
  echo "ERROR: no STM32F4 answering. Check SWDIO/SWCLK/GND/3.3V wiring and power."; exit 1
fi

echo
echo "=== Flash (write + verify + reset) ==="
st-flash --reset write "$FW" "$BL_ADDR"

echo
echo "OK: programmed at $BL_ADDR and verified."
echo "DFU: KEY+NRST (or PA8->GND+NRST with bootload-PA8). Remove PA8 jumper to run the app."
