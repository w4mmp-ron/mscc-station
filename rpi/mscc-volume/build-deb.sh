#!/bin/bash
# Build mscc-volume_*_all.deb (one Python/Tk script + menu entry).
# Run in WSL Debian or on the Pi. CR is stripped from the staged copy, so a
# CRLF checkout on Windows is fine.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PKG_SRC="$ROOT/packaging"
VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in control" >&2; exit 1; }
OUT="$ROOT/mscc-volume_${VERSION}_all.deb"

command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb not found" >&2; exit 1; }

STAGE="${TMPDIR:-/tmp}/mscc-volume-build-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -r "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"

mkdir -p "$PKG/usr/bin"
cp "$ROOT/mscc-volume.py" "$PKG/usr/bin/mscc-volume"

find "$PKG" -type f -exec sed -i 's/\r$//' {} +
find "$PKG" -type d -exec chmod 755 {} +
find "$PKG" -type f -exec chmod 644 {} +
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/usr/bin/mscc-volume"

echo "Installed-Size: $(du -sk "$PKG" | awk '{print $1}')" >>"$PKG/DEBIAN/control"

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo "OK: $OUT"
echo "Install on the Pi:  sudo apt install -y ./mscc-volume_${VERSION}_all.deb"
