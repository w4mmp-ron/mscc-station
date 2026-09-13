#!/bin/bash
# Flash Proficio app via the flash DFU bootloader (KEY+NRST first).
# Naming: proficio-stm32f411-25MHz-YYYY-MM-DD.bin  (linked at 0x08008000)
# Usage:  sudo ./flash-proficio.sh  proficio-stm32f411-25MHz-2026-09-12.bin

set -euo pipefail

APP_ADDR="0x08008000"

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <proficio-stm32f411-25MHz-YYYY-MM-DD.bin>"
  echo "  1) Hold KEY (PA0) + NRST, release KEY (moderate blink / DFU)"
  echo "  2) $0 proficio-stm32f411-25MHz-YYYY-MM-DD.bin"
  exit 1
fi

FW="$1"
if [[ ! -f "$FW" ]]; then
  echo "ERROR: file not found: $FW"
  exit 1
fi

if ! command -v dfu-util >/dev/null; then
  echo "ERROR: dfu-util not installed (sudo apt install dfu-util)"
  exit 1
fi

if ! dfu-util -l 2>/dev/null | grep -qi '0483:df11'; then
  echo "ERROR: no STM32 DFU device (0483:df11)."
  echo "  Hold KEY (PA0) + press NRST, release KEY, then retry."
  exit 1
fi

echo "Flashing: $FW  →  $APP_ADDR"
dfu-util -a 0 -s "${APP_ADDR}:leave" -D "$FW"
echo "Done. If needed, press NRST (no KEY) to run the app."
