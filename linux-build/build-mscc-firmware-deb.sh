#!/usr/bin/env bash
# Package prebuilt PSoC application firmware as mscc-firmware_*_all.deb.
# Source: newest <Radio>-<major>.<minor>.cyacd / .hex in each tree's
#         Release/ (Geminus trees use release/). Dated names are not required.
#         The retired radio-psoc-firmware/release/ drop folder is not used.
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
FW_ROOT="$ROOT/radio-psoc-firmware"

# dest folder under /usr/share/mscc/firmware, source tree, filename prefix.
# Same trees and prefixes as mscc-ui/installer/windows/stage.ps1.
RADIOS=(
  "Proficio-Legacy|Proficio-Ultimus-Legacy|Proficio-Legacy"
  "Proficio-MKII-PTT|Proficio-Ultimus-MKII-PTT|Proficio-MKII-PTT"
  "Proficio-MKII-ATU|Proficio-Ultimus-MKII-ATU|Proficio-MKII-ATU"
  "Geminus-Legacy|Geminus-Legacy|Geminus-Legacy"
  "Geminus-MKII|Geminus-MKII|Geminus-MKII"
  "Ultimus-Legacy|Proficio-Ultimus-Legacy|Ultimus-Legacy"
  "Ultimus-MKII-ATU|Proficio-Ultimus-MKII-ATU|Ultimus-MKII-ATU"
  "Ultimus-MKII-PTT|Proficio-Ultimus-MKII-PTT|Ultimus-MKII-PTT"
)

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }
OUT="$OUT_DIR/mscc-firmware_${VERSION}_all.deb"

need_file() { [[ -f "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }
need_dir() { [[ -d "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }

need_dir "$FW_ROOT"
need_file "$PKG_SRC/DEBIAN/control"
need_file "$PKG_SRC/DEBIAN/postinst"
need_file "$OUT_DIR/README.md"
command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing (apt install dpkg-dev)" >&2; exit 1; }

echo "=== mscc-firmware .deb builder ==="
echo "  source:  $FW_ROOT/<tree>/Release (newest <Radio>-<major>.<minor>)"
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

# Newest <prefix>-<major>.<minor>.<ext> in dir. Prints the path, or nothing.
pick_newest() {
  local dir="$1" prefix="$2" ext="$3"
  local f base ver maj min best="" best_maj=-1 best_min=-1
  shopt -s nullglob
  for f in "$dir/${prefix}-"[0-9]*.[0-9]*."$ext"; do
    base="$(basename "$f" ".$ext")"
    ver="${base#"${prefix}-"}"
    if [[ "$ver" =~ ^([0-9]+)\.([0-9]+)$ ]]; then
      maj="${BASH_REMATCH[1]}"
      min="${BASH_REMATCH[2]}"
      if (( maj > best_maj || (maj == best_maj && min > best_min) )); then
        best_maj=$maj
        best_min=$min
        best="$f"
      fi
    fi
  done
  shopt -u nullglob
  printf '%s' "$best"
}

copied=0
for entry in "${RADIOS[@]}"; do
  IFS='|' read -r radio tree prefix <<<"$entry"
  if [[ -d "$FW_ROOT/$tree/Release" ]]; then
    rel="$FW_ROOT/$tree/Release"
  elif [[ -d "$FW_ROOT/$tree/release" ]]; then
    rel="$FW_ROOT/$tree/release"
  else
    echo "ERROR: no Release or release directory for $tree" >&2
    exit 1
  fi
  cy="$(pick_newest "$rel" "$prefix" cyacd)"
  hx="$(pick_newest "$rel" "$prefix" hex)"
  [[ -n "$cy" && -n "$hx" ]] || {
    echo "ERROR: $radio has no versioned .cyacd/.hex in $rel" >&2
    exit 1
  }
  [[ "$(basename "$cy" .cyacd)" == "$(basename "$hx" .hex)" ]] || {
    echo "ERROR: $radio cyacd/hex version mismatch ($cy vs $hx)" >&2
    exit 1
  }
  mkdir -p "$DEST/$radio"
  install -m 644 "$cy" "$hx" "$DEST/$radio/"
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
echo "  copied $copied versioned artifacts (8 radios, one cyacd+hex each)"
echo "  drop: installers/linux/$(basename "$OUT")"
if [[ "${MSCC_FW_DROP_RPI:-0}" == "1" ]]; then
  echo "  drop: installers/rpi/$(basename "$OUT")"
else
  echo "  drop: installers/rpi skipped (set MSCC_FW_DROP_RPI=1 to copy)"
fi
echo
dpkg-deb -I "$OUT" | sed -n '1,20p'
