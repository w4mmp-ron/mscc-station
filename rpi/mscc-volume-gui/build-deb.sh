#!/bin/bash
# Build mscc-volume-gui_*.deb (Architecture: all — Python/Tk)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PKG_SRC="$ROOT/packaging"
VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in control" >&2; exit 1; }
OUT="$ROOT/mscc-volume-gui_${VERSION}_all.deb"

command -v dpkg-deb >/dev/null || {
  echo "ERROR: dpkg-deb not found" >&2
  exit 1
}

STAGE="${TMPDIR:-/tmp}/mscc-volume-gui-build-$$"
echo "=== mscc-volume-gui deb builder ==="
echo "  version: $VERSION"
echo "  out:     $OUT"
echo "  stage:   $STAGE"

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"

# Python package + helpers under /usr/share/mscc-volume-gui
mkdir -p "$PKG/usr/share/mscc-volume-gui"
cp -a "$ROOT/mscc_volume_gui" "$PKG/usr/share/mscc-volume-gui/"
cp -a "$ROOT/mscc-volume-gui" "$PKG/usr/share/mscc-volume-gui/mscc-volume-gui"
cp -a "$ROOT/mscc-volume-restore" "$PKG/usr/share/mscc-volume-gui/mscc-volume-restore"
cp -a "$ROOT/README.md" "$PKG/usr/share/mscc-volume-gui/" 2>/dev/null || true

# PATH launchers
mkdir -p "$PKG/usr/bin"
cat >"$PKG/usr/bin/mscc-volume-gui" <<'EOF'
#!/usr/bin/env python3
import sys
sys.path.insert(0, "/usr/share/mscc-volume-gui")
from mscc_volume_gui.app import main
if __name__ == "__main__":
    main()
EOF

cat >"$PKG/usr/bin/mscc-volume-restore" <<'EOF'
#!/usr/bin/env python3
import sys
sys.path.insert(0, "/usr/share/mscc-volume-gui")
from mscc_volume_gui.restore import main
if __name__ == "__main__":
    main()
EOF

chmod 755 "$PKG/usr/bin/mscc-volume-gui"
chmod 755 "$PKG/usr/bin/mscc-volume-restore"
chmod 755 "$PKG/usr/share/mscc-volume-gui/mscc-volume-gui"
chmod 755 "$PKG/usr/share/mscc-volume-gui/mscc-volume-restore"
chmod 755 "$PKG/DEBIAN/postinst"
chmod 644 "$PKG/DEBIAN/control"
chmod 644 "$PKG/usr/share/applications/mscc-volume-gui.desktop"

mkdir -p "$PKG/usr/share/doc/mscc-volume-gui"
cp -a "$ROOT/README.md" "$PKG/usr/share/doc/mscc-volume-gui/" 2>/dev/null || true

find "$PKG" -type d -exec chmod 755 {} \;

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control" 2>/dev/null \
    || sed -i '' "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
else
  echo "Installed-Size: $SIZE_KB" >>"$PKG/DEBIAN/control"
fi

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo
echo "OK: $OUT"
ls -la "$OUT"
echo
echo "Install on Pi:"
echo "  sudo apt install -y ./mscc-volume-gui_${VERSION}_all.deb"
echo "  # menu: MSCC Volume   or:  mscc-volume-gui"
echo "  # also: \$HOME/mscc/mscc-volume-gui/"
