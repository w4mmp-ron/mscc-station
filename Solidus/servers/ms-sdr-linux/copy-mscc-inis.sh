#!/bin/bash
# Copy Windows MSCC-NET9 init files into Linux $HOME if not already present.
set -e
SRC="/mnt/c/Users/Ron/AppData/Local/MSCC-NET9"
DST="${HOME:-/home/ron}"

if [ ! -d "$SRC" ]; then
  echo "ERROR: Windows source not found: $SRC" >&2
  exit 1
fi

copy_if_missing() {
  local name="$1"
  local dest="${2:-$DST/$name}"
  if [ -f "$dest" ]; then
    echo "SKIP (exists): $dest"
    return 0
  fi
  if [ -f "$SRC/$name" ]; then
    cp -v "$SRC/$name" "$dest"
  else
    echo "MISSING SRC: $name"
  fi
}

# Server-side inis used by ms-sdr / sdrcore-recv / sdrcore-trans
copy_if_missing amplifier_cal.ini
copy_if_missing amp-cal-status.ini
copy_if_missing freq_cal.ini
copy_if_missing i2c.ini
copy_if_missing iq.ini
copy_if_missing startup.ini
copy_if_missing digital-speaker.ini
copy_if_missing operator-speaker.ini
copy_if_missing digital-microphone.ini
copy_if_missing operator-microphone.ini
copy_if_missing Multus_mscc.ini
copy_if_missing mfc_controller.ini
copy_if_missing sdrcore_recv.ini

# Already present on Linux — do not overwrite:
#   amplifier.ini, comm-port.ini, cw.ini, mscc.ini, power.ini, power_cal.ini,
#   recv-iq.ini, user_controls.ini
#
# Skipped (Windows client / path-specific):
#   log_file_dir.ini (Windows path), MSCC_Client.ini, MSCC_Favorites.ini,
#   MSCC_LastUsed*.ini, client-settings.ini, mscc-rpi.ini, logs/*

# VFO last-used layout used by last_used.c
mkdir -p "$DST/vfoa" "$DST/vfob"
if [ ! -f "$DST/vfoa/Last_used.ini" ] && [ -f "$SRC/Last_used.ini" ]; then
  cp -v "$SRC/Last_used.ini" "$DST/vfoa/Last_used.ini"
fi
if [ ! -f "$DST/vfob/Last_used.ini" ] && [ -f "$SRC/Last_used.ini" ]; then
  cp -v "$SRC/Last_used.ini" "$DST/vfob/Last_used.ini"
fi

# Default IQ templates for PCB rev MKII
mkdir -p "$DST/rev-MKII"
if [ ! -f "$DST/rev-MKII/recv-iq.ini" ] && [ -f "$SRC/recv-iq.ini" ]; then
  cp -v "$SRC/recv-iq.ini" "$DST/rev-MKII/recv-iq.ini"
fi
if [ ! -f "$DST/rev-MKII/iq.ini" ] && [ -f "$SRC/iq.ini" ]; then
  cp -v "$SRC/iq.ini" "$DST/rev-MKII/iq.ini"
fi

echo
echo "==== $DST inis ===="
ls -la "$DST"/*.ini 2>/dev/null || true
echo "==== vfoa / vfob / rev-MKII ===="
ls -la "$DST/vfoa" "$DST/vfob" "$DST/rev-MKII" 2>/dev/null || true
