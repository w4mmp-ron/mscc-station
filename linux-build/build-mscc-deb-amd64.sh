#!/usr/bin/env bash
# Package Ubuntu x86_64 servers as mscc_*_amd64.deb.
# Does not touch rpi/mscc-binaries/ or the Pi arm64 .deb.
#
#   ./linux-build/mscc-linux.sh build     # if $HOME/mscc is stale
#   ./linux-build/build-mscc-deb-amd64.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
LINUX="$ROOT/linux"
PKG_SRC="$LINUX/mscc-deb/packaging"
OUT_DIR="$LINUX/mscc-deb"
BIN_SRC="${MSCC_DIR:-$HOME/mscc}"
INIT_SRC="$LINUX/mscc-init-files-linux"
TTY_SRC="$LINUX/tty0tty-master/module"
HELPERS="$LINUX/helpers"
UDEV_SRC="$LINUX/udev/99-proficio.rules"

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "ERROR: amd64 server deb is built on an x86_64 host." >&2
  exit 1
fi

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }
OUT="$OUT_DIR/mscc_${VERSION}_amd64.deb"

need_file() { [[ -f "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }
need_dir() { [[ -d "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }

need_dir "$INIT_SRC"
need_dir "$TTY_SRC"
need_dir "$HELPERS"

PA_DEB="$(ls -1 "$LINUX/mscc-portaudio"/mscc-portaudio_*_amd64.deb 2>/dev/null | sort -V | tail -1 || true)"
[[ -n "$PA_DEB" ]] || {
  echo "ERROR: no linux/mscc-portaudio/mscc-portaudio_*_amd64.deb" >&2
  echo "  Build first:  ./linux-build/build-mscc-portaudio-amd64.sh" >&2
  exit 1
}
PA_STAGE="${TMPDIR:-/tmp}/mscc-pa-amd64-link-$$"
rm -rf "$PA_STAGE"
mkdir -p "$PA_STAGE"
dpkg-deb -x "$PA_DEB" "$PA_STAGE"
echo "  portaudio deb: $PA_DEB"
"$HERE/mscc-linux.sh" --portaudio mscc \
  --pa-prefix "$PA_STAGE/usr/local" --pa-rpath /usr/local/lib \
  --bindir "$BIN_SRC" build
install -m 755 "$HELPERS/mscc.sh" "$BIN_SRC/mscc.sh"
rm -rf "$PA_STAGE"

need_dir "$BIN_SRC"
for b in ms-sdr sdrcore-recv sdrcore-trans mscc-init bootloader mscc.sh; do
  need_file "$BIN_SRC/$b"
done
need_file "$HELPERS/bootloader-gui"
need_file "$HELPERS/mscc-desktop-ctl.sh"
need_file "$HELPERS/mscc-status-report"
need_file "$HELPERS/mscc-virtual-audio.sh"
need_file "$UDEV_SRC"
need_file "$TTY_SRC/tty0tty.c"
need_file "$PKG_SRC/DEBIAN/postinst"
command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing (apt install dpkg-dev)" >&2; exit 1; }

echo "=== MSCC amd64 .deb builder ==="
echo "  binaries: $BIN_SRC"
echo "  version:  $VERSION"
echo "  out:      $OUT"

STAGE="${TMPDIR:-/tmp}/mscc-deb-amd64-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"

rm -rf "$PKG/usr/share/mscc/binaries" "$PKG/usr/share/mscc/init-files" "$PKG/usr/share/mscc/tty0tty"
mkdir -p "$PKG/usr/share/mscc/binaries" \
         "$PKG/usr/share/mscc/init-files" \
         "$PKG/usr/share/mscc/tty0tty/module" \
         "$PKG/usr/share/mscc/udev" \
         "$PKG/usr/share/mscc/bin"

for b in ms-sdr sdrcore-recv sdrcore-trans mscc-init bootloader mscc.sh; do
  cp -a "$BIN_SRC/$b" "$PKG/usr/share/mscc/binaries/$b"
done
chmod 755 "$PKG/usr/share/mscc/binaries/"*

# Refuse AArch64 / wrong CPU
if command -v readelf >/dev/null 2>&1; then
  for b in ms-sdr sdrcore-recv sdrcore-trans mscc-init bootloader; do
    f="$PKG/usr/share/mscc/binaries/$b"
    if ! readelf -h "$f" 2>/dev/null | grep -qi 'X86-64\|x86-64'; then
      echo "ERROR: $b is not x86-64 — refuse to build amd64 .deb" >&2
      file "$f" >&2
      echo "Build with ./linux-build/mscc-linux.sh build (never copy Pi ELFs here)." >&2
      exit 1
    fi
  done
  echo "  arch:    x86-64 OK"
fi

# Must rpath /usr/local (mscc-portaudio). ldd may still show distro until that .deb is installed.
if command -v readelf >/dev/null 2>&1; then
  rpath="$(readelf -d "$PKG/usr/share/mscc/binaries/sdrcore-recv" | grep -E 'RPATH|RUNPATH' || true)"
  echo "  rpath: $rpath"
  if ! echo "$rpath" | grep -q '/usr/local/lib'; then
    echo "ERROR: sdrcore-recv is not rpath'd to /usr/local/lib." >&2
    echo "  Build with: ./linux-build/mscc-linux.sh --portaudio mscc build" >&2
    exit 1
  fi
fi

cp -a "$HELPERS/bootloader-gui" "$PKG/usr/share/mscc/bin/"
cp -a "$HELPERS/mscc-desktop-ctl.sh" "$PKG/usr/share/mscc/bin/"
cp -a "$HELPERS/mscc-status-report" "$PKG/usr/share/mscc/bin/"
cp -a "$HELPERS/mscc-virtual-audio.sh" "$PKG/usr/share/mscc/bin/"
chmod 755 "$PKG/usr/share/mscc/bin/"*

cp -a "$INIT_SRC/." "$PKG/usr/share/mscc/init-files/"
cp -a "$UDEV_SRC" "$PKG/usr/share/mscc/udev/99-proficio.rules"
cp -a "$TTY_SRC/Makefile" "$TTY_SRC/tty0tty.c" "$TTY_SRC/99-tty0tty.rules" \
  "$PKG/usr/share/mscc/tty0tty/module/"
for f in dkms.conf tty0tty.conf; do
  [[ -f "$TTY_SRC/$f" ]] && cp -a "$TTY_SRC/$f" "$PKG/usr/share/mscc/tty0tty/module/" || true
done

grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-start.desktop"
grep -q 'mscc-status-window' "$PKG/usr/share/applications/mscc-status.desktop"
grep -q 'mscc-status-report' "$PKG/usr/share/mscc/binaries/mscc.sh"
if grep -rE '/home/stew|/home/ron|/home/[a-zA-Z0-9]+/' "$PKG/usr/share/mscc/bin" "$PKG/usr/share/mscc/systemd" 2>/dev/null; then
  echo "ERROR: hard-coded home path in package scripts" >&2
  exit 1
fi

chmod 755 "$PKG/DEBIAN"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/prerm" "$PKG/DEBIAN/postrm"
chmod 644 "$PKG/DEBIAN/control"
find "$PKG" -type d -exec chmod 755 {} \;

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
fi

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo "OK: $OUT"
ls -lh "$OUT"
file "$OUT"

# Current kit for GitHub web
if [[ -x "$HERE/drop-installers.sh" ]]; then
  "$HERE/drop-installers.sh" linux
fi
