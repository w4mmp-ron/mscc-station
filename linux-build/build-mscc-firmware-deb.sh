#!/usr/bin/env bash
# Package prebuilt PSoC application firmware as mscc-firmware_*_all.deb.
# Source: radio-psoc-firmware/release/<RadioName>/<Name>-YYYYMMDD.{cyacd,hex}
#         (exactly one dated pair per radio; undated copies are not packed)
# Dest:   /usr/share/mscc/firmware/<RadioName>/
# Architecture: all. Does not rebuild Keil. Does not fold files into the
# mscc servers deb. Default drop is installers/linux only.
# Set MSCC_FW_DROP_RPI=1 to also copy into installers/rpi.
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
need_file "$PKG_SRC/DEBIAN/control"
need_file "$PKG_SRC/DEBIAN/postinst"
need_file "$OUT_DIR/README.md"
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
install -m 644 "$OUT_DIR/README.md" "$DEST/README.md"

copied=0
for radio in "${RADIOS[@]}"; do
  need_dir "$SRC/$radio"
  mkdir -p "$DEST/$radio"
  shopt -s nullglob
  cy=( "$SRC/$radio"/*-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].cyacd )
  hx=( "$SRC/$radio"/*-[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].hex )
  shopt -u nullglob
  if [[ ${#cy[@]} -ne 1 || ${#hx[@]} -ne 1 ]]; then
    echo "ERROR: $radio needs exactly one dated .cyacd and one dated .hex (got ${#cy[@]}/${#hx[@]})" >&2
    exit 1
  fi
  [[ "$(basename "${cy[0]}" .cyacd)" == "$(basename "${hx[0]}" .hex)" ]] || {
    echo "ERROR: $radio cyacd/hex date mismatch" >&2
    exit 1
  }
  install -m 644 "${cy[0]}" "${hx[0]}" "$DEST/$radio/"
  copied=$((copied + 2))
done

if [[ "$copied" -ne 16 ]]; then
  echo "ERROR: expected 16 files, copied $copied" >&2
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

mkdir -p "$ROOT/installers/linux"
cp -a "$OUT" "$ROOT/installers/linux/"
if [[ "${MSCC_FW_DROP_RPI:-0}" == "1" ]]; then
  mkdir -p "$ROOT/installers/rpi"
  cp -a "$OUT" "$ROOT/installers/rpi/"
fi

echo
echo "OK: $OUT"
ls -lh "$OUT"
echo "  copied $copied dated artifacts (8 radios, one cyacd+hex each)"
echo "  drop: installers/linux/$(basename "$OUT")"
if [[ "${MSCC_FW_DROP_RPI:-0}" == "1" ]]; then
  echo "  drop: installers/rpi/$(basename "$OUT")"
else
  echo "  drop: installers/rpi skipped (set MSCC_FW_DROP_RPI=1 to copy)"
fi
echo
dpkg-deb -I "$OUT" | sed -n '1,20p'
