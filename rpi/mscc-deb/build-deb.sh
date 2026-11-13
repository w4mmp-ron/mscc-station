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
FACTORY_SRC="$TREE/../factory"
TTY_SRC="$TREE/tty0tty-master/module"
# Black Pill flash scripts (were the proficio-flash-tools package until 1.0.5)
FLASH_SRC="$TREE/../firmware-programmers/rpi/stm32-flash"

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
need_dir "$FACTORY_SRC"
need_file "$FACTORY_SRC/iq/proficio-mkii/iq.ini"
need_dir "$TTY_SRC"
need_file "$BIN_SRC/ms-sdr"
need_file "$BIN_SRC/mscc.sh"
need_file "$BIN_SRC/sdrcore-trans"
need_file "$BIN_SRC/sdrcore-recv"
need_file "$BIN_SRC/psoc-bootloader"
need_file "$TTY_SRC/tty0tty.c"
need_file "$TTY_SRC/Makefile"
need_file "$TTY_SRC/99-tty0tty.rules"
need_file "$FLASH_SRC/stm32-app-flash.sh"
need_file "$FLASH_SRC/stm32-bootloader-flash.sh"
need_file "$FLASH_SRC/README.md"
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

rm -rf "$PKG/usr/share/mscc/binaries" "$PKG/usr/share/mscc/init-files" \
       "$PKG/usr/share/mscc/factory" "$PKG/usr/share/mscc/tty0tty"
mkdir -p "$PKG/usr/share/mscc/binaries"
mkdir -p "$PKG/usr/share/mscc/init-files"
mkdir -p "$PKG/usr/share/mscc/factory"
mkdir -p "$PKG/usr/share/mscc/tty0tty/module"
mkdir -p "$PKG/usr/share/mscc/udev"
mkdir -p "$PKG/usr/share/mscc/bin"
mkdir -p "$PKG/usr/share/mscc/systemd/user"
mkdir -p "$PKG/usr/share/applications"

cp -a "$BIN_SRC/." "$PKG/usr/share/mscc/binaries/"
# Drop legacy ALSA digi helper, old mscc-init CLI (use mscc-init-gui) and editor backups
rm -f "$PKG/usr/share/mscc/binaries/audio-setup" \
      "$PKG/usr/share/mscc/binaries/mscc-init" \
      "$PKG/usr/share/mscc/binaries/"*.bak* 2>/dev/null || true
chmod 755 "$PKG/usr/share/mscc/binaries/"* || true
if [[ -f "$PKG/usr/share/mscc/binaries/audio-setup" ]]; then
  echo "ERROR: audio-setup must not be packaged" >&2
  exit 1
fi
echo "  ms-sdr:      $(wc -c < "$PKG/usr/share/mscc/binaries/ms-sdr") bytes"
echo "  sdrcore-recv:$(wc -c < "$PKG/usr/share/mscc/binaries/sdrcore-recv") bytes"
echo "  sdrcore-trans:$(wc -c < "$PKG/usr/share/mscc/binaries/sdrcore-trans") bytes"
echo "  psoc-bootloader:  $(wc -c < "$PKG/usr/share/mscc/binaries/psoc-bootloader") bytes"

# This .deb is Architecture: arm64. Refuse Ubuntu-laptop (amd64) binaries.
if command -v readelf >/dev/null 2>&1; then
  for b in ms-sdr sdrcore-recv sdrcore-trans psoc-bootloader; do
    f="$PKG/usr/share/mscc/binaries/$b"
    [[ -f "$f" ]] || { echo "ERROR: missing $f" >&2; exit 1; }
    if ! readelf -h "$f" 2>/dev/null | grep -qi 'AArch64\|ARM aarch64'; then
      echo "ERROR: $b is not AArch64 — refuse to build arm64 .deb" >&2
      file "$f" >&2
      echo "Do not copy this laptop's \$HOME/mscc binaries into mscc-binaries/." >&2
      echo "Build servers on the Pi, then copy those ELFs into mscc-binaries/." >&2
      exit 1
    fi
  done
  echo "  arch:    AArch64 OK"
