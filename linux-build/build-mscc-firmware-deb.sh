#!/usr/bin/env bash
# Package prebuilt PSoC application firmware as mscc-firmware_*_all.deb.
# Source: radio-psoc-firmware/release/<RadioName>/*.cyacd and *.hex
# Dest:   /usr/share/mscc/firmware/<RadioName>/
# Architecture: all (Ubuntu and Pi). Does not rebuild Keil. Does not
# fold files into the mscc servers deb.
#
#   ./linux-build/build-mscc-firmware-deb.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
PKG_SRC="$ROOT/linux/mscc-firmware-deb/packaging"
OUT_DIR="$ROOT/linux/mscc-firmware-deb"
SRC="$ROOT/radio-psoc-firmware/release"

RADIOS=(
  Proficio-Legacy
  Proficio-MKII-PTT
  Proficio-MKII-ATU
  Geminus-Legacy
  Geminus-MKII
  Ultimus-Legacy
  Ultimus-MKII-ATU
  Ultimus-MKII-PTT
)

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }
OUT="$OUT_DIR/mscc-firmware_${VERSION}_all.deb"

need_file() { [[ -f "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }
need_dir() { [[ -d "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }

need_dir "$SRC"
need_file "$SRC/Proficio-MKII-PTT/Proficio-MKII-PTT.cyacd"
need_file "$PKG_SRC/DEBIAN/control"
need_file "$PKG_SRC/DEBIAN/postinst"
command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing (apt install dpkg-dev)" >&2; exit 1; }

echo "=== mscc-firmware .deb builder ==="
echo "  source:  $SRC"
echo "  version: $VERSION"
echo "  out:     $OUT"

STAGE="${TMPDIR:-/tmp}/mscc-firmware-deb-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE/pkg"
cp -a "$PKG_SRC/." "$STAGE/pkg/"
PKG="$STAGE/pkg"

chmod 755 "$PKG/DEBIAN/postinst"
chmod 644 "$PKG/DEBIAN/control"

DEST="$PKG/usr/share/mscc/firmware"
mkdir -p "$DEST"
if [[ -f "$SRC/README.md" ]]; then
  install -m 644 "$SRC/README.md" "$DEST/README.md"
fi

copied=0
for radio in "${RADIOS[@]}"; do
  need_dir "$SRC/$radio"
  mkdir -p "$DEST/$radio"
  shopt -s nullglob
  files=( "$SRC/$radio"/*.cyacd "$SRC/$radio"/*.hex )
  shopt -u nullglob
  if [[ ${#files[@]} -eq 0 ]]; then
    echo "ERROR: no .cyacd/.hex in $SRC/$radio" >&2
    exit 1
  fi
  for f in "${files[@]}"; do
    install -m 644 "$f" "$DEST/$radio/"
    copied=$((copied + 1))
  done
done

if [[ "$copied" -lt 32 ]]; then
  echo "ERROR: expected 32 .cyacd+.hex files, copied $copied" >&2
  exit 1
fi

find "$PKG" -type d -exec chmod 755 {} \;

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
else
  echo "Installed-Size: $SIZE_KB" >>"$PKG/DEBIAN/control"
fi

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"

mkdir -p "$ROOT/installers/linux" "$ROOT/installers/rpi"
cp -a "$OUT" "$ROOT/installers/linux/"
cp -a "$OUT" "$ROOT/installers/rpi/"

echo
echo "OK: $OUT"
ls -lh "$OUT"
echo "  copied $copied artifacts (8 radios)"
echo "  drop: installers/linux/$(basename "$OUT")"
echo "  drop: installers/rpi/$(basename "$OUT")"
echo
dpkg-deb -I "$OUT" | sed -n '1,20p'
