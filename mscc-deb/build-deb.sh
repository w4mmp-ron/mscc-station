#!/bin/bash
# Build the mscc arm64 .deb (Raspberry Pi OS / any host with dpkg-deb).
# Run from this directory:
#   chmod +x build-deb.sh
#   ./build-deb.sh
#
# Version comes from packaging/DEBIAN/control.
# Produces: ./mscc_<version>_arm64.deb
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TREE="$(cd "$ROOT/.." && pwd)"   # worktrees/
PKG="$ROOT/packaging"

BIN_SRC="$TREE/mscc-binaries"
INIT_SRC="$TREE/mscc-init-files-linux"
TTY_SRC="$TREE/tty0tty-master/module"

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in $PKG/DEBIAN/control" >&2; exit 1; }
OUT="$ROOT/mscc_${VERSION}_arm64.deb"

echo "=== MSCC .deb builder ==="
echo "  tree:    $TREE"
echo "  version: $VERSION"
echo "  out:     $OUT"

need_dir() {
  [[ -d "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }
}
need_file() {
  [[ -f "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }
}

need_dir "$BIN_SRC"
need_dir "$INIT_SRC"
need_dir "$TTY_SRC"
need_file "$BIN_SRC/ms-sdr"
need_file "$BIN_SRC/mscc.sh"
need_file "$BIN_SRC/sdrcore-trans"
need_file "$BIN_SRC/sdrcore-recv"
need_file "$BIN_SRC/mscc-init"
need_file "$BIN_SRC/bootloader"
need_file "$TTY_SRC/tty0tty.c"
need_file "$TTY_SRC/Makefile"
need_file "$TTY_SRC/99-tty0tty.rules"
need_file "$PKG/DEBIAN/control"
need_file "$PKG/DEBIAN/postinst"

command -v dpkg-deb >/dev/null || {
  echo "ERROR: dpkg-deb not found. On Pi: sudo apt-get install -y dpkg-dev"
  exit 1
}

# Stage under /tmp so dpkg-deb sees real Unix perms (Windows mounts often force 777)
STAGE="${TMPDIR:-/tmp}/mscc-deb-build-$$"
echo "Staging under $STAGE …"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG" "$STAGE/packaging"
PKG="$STAGE/packaging"

rm -rf "$PKG/usr/share/mscc/binaries" "$PKG/usr/share/mscc/init-files" "$PKG/usr/share/mscc/tty0tty"
mkdir -p "$PKG/usr/share/mscc/binaries"
mkdir -p "$PKG/usr/share/mscc/init-files"
mkdir -p "$PKG/usr/share/mscc/tty0tty/module"
mkdir -p "$PKG/usr/share/mscc/udev"
mkdir -p "$PKG/usr/share/mscc/bin"
mkdir -p "$PKG/usr/share/mscc/systemd/user"
mkdir -p "$PKG/usr/share/applications"

cp -a "$BIN_SRC/." "$PKG/usr/share/mscc/binaries/"
# Drop legacy ALSA digi helper and editor backups — digi is Pulse VirtualA/B only
rm -f "$PKG/usr/share/mscc/binaries/audio-setup" \
      "$PKG/usr/share/mscc/binaries/"*.bak* 2>/dev/null || true
chmod 755 "$PKG/usr/share/mscc/binaries/"* || true
if [[ -f "$PKG/usr/share/mscc/binaries/audio-setup" ]]; then
  echo "ERROR: audio-setup must not be packaged" >&2
  exit 1
fi
echo "  ms-sdr:      $(wc -c < "$PKG/usr/share/mscc/binaries/ms-sdr") bytes"
echo "  sdrcore-recv:$(wc -c < "$PKG/usr/share/mscc/binaries/sdrcore-recv") bytes"
echo "  sdrcore-trans:$(wc -c < "$PKG/usr/share/mscc/binaries/sdrcore-trans") bytes"
echo "  bootloader:  $(wc -c < "$PKG/usr/share/mscc/binaries/bootloader") bytes"

# Re-copy virtual-audio assets from packaging source (stage wiped partial trees carefully)
if [[ -f "$ROOT/packaging/usr/share/mscc/bin/mscc-virtual-audio.sh" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/bin/mscc-virtual-audio.sh" "$PKG/usr/share/mscc/bin/"
  chmod 755 "$PKG/usr/share/mscc/bin/mscc-virtual-audio.sh"
else
  echo "ERROR: missing mscc-virtual-audio.sh" >&2
  exit 1
fi
if [[ -f "$ROOT/packaging/usr/share/mscc/bin/mscc-desktop-ctl.sh" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/bin/mscc-desktop-ctl.sh" "$PKG/usr/share/mscc/bin/"
  chmod 755 "$PKG/usr/share/mscc/bin/mscc-desktop-ctl.sh"
else
  echo "ERROR: missing mscc-desktop-ctl.sh" >&2
  exit 1
fi
if [[ -f "$ROOT/packaging/usr/share/mscc/bin/mscc-status-report" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/bin/mscc-status-report" "$PKG/usr/share/mscc/bin/"
  chmod 755 "$PKG/usr/share/mscc/bin/mscc-status-report"
else
  echo "ERROR: missing mscc-status-report" >&2
  exit 1
fi
if [[ -d "$ROOT/packaging/usr/share/applications" ]]; then
  cp -a "$ROOT/packaging/usr/share/applications/." "$PKG/usr/share/applications/"
  chmod 644 "$PKG/usr/share/applications/"*.desktop 2>/dev/null || true
fi
# Custom desktop menu category "MSCC" (X-MSCC)
mkdir -p "$PKG/usr/share/desktop-directories"
mkdir -p "$PKG/etc/xdg/menus/applications-merged"
if [[ -f "$ROOT/packaging/usr/share/desktop-directories/mscc.directory" ]]; then
  cp -a "$ROOT/packaging/usr/share/desktop-directories/mscc.directory" \
        "$PKG/usr/share/desktop-directories/"
  chmod 644 "$PKG/usr/share/desktop-directories/mscc.directory"
else
  echo "ERROR: missing mscc.directory" >&2
  exit 1
fi
if [[ -f "$ROOT/packaging/etc/xdg/menus/applications-merged/mscc.menu" ]]; then
  cp -a "$ROOT/packaging/etc/xdg/menus/applications-merged/mscc.menu" \
        "$PKG/etc/xdg/menus/applications-merged/"
  chmod 644 "$PKG/etc/xdg/menus/applications-merged/mscc.menu"
else
  echo "ERROR: missing mscc.menu" >&2
  exit 1
fi
grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-start.desktop"
grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-stop.desktop"
grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-status.desktop"
grep -q 'mscc-desktop-ctl' "$PKG/usr/share/applications/mscc-start.desktop"
grep -q 'mscc-desktop-ctl status' "$PKG/usr/share/applications/mscc-status.desktop"
grep -q 'mscc-status-report' "$PKG/usr/share/mscc/binaries/mscc.sh"
grep -q 'mscc-desktop-ctl' "$PKG/DEBIAN/postinst"
if [[ -f "$ROOT/packaging/usr/share/mscc/systemd/user/mscc-virtual-audio.service" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/systemd/user/mscc-virtual-audio.service" \
        "$PKG/usr/share/mscc/systemd/user/"
  chmod 644 "$PKG/usr/share/mscc/systemd/user/mscc-virtual-audio.service"
else
  echo "ERROR: missing mscc-virtual-audio.service" >&2
  exit 1
fi
# No hard-coded user homes in packaged scripts
if grep -rE '/home/ron|/home/[a-zA-Z0-9]+/' "$PKG/usr/share/mscc/bin" "$PKG/usr/share/mscc/systemd" 2>/dev/null; then
  echo "ERROR: hard-coded home path in package scripts" >&2
  exit 1
fi
grep -q 'VirtualA' "$PKG/usr/share/mscc/bin/mscc-virtual-audio.sh"
grep -q 'mscc-virtual-audio' "$PKG/DEBIAN/postinst"

echo "Staging init files…"
cp -a "$INIT_SRC/." "$PKG/usr/share/mscc/init-files/"

echo "Staging udev…"
if [[ -d "$ROOT/packaging/usr/share/mscc/udev" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/udev/." "$PKG/usr/share/mscc/udev/" 2>/dev/null || true
fi

echo "Staging tty0tty module sources (no prebuilt .ko)…"
cp -a "$TTY_SRC/Makefile" "$TTY_SRC/tty0tty.c" "$TTY_SRC/99-tty0tty.rules" \
      "$PKG/usr/share/mscc/tty0tty/module/"
for f in dkms.conf tty0tty.conf; do
  [[ -f "$TTY_SRC/$f" ]] && cp -a "$TTY_SRC/$f" "$PKG/usr/share/mscc/tty0tty/module/" || true
done

# Permissions for maintainer scripts (must work on real Linux FS)
chmod 755 "$PKG/DEBIAN"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/prerm" "$PKG/DEBIAN/postrm"
chmod 644 "$PKG/DEBIAN/control"
find "$PKG" -type d -exec chmod 755 {} \;

# Installed-Size in KB
SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control" 2>/dev/null \
    || sed -i '' "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
fi

echo "Building package…"
rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo
echo "OK: $OUT"
ls -la "$OUT"
echo
echo "Install on Raspberry Pi OS (64-bit, Pi 4/5):"
echo "  ./install-mscc.sh ./mscc_${VERSION}_arm64.deb"
echo "  # or: sudo apt install -y ./mscc_${VERSION}_arm64.deb"
echo
echo "Then as normal user:"
echo "  systemctl --user enable --now mscc-virtual-audio   # if sinks missing"
echo "  mscc-init     # digi: VirtualA / VirtualB.monitor"
echo "  mscc start    # or desktop menu: MSCC Start / MSCC Stop"
echo "  # optional PortAudio+Pulse: \$HOME/portaudio-install (see mscc.sh)"
