#!/usr/bin/env bash
# Package Avalonia UI as mscc-ui_*_arm64.deb for Raspberry Pi OS 64-bit.
# Cross-publishes on this x86_64 laptop. Does not overwrite linux-x64 publish.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
AVA="$ROOT/mscc-ui/Avalonia-Migration"
PUBLISH="$AVA/publish/linux-arm64-sc"
TEMPLATE="$AVA/packaging/mscc-ui"
RELEASE="$ROOT/mscc-ui/Release/avalonia/arm64"

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$TEMPLATE/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }

if [[ ! -f "$PUBLISH/MSCC.Avalonia" ]]; then
  echo "No linux-arm64 publish yet — running mscc-ui-arm64.sh"
  "$HERE/mscc-ui-arm64.sh"
fi
file "$PUBLISH/MSCC.Avalonia" | grep -qiE 'ARM aarch64|ARM64|aarch64' || {
  echo "ERROR: $PUBLISH/MSCC.Avalonia is not arm64" >&2
  file "$PUBLISH/MSCC.Avalonia" >&2
  exit 1
}

command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing (apt install dpkg-dev)" >&2; exit 1; }

STAGE="${TMPDIR:-/tmp}/mscc-ui-arm64-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE/pkg"
cp -a "$TEMPLATE/." "$STAGE/pkg/"
PKG="$STAGE/pkg"

sed -i \
  -e 's/^Architecture:.*/Architecture: arm64/' \
  -e 's/Ubuntu Desktop x86_64 (linux-x64)/Raspberry Pi OS 64-bit (linux-arm64)/' \
  "$PKG/DEBIAN/control"

rm -rf "$PKG/opt/mscc-ui"
mkdir -p "$PKG/opt/mscc-ui"
cp -a "$PUBLISH"/. "$PKG/opt/mscc-ui/"
chmod 755 "$PKG/opt/mscc-ui/MSCC.Avalonia" "$PKG/usr/bin/mscc-ui"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/postrm" 2>/dev/null || true

mkdir -p "$PKG/usr/share/icons/hicolor/256x256/apps" \
         "$PKG/usr/share/icons/hicolor/48x48/apps" \
         "$PKG/usr/share/pixmaps"
if [[ -f "$TEMPLATE/icons/mscc-ui-256.png" ]]; then
  cp -a "$TEMPLATE/icons/mscc-ui-256.png" "$PKG/usr/share/icons/hicolor/256x256/apps/mscc-ui.png"
  cp -a "$TEMPLATE/icons/mscc-ui-256.png" "$PKG/usr/share/pixmaps/mscc-ui.png"
fi
if [[ -f "$TEMPLATE/icons/mscc-ui-48.png" ]]; then
  cp -a "$TEMPLATE/icons/mscc-ui-48.png" "$PKG/usr/share/icons/hicolor/48x48/apps/mscc-ui.png"
fi
rm -rf "$PKG/icons"

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
else
  echo "Installed-Size: $SIZE_KB" >> "$PKG/DEBIAN/control"
fi

OUT="$AVA/mscc-ui_${VERSION}_arm64.deb"
rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"

mkdir -p "$RELEASE"
cp -a "$OUT" "$RELEASE/"
echo "OK: $OUT"
echo "    $RELEASE/mscc-ui_${VERSION}_arm64.deb"
ls -lh "$OUT"
file "$OUT"
dpkg-deb -f "$OUT" Package Version Architecture
if [[ -x "$HERE/drop-installers.sh" ]]; then
  "$HERE/drop-installers.sh" rpi
fi
