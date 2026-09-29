#!/bin/bash
# Build mscc-init_*.deb (Architecture: all — Python)
# Package was mscc-init-gui up to 1.0.17 (renamed 1.0.18; Replaces/Conflicts it).
# MSCC Init GUI + mscc-init CLI (Python, 1.0.16; volume GUI dropped in 1.0.15).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PKG_SRC="$ROOT/packaging"
VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in control" >&2; exit 1; }
OUT="$ROOT/mscc-init_${VERSION}_all.deb"

command -v dpkg-deb >/dev/null || {
  echo "ERROR: dpkg-deb not found" >&2
  exit 1
}

STAGE="${TMPDIR:-/tmp}/mscc-init-gui-build-$$"
echo "=== mscc-init deb builder ==="
echo "  version: $VERSION"
echo "  out:     $OUT"
echo "  stage:   $STAGE"

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"

mkdir -p "$PKG/usr/share/mscc-init-gui"
cp -a "$ROOT/mscc_init_gui" "$PKG/usr/share/mscc-init-gui/"

mkdir -p "$PKG/usr/bin"

cat >"$PKG/usr/bin/mscc-init-gui" <<'EOF'
#!/usr/bin/env python3
import sys
sys.path.insert(0, "/usr/share/mscc-init-gui")
from mscc_init_gui.app import main
if __name__ == "__main__":
    main()
EOF

cat >"$PKG/usr/bin/mscc-init" <<'EOF'
#!/usr/bin/env python3
import sys
sys.path.insert(0, "/usr/share/mscc-init-gui")
from mscc_init_gui.cli import main
if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nCancelled - files written so far are kept.")
        sys.exit(130)
EOF

chmod 755 "$PKG/usr/bin/mscc-init-gui" "$PKG/usr/bin/mscc-init"
chmod 755 "$PKG/DEBIAN/postinst"
chmod 644 "$PKG/DEBIAN/control"
chmod 644 "$PKG/usr/share/applications/"*.desktop

mkdir -p "$PKG/usr/share/doc/mscc-init"
cp -a "$ROOT/README-MSCC-INIT-GUI.md" "$PKG/usr/share/doc/mscc-init/" 2>/dev/null || true

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
echo "  sudo apt install -y ./mscc-init_${VERSION}_all.deb"
echo "  Menu: MSCC Init   CLI (SSH): mscc-init"