fi

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
# Black Pill flash tools: stm32-app-flash (USB DFU) / stm32-bootloader-flash (ST-Link)
rm -rf "$PKG/usr/share/mscc/flash-tools"
mkdir -p "$PKG/usr/share/mscc/flash-tools" "$PKG/usr/bin"
for f in stm32-app-flash.sh stm32-bootloader-flash.sh README.md; do
  tr -d '\r' < "$FLASH_SRC/$f" > "$PKG/usr/share/mscc/flash-tools/$f"
done
chmod 755 "$PKG/usr/share/mscc/flash-tools/"*.sh
chmod 644 "$PKG/usr/share/mscc/flash-tools/README.md"
ln -sf "../share/mscc/flash-tools/stm32-app-flash.sh"   "$PKG/usr/bin/stm32-app-flash"
ln -sf "../share/mscc/flash-tools/stm32-bootloader-flash.sh" "$PKG/usr/bin/stm32-bootloader-flash"
echo "  flash-tools: stm32-app-flash, stm32-bootloader-flash"

# ---------------------------------------------------------------------------------------------
# mscc 1.0.79 (Ron 2026-11-10): one Pi package. The servers package now also carries what used to
# be the separate mscc-portaudio, mscc-init, mscc-volume and mscc-firmware packages, plus the
# Black Pill firmware. Firmware = the same files as the Windows installer (stage.ps1 fills
# mscc-ui/installer/windows/payload first). The old packages are Conflicts/Replaces in control.
WIN_PAYLOAD="$TREE/../mscc-ui/installer/windows/payload"

