#!/bin/bash
# Flash the flash-resident DFU bootloader at 0x08000000.
# Prefer ST-Link for first install. USB DFU only if already in DFU (ROM or old BL).
#
# Usage:
#   sudo ./flash-bootloader.sh              # uses ./bootloader.bin
#   sudo ./flash-bootloader.sh bootloader.bin

set -euo pipefail

BL_ADDR="0x08000000"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW="${1:-$SCRIPT_DIR/bootloader.bin}"

if [[ ! -f "$FW" ]]; then
  echo "ERROR: file not found: $FW"
  echo "Usage: $0 [bootloader.bin]"
  exit 1
fi

if ! command -v dfu-util >/dev/null; then
  echo "ERROR: dfu-util not installed (sudo apt install dfu-util)"
  exit 1
fi

if ! dfu-util -l 2>/dev/null | grep -qi '0483:df11'; then
  echo "ERROR: no STM32 DFU device (0483:df11)."
  echo "  First install: use ST-Link, or enter ROM DFU (BOOT0+NRST), then retry."
  echo "  Later: KEY (PA0)+NRST (flash bootloader DFU)."
  exit 1
fi

echo "Flashing bootloader: $FW  →  $BL_ADDR"
echo "WARNING: overwrites 0x08000000..32KB. App at 0x08008000 is left alone."
dfu-util -a 0 -s "${BL_ADDR}:leave" -D "$FW"
echo "Done. KEY+NRST → DFU; reset without KEY → app (if present)."
