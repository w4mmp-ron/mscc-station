#!/bin/bash
# Build the Pi mscc .deb from a Windows checkout, in WSL (Ron's PC).
#
# The package itself is made by build-deb.sh (same folder). This wrapper is only needed on
# Windows: it copies what build-deb.sh reads to /tmp (real Linux file modes), takes the CR out
# of text files (never firmware images: a CRLF postinst fails on the Pi), runs build-deb.sh
# there and copies the .deb back.
#
# On Linux (Pi, Ubuntu) just run ./build-deb.sh in this folder; no wrapper needed.
#
# Run from Windows:  wsl bash /mnt/c/Users/Ron/.grok/worktrees/rpi/mscc-deb/build-deb-from-windows.sh [outdir]
# outdir (Linux path) defaults to <repo>/installers/rpi. An existing .deb is never overwritten.
set -euo pipefail

W="$(cd "$(dirname "$0")/../.." && pwd)"     # repo root (worktrees/)
OUTDIR="${1:-$W/installers/rpi}"
S=/tmp/mscc-src

rm -rf "$S"
mkdir -p "$S/rpi/mscc-deb" "$S/firmware-programmers/rpi" "$S/mscc-ui/installer/windows/payload"
for d in mscc-binaries mscc-init-files-linux tty0tty-master mscc-portaudio mscc-init-gui mscc-volume; do
  cp -r "$W/rpi/$d" "$S/rpi/"
done
cp -r "$W/rpi/mscc-deb/packaging" "$W/rpi/mscc-deb/build-deb.sh" "$S/rpi/mscc-deb/"
cp -r "$W/factory" "$S/"
cp -r "$W/firmware-programmers/rpi/stm32-flash" "$S/firmware-programmers/rpi/"
cp -r "$W/firmware-programmers/keyer-flash" "$S/firmware-programmers/"
for d in "PSoC Firmware" "Black Pill Firmware" "Keyer Firmware"; do
  cp -r "$W/mscc-ui/installer/windows/payload/$d" "$S/mscc-ui/installer/windows/payload/"
done

# strip CR from text files only (grep -I skips binaries; .hex / .cyacd left as they are)
grep -rIl --exclude=*.hex --exclude=*.cyacd $'\r' "$S" | while read -r f; do sed -i 's/\r$//' "$f"; done || true

(cd "$S/rpi/mscc-deb" && bash ./build-deb.sh)

DEB="$(ls "$S"/rpi/mscc-deb/mscc_*_arm64.deb)"
mkdir -p "$OUTDIR"
if [[ -e "$OUTDIR/$(basename "$DEB")" ]]; then
  echo "NOT copied: $OUTDIR/$(basename "$DEB") already exists (bump Version in packaging/DEBIAN/control,"
  echo "or move the old one to installers/rpi/old). Built file: $DEB"
  exit 1
fi
cp -v "$DEB" "$OUTDIR/"
