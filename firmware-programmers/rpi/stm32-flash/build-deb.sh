#!/bin/bash
# Build proficio-flash-tools_<ver>_all.deb (Pi / any Debian Linux).
# Packs the rpi-tools scripts only. No firmware images since 1.0.5 (Ron, 2026-10-23: users
# would flash the old image in the package): the commands take the image file as argument.
# Run in WSL Debian or on the Pi:   bash build-deb.sh [version]

set -euo pipefail
VERSION="${1:-1.0.0}"
PKG="proficio-flash-tools"
HERE="$(dirname "$(readlink -f "$0")")"
SHARE="usr/share/$PKG"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
ROOT="$STAGE/$PKG"
mkdir -p "$ROOT/DEBIAN" "$ROOT/$SHARE/rpi-tools" "$ROOT/usr/bin"

# Scripts + README (strip CR: the Windows checkout may have CRLF)
for f in flash-bootloader.sh flash-proficio.sh README.md; do
  tr -d '\r' < "$HERE/$f" > "$ROOT/$SHARE/rpi-tools/$f"
done
chmod 755 "$ROOT/$SHARE/rpi-tools/"*.sh
chmod 644 "$ROOT/$SHARE/rpi-tools/README.md"

# Commands on the PATH
ln -s "../share/$PKG/rpi-tools/flash-bootloader.sh" "$ROOT/usr/bin/proficio-flash-bootloader"
ln -s "../share/$PKG/rpi-tools/flash-proficio.sh"   "$ROOT/usr/bin/proficio-flash-app"

SIZE_KB="$(du -sk "$ROOT" | cut -f1)"
cat > "$ROOT/DEBIAN/control" <<EOF
Package: $PKG
Version: $VERSION
Architecture: all
Maintainer: Ron Patton (W4MMP) <ron_patton@protonmail.com>
Depends: stlink-tools, dfu-util, python3-usb, usbutils
Installed-Size: $SIZE_KB
Section: electronics
Priority: optional
Description: Flash tools for the Proficio MKII Black Pill (STM32F411)
 proficio-flash-bootloader <file.bin>: ST-Link (st-flash) burn at 0x08000000
 (boot-app-firmware-async.bin = bootloader + app, or bootload-PA8.bin).
 proficio-flash-app <file.bin>: USB DFU (dfu-util) app burn at 0x08008000; puts
 the pill into DFU itself (mscc must be stopped).
 No firmware images included: give the image file from the release folder.
EOF

# After install: say what was installed and where (Ron 2026-10-03)
cat > "$ROOT/DEBIAN/postinst" <<EOF
#!/bin/sh
set -e
if [ "\$1" = "configure" ]; then
  echo ""
  echo "$PKG $VERSION installed (tools only, no firmware images)."
  echo "  Commands (in /usr/bin):"
  echo "    proficio-flash-bootloader <file.bin>   ST-Link burn at 0x08000000"
  echo "    proficio-flash-app <file.bin>          USB DFU app burn (mscc stop first)"
  echo "  Give the image file, e.g. ~/app-firmware-async.bin from the release folder."
  echo "  Scripts and README in /usr/share/$PKG/rpi-tools/"
  echo ""
fi
exit 0
EOF
chmod 755 "$ROOT/DEBIAN/postinst"

OUT="$HERE/${PKG}_${VERSION}_all.deb"
fakeroot dpkg-deb --build "$ROOT" "$OUT" >/dev/null
echo "Built: $OUT"
dpkg-deb --info "$OUT" | sed -n '/Package:/,$p'
dpkg-deb --contents "$OUT"
