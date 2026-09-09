#!/usr/bin/env bash
# Package Avalonia UI as mscc-ui_*_amd64.deb for Ubuntu x86_64.
# Does not touch linux-arm64 publish or mscc-ui_*_arm64.deb.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
AVA="$ROOT/mscc-ui/Avalonia-Migration"
PUBLISH="$AVA/publish/linux-x64-sc"
TEMPLATE="$AVA/packaging/mscc-ui"
RELEASE="$ROOT/mscc-ui/Release/avalonia/x86_64"

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "ERROR: amd64 UI deb is built on an x86_64 host." >&2
  exit 1
fi

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$TEMPLATE/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }

if [[ ! -x "$PUBLISH/MSCC.Avalonia" ]]; then
  echo "No linux-x64 publish yet — running mscc-ui-x64.sh"
  "$HERE/mscc-ui-x64.sh"
fi
file "$PUBLISH/MSCC.Avalonia" | grep -q 'x86-64' || {
  echo "ERROR: $PUBLISH/MSCC.Avalonia is not x86-64" >&2
  file "$PUBLISH/MSCC.Avalonia" >&2
  exit 1
}

command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing (apt install dpkg-dev)" >&2; exit 1; }

STAGE="${TMPDIR:-/tmp}/mscc-ui-amd64-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE/pkg"
cp -a "$TEMPLATE/." "$STAGE/pkg/"
PKG="$STAGE/pkg"

# Do not ship the arm64 control Architecture
sed -i \
  -e 's/^Architecture:.*/Architecture: amd64/' \
  -e 's/Raspberry Pi OS 64-bit (linux-arm64)/Ubuntu Desktop x86_64 (linux-x64)/' \
  "$PKG/DEBIAN/control"

rm -rf "$PKG/opt/mscc-ui"
mkdir -p "$PKG/opt/mscc-ui"
cp -a "$PUBLISH"/. "$PKG/opt/mscc-ui/"
chmod 755 "$PKG/opt/mscc-ui/MSCC.Avalonia" "$PKG/usr/bin/mscc-ui"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/postrm" 2>/dev/null || true

# Icons into hicolor (template has packaging/icons/)
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

OUT="$AVA/mscc-ui_${VERSION}_amd64.deb"
rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"

mkdir -p "$RELEASE"
cp -a "$OUT" "$RELEASE/"
echo "OK: $OUT"
echo "    $RELEASE/mscc-ui_${VERSION}_amd64.deb"
ls -lh "$OUT"
file "$OUT"
dpkg-deb -f "$OUT" Package Version Architecture
