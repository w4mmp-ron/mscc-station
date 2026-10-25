#!/bin/bash
# Flash the Proficio app at 0x08008000 over USB DFU (no ST-Link, pill stays in the rig).
# Usage: sudo ./stm32-app-flash.sh <app-firmware.bin>
#
# 1.0.4 (2026-10-23): puts the pill into DFU itself (USB command 0xFE, as the Windows
# flasher) when the radio is running; refuses while mscc is running; waits for the radio
# to come back and prints its firmware version. A BOOT jumper (PA8) left on is reported.
# 1.0.5: no default image (the package carries none, Ron: users would flash an old one).

set -euo pipefail
APP_ADDR="0x08008000"
RADIO_ID="16c0:05dc"
DFU_ID="0483:df11"

[[ $# -eq 1 ]] || {
  echo "Usage: sudo stm32-app-flash <app-firmware.bin>"
  echo "  App image for 0x08008000, e.g. ~/app-firmware-async.bin (from the release folder)."
  exit 1
}
FW="$1"
[[ -f "$FW" ]] || { echo "ERROR: missing $FW"; exit 1; }
echo "Image: $FW  ($(stat -c %s "$FW") bytes, $(date -r "$FW" '+%Y-%m-%d %H:%M'))"
command -v dfu-util >/dev/null || { echo "ERROR: install dfu-util"; exit 1; }

if pgrep -x ms-sdr >/dev/null || pgrep -x sdrcore-recv >/dev/null || pgrep -x sdrcore-trans >/dev/null; then
  echo "ERROR: mscc is running. Run 'mscc stop' first, then this again."
  exit 1
fi

have_dfu()   { dfu-util -l 2>/dev/null | grep -qi "$DFU_ID"; }
have_radio() { lsusb 2>/dev/null | grep -qi "$RADIO_ID"; }

# Radio app: CMD_GET_VERSION (0x00), 2 bytes [major, minor]. Prints e.g. 3.10
radio_version() {
  python3 - <<'PY' 2>/dev/null || echo "unknown"
import usb.core
d = usb.core.find(idVendor=0x16C0, idProduct=0x05DC)
v = d.ctrl_transfer(0xC0, 0x00, 0, 0, 2, 500)
print(f"{v[0]}.{v[1]}")
PY
}

if have_dfu; then
  echo "Pill already in flash mode (BOOT jumper on?). Flashing anyway."
  JUMPER_SUSPECT=1
elif have_radio; then
  echo "Radio running firmware $(radio_version). Sending it to flash mode..."
  python3 - <<'PY' || true
import usb.core
d = usb.core.find(idVendor=0x16C0, idProduct=0x05DC)
try:
    d.ctrl_transfer(0x40, 0xFE, 0, 0, b"", 500)
except usb.core.USBError:
    pass    # the pill may reset before it answers
PY
  for _ in $(seq 1 20); do
    have_dfu && break
    sleep 0.5
  done
  have_dfu || { echo "ERROR: pill did not enter flash mode (no $DFU_ID after 10 s)."; exit 1; }
  JUMPER_SUSPECT=0
else
  echo "ERROR: no radio ($RADIO_ID) and no flash-mode pill ($DFU_ID) on USB."
  echo "  Is the rig powered and the USB cable in?"
  exit 1
fi

echo "Flashing app: $FW -> $APP_ADDR"
LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT
# dfu-util often ends with "Error during download get_status" after ':leave' because the
# pill resets at once; that is normal, so judge by "File downloaded successfully".
dfu-util -a 0 -s "${APP_ADDR}:leave" -D "$FW" 2>&1 | tee "$LOG" || true
grep -q "File downloaded successfully" "$LOG" || { echo "ERROR: flash FAILED (see above)."; exit 1; }

echo
echo "Flashed OK. Waiting for the radio to start..."
for _ in $(seq 1 20); do
  have_radio && break
  sleep 0.5
done
if have_radio; then
  sleep 1
  echo "Done: radio running firmware $(radio_version)."
elif have_dfu; then
  echo "Flashed OK, but the pill is still in the bootloader (LED B):"
  echo "  remove the BOOT jumper (PA8) and press NRST (reset). No power cycle needed."
  [[ "${JUMPER_SUSPECT:-0}" == 1 ]] || echo "  (or KEY is held / stuck)"
else
  echo "Radio not seen on USB after 10 s. Check the LED letter."
fi