# PSoC radio firmware -> /usr/share/mscc/firmware/<RadioName>/ (where Firmware Upload looks)
FW_SRC="$WIN_PAYLOAD/PSoC Firmware"
need_dir "$FW_SRC"
rm -rf "$PKG/usr/share/mscc/firmware"
mkdir -p "$PKG/usr/share/mscc/firmware"
fw_count=0
FW_RADIOS=(
  Proficio-Legacy:Proficio-legacy
  Proficio-MKII-PTT:Proficio-MKII-PTT
  Proficio-MKII-ATU:Proficio-MKII-ATU
  Geminus-Legacy:Geminus-legacy
  Geminus-MKII:Geminus-MKII
  Ultimus-Legacy:Ultimus-Legacy
  Ultimus-MKII-ATU:Ultimus-MKII-ATU
  Ultimus-MKII-PTT:Ultimus-MKII-PTT
)
for entry in "${FW_RADIOS[@]}"; do
  radio="${entry%%:*}"
  winfolder="${entry##*:}"
  need_dir "$FW_SRC/$winfolder"
  shopt -s nullglob
  cy=( "$FW_SRC/$winfolder"/*-[0-9]*.[0-9]*.cyacd )
  hx=( "$FW_SRC/$winfolder"/*-[0-9]*.[0-9]*.hex )
  shopt -u nullglob
  if [[ ${#cy[@]} -ne 1 || ${#hx[@]} -ne 1 ]]; then
    echo "ERROR: firmware $radio needs one versioned .cyacd and one .hex in $winfolder (got ${#cy[@]}/${#hx[@]})" >&2
    exit 1
  fi
  [[ "$(basename "${cy[0]}" .cyacd)" == "$(basename "${hx[0]}" .hex)" ]] || {
    echo "ERROR: firmware $radio cyacd/hex version mismatch" >&2
    exit 1
  }
  mkdir -p "$PKG/usr/share/mscc/firmware/$radio"
  install -m 644 "${cy[0]}" "${hx[0]}" "$PKG/usr/share/mscc/firmware/$radio/"
  echo "  firmware:    $radio $(basename "${cy[0]}" .cyacd)"
  fw_count=$((fw_count + 2))
done
[[ "$fw_count" -eq 16 ]] || { echo "ERROR: expected 16 firmware files, got $fw_count" >&2; exit 1; }

# Black Pill (STM32F411, 25 MHz board) firmware -> /usr/share/mscc/firmware/black-pill/
BP_SRC="$WIN_PAYLOAD/Black Pill Firmware"
need_dir "$BP_SRC"
mkdir -p "$PKG/usr/share/mscc/firmware/black-pill"
# stage.ps1 puts exactly one versioned set in the payload: BlackPill-25MHz-<ver>-app.bin + -boot-app.bin
bp_app=()
shopt -s nullglob
for f in "$BP_SRC"/BlackPill-25MHz-*-app.bin; do
  [[ "$f" == *-boot-app.bin ]] || bp_app+=("$f")
done
shopt -u nullglob
[[ ${#bp_app[@]} -eq 1 ]] || { echo "ERROR: expected one BlackPill-25MHz-<ver>-app.bin in $BP_SRC, got ${#bp_app[@]} (run stage.ps1)" >&2; exit 1; }
bp_boot="${bp_app[0]%-app.bin}-boot-app.bin"
for f in "${bp_app[0]}" "$bp_boot" "$BP_SRC/bootload-PA8.bin"; do
  need_file "$f"
  install -m 644 "$f" "$PKG/usr/share/mscc/firmware/black-pill/"
done
echo "  firmware:    black-pill $(basename "${bp_app[0]}"), $(basename "$bp_boot"), bootload-PA8.bin"

# Keyer (PIC on the daughter board, 1.0.83): Keyer-MCU-app / -full-<date>.hex (stage.ps1 puts the
# newest pair in the payload) -> /usr/share/mscc/firmware/keyer/; update tool keyer-flash (command
# line) + keyer-flash.gui (window, menu "Keyer Firmware Update") from firmware-programmers/keyer-flash.
KF_FW="$WIN_PAYLOAD/Keyer Firmware"
KF_SRC="$TREE/../firmware-programmers/keyer-flash"
need_dir "$KF_FW"
mkdir -p "$PKG/usr/share/mscc/firmware/keyer"
shopt -s nullglob
kf_hex=( "$KF_FW"/Keyer-MCU-app-*.hex "$KF_FW"/Keyer-MCU-full-*.hex )
shopt -u nullglob
[[ ${#kf_hex[@]} -eq 2 ]] || { echo "ERROR: expected one Keyer-MCU-app and one Keyer-MCU-full .hex in $KF_FW, got ${#kf_hex[@]} (run stage.ps1)" >&2; exit 1; }
install -m 644 "${kf_hex[@]}" "$PKG/usr/share/mscc/firmware/keyer/"
for f in keyer-flash.py keyer-flash-gui.py; do
  need_file "$KF_SRC/$f"
  tr -d '\r' < "$KF_SRC/$f" > "$PKG/usr/share/mscc/bin/$f"
  chmod 755 "$PKG/usr/share/mscc/bin/$f"
done
need_file "$KF_SRC/keyer-flash.desktop"
tr -d '\r' < "$KF_SRC/keyer-flash.desktop" > "$PKG/usr/share/applications/mscc-keyer-flash.desktop"
chmod 644 "$PKG/usr/share/applications/mscc-keyer-flash.desktop"
grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-keyer-flash.desktop"
echo "  keyer:       $(basename "${kf_hex[0]}"), $(basename "${kf_hex[1]}"), keyer-flash, keyer-flash.gui"

# PortAudio with Pulse + ALSA (was mscc-portaudio): library, headers (servers are built on the
# Pi) and pkg-config into /usr/local. postinst registers /usr/local/lib (mscc.conf); recv / trans
# also have it as RUNPATH.
PA_DEB="$TREE/mscc-portaudio/mscc-portaudio_19.8.3_arm64.deb"
need_file "$PA_DEB"
rm -rf "$STAGE/pa"
mkdir -p "$STAGE/pa"
dpkg-deb -x "$PA_DEB" "$STAGE/pa"
need_file "$STAGE/pa/usr/local/lib/libportaudio.so.19.8"
mkdir -p "$PKG/usr/local"
cp -a "$STAGE/pa/usr/local/." "$PKG/usr/local/"
rm -rf "$STAGE/pa"
echo "  portaudio:   $(ls "$PKG/usr/local/lib" | tr '\n' ' ')"

# MSCC Init (was mscc-init): GUI + text wizard
INITGUI_SRC="$TREE/mscc-init-gui"
need_dir "$INITGUI_SRC/mscc_init_gui"
mkdir -p "$PKG/usr/share/mscc-init-gui/mscc_init_gui" "$PKG/usr/bin" "$PKG/usr/share/doc/mscc"
for f in "$INITGUI_SRC/mscc_init_gui/"*.py; do
  tr -d '\r' < "$f" > "$PKG/usr/share/mscc-init-gui/mscc_init_gui/$(basename "$f")"
done
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
tr -d '\r' < "$INITGUI_SRC/packaging/usr/share/applications/mscc-init-gui.desktop" \
  > "$PKG/usr/share/applications/mscc-init-gui.desktop"
[[ -f "$INITGUI_SRC/README-MSCC-INIT-GUI.md" ]] && \
  tr -d '\r' < "$INITGUI_SRC/README-MSCC-INIT-GUI.md" > "$PKG/usr/share/doc/mscc/README-MSCC-INIT-GUI.md"
echo "  mscc-init:   mscc-init-gui, mscc-init"

# MSCC Volume (was mscc-volume)
VOL_SRC="$TREE/mscc-volume"
need_file "$VOL_SRC/mscc-volume.py"
tr -d '\r' < "$VOL_SRC/mscc-volume.py" > "$PKG/usr/bin/mscc-volume"
tr -d '\r' < "$VOL_SRC/packaging/usr/share/applications/mscc-volume.desktop" \
  > "$PKG/usr/share/applications/mscc-volume.desktop"
chmod 755 "$PKG/usr/bin/mscc-init-gui" "$PKG/usr/bin/mscc-init" "$PKG/usr/bin/mscc-volume"
find "$PKG/usr/share/mscc-init-gui" -type d -exec chmod 755 {} \;
find "$PKG/usr/share/mscc-init-gui" -type f -exec chmod 644 {} \;
echo "  mscc-volume: mscc-volume"

if [[ -f "$ROOT/packaging/usr/share/mscc/bin/psoc-bootloader.gui" ]]; then
  cp -a "$ROOT/packaging/usr/share/mscc/bin/psoc-bootloader.gui" "$PKG/usr/share/mscc/bin/"
  chmod 755 "$PKG/usr/share/mscc/bin/psoc-bootloader.gui"
else
  echo "ERROR: missing psoc-bootloader.gui" >&2
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
grep -q 'X-MSCC' "$PKG/usr/share/applications/mscc-bootloader.desktop"
need_file "$ROOT/packaging/usr/share/mscc/bin/psoc-bootloader.gui"
need_file "$ROOT/packaging/usr/share/applications/mscc-bootloader.desktop"
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

echo "Staging factory templates…"
cp -a "$FACTORY_SRC/." "$PKG/usr/share/mscc/factory/"

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

# tty0tty kernel hooks (1.0.74): rebuild /dev/tnt* support after a kernel update
need_file "$ROOT/packaging/usr/share/mscc/bin/mscc-tty0tty-kernel"
need_file "$ROOT/packaging/etc/kernel/postinst.d/mscc-tty0tty"
need_file "$ROOT/packaging/etc/kernel/header_postinst.d/mscc-tty0tty"
for f in usr/share/mscc/bin/mscc-tty0tty-kernel etc/kernel/postinst.d/mscc-tty0tty \
         etc/kernel/header_postinst.d/mscc-tty0tty; do
  mkdir -p "$(dirname "$PKG/$f")"
  tr -d '\r' < "$ROOT/packaging/$f" > "$PKG/$f"
  chmod 755 "$PKG/$f"
done
echo "  kernel hooks: tty0tty rebuilt on kernel / header install"

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
DROP="$(cd "$ROOT/../.." && pwd)/linux-build/drop-installers.sh"
if [[ -x "$DROP" ]]; then
  "$DROP" rpi
fi
echo
echo "Install on Raspberry Pi OS (64-bit, Pi 4/5):"
echo "  ./install-mscc.sh ./mscc_${VERSION}_arm64.deb"
echo "  # or: sudo apt install -y ./mscc_${VERSION}_arm64.deb"
echo "Current kit copy: installers/rpi/"
echo
echo "Then as normal user:"
echo "  systemctl --user enable --now mscc-virtual-audio   # if sinks missing"
echo "  mscc-init-gui # operator audio (MSCC Init, in this package)"
echo "  mscc start    # or desktop menu: MSCC Start / MSCC Stop"
echo "  # PortAudio (Pulse + ALSA) is in this package: /usr/local/lib"
