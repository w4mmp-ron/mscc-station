#!/usr/bin/env bash
# Build mscc_<version>_amd64.deb for Ubuntu / Debian PCs: the same package as the Pi's
# mscc_<version>_arm64.deb, with x86-64 programs.
#
#   ./linux-build/build-mscc-deb-amd64.sh
#   -> installers/linux/mscc_<version>_amd64.deb
#
# How (Stew 2026-11-13): this script does NOT keep its own copy of the packaging steps. It runs
# Ron's rpi/mscc-deb/build-deb.sh with a few substitutions (checked below; the build stops if
# one no longer matches). So the package files, version, firmware, MSCC Init / Volume, flash
# tools and postinst all come from rpi/, and a Pi package change reaches this package by itself.
#
# What is amd64-only:
#   - programs built here from linux/ (ms-sdr, sdrcore-recv, sdrcore-trans) and
#     Solidus/servers/psoc-usb-bootload-linux (psoc-bootloader), in a stage copy, so the repo is
#     untouched and ms-sdr's version.h is NOT bumped (it reports the same version as the Pi)
#   - PortAudio (Pulse + ALSA) built here from portaudio/ (linux-build/build-mscc-portaudio-amd64.sh)
#   - the control file: the Pi's, with Architecture amd64, Ubuntu kernel headers, PC wording
#
# Needs: build-essential, libusb-1.0-0-dev, libhidapi-dev, libpulse-dev, libasound2-dev, dpkg-dev,
# python3. Server scripts mscc.sh / mscc-status-report come from rpi/mscc-binaries (not CPU-specific).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
RPI="$ROOT/rpi"
PI_SCRIPT="$RPI/mscc-deb/build-deb.sh"
PI_CONTROL="$RPI/mscc-deb/packaging/DEBIAN/control"
BOOTLOADER_SRC="$ROOT/Solidus/servers/psoc-usb-bootload-linux"
OUT_DIR="$ROOT/installers/linux"

[[ "$(uname -m)" == "x86_64" ]] || { echo "ERROR: build the amd64 package on an x86_64 PC" >&2; exit 1; }
for t in dpkg-deb python3 make gcc readelf; do
  command -v "$t" >/dev/null || { echo "ERROR: $t missing" >&2; exit 1; }
done

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PI_CONTROL" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in $PI_CONTROL" >&2; exit 1; }
OUT="$OUT_DIR/mscc_${VERSION}_amd64.deb"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/mscc-deb-amd64-XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
BIN="$WORK/binaries"
mkdir -p "$BIN" "$WORK/src" "$WORK/pa" "$OUT_DIR"

echo "=== mscc amd64 .deb (from the Pi package $VERSION) ==="
echo "  work: $WORK"
echo "  out:  $OUT"

# 1) PortAudio (Pulse + ALSA) for amd64, as a .deb in the work folder (Pi: mscc-portaudio_*_arm64.deb)
echo "--- PortAudio"
MSCC_PA_OUT_DIR="$WORK/pa" MSCC_NO_DROP=1 "$HERE/build-mscc-portaudio-amd64.sh" > "$WORK/portaudio.log" 2>&1 \
  || { tail -30 "$WORK/portaudio.log" >&2; echo "ERROR: PortAudio build failed" >&2; exit 1; }
PA_DEB="$(ls -1 "$WORK/pa"/mscc-portaudio_*_amd64.deb | tail -1)"
PA_ROOT="$WORK/pa/root"
dpkg-deb -x "$PA_DEB" "$PA_ROOT"
echo "  $(basename "$PA_DEB")"

# 2) Programs, built from stage copies (no .o files or version bumps in the repo)
echo "--- servers"
for t in ms-sdr-linux SDRcore-recv-linux SDRcore-trans-linux; do
  cp -a "$ROOT/linux/$t" "$WORK/src/"
  # BINDIR on clean too: the Makefiles' clean deletes $(BINDIR)/<program>, default ~/mscc
  make -C "$WORK/src/$t" BINDIR="$BIN" clean > /dev/null 2>&1 || true
done
# ms-sdr: build the target directly, which skips bump-version (same version as the Pi build)
make -C "$WORK/src/ms-sdr-linux" BINDIR="$BIN" "$BIN/ms-sdr" > "$WORK/ms-sdr.log" 2>&1 \
  || { tail -30 "$WORK/ms-sdr.log" >&2; exit 1; }
for t in SDRcore-recv-linux SDRcore-trans-linux; do
  make -C "$WORK/src/$t" PORTAUDIO=mscc PORTAUDIO_PREFIX="$PA_ROOT/usr/local" \
       PORTAUDIO_RPATH=/usr/local/lib BINDIR="$BIN" > "$WORK/$t.log" 2>&1 \
    || { tail -30 "$WORK/$t.log" >&2; exit 1; }
done
cp -a "$BOOTLOADER_SRC" "$WORK/src/psoc-bootloader"
make -C "$WORK/src/psoc-bootloader" clean > /dev/null 2>&1 || true
make -C "$WORK/src/psoc-bootloader" > "$WORK/psoc-bootloader.log" 2>&1 \
  || { tail -30 "$WORK/psoc-bootloader.log" >&2; exit 1; }
install -m 755 "$WORK/src/psoc-bootloader/bootloader" "$BIN/psoc-bootloader"
for f in mscc.sh mscc-status-report; do
  tr -d '\r' < "$RPI/mscc-binaries/$f" > "$BIN/$f"
  chmod 755 "$BIN/$f"
done
for b in ms-sdr sdrcore-recv sdrcore-trans; do
  echo "  $b: $(grep -c 'warning:' "$WORK/$(case $b in ms-sdr) echo ms-sdr;; sdrcore-recv) echo SDRcore-recv-linux;; *) echo SDRcore-trans-linux;; esac).log") warnings"
done
for b in sdrcore-recv sdrcore-trans; do
  readelf -d "$BIN/$b" | grep -qE '(RPATH|RUNPATH).*/usr/local/lib' \
    || { echo "ERROR: $b has no rpath /usr/local/lib" >&2; exit 1; }
done
MS_VER="$(awk '$2=="VERSION_MAJOR"{a=$3} $2=="VERSION_MINOR"{b=$3} END{print a"."b}' "$ROOT/linux/ms-sdr-linux/source/version.h")"
echo "  ms-sdr version (not bumped): $MS_VER"

# Firmware: a copy of the Windows payload with .cyacd / .hex in CRLF, as in the Pi and Windows
# installers (Ron builds from a Windows checkout; git here gives LF). Byte-identical files.
PAYLOAD="$WORK/payload"
mkdir -p "$PAYLOAD"
for d in "PSoC Firmware" "Black Pill Firmware" "Keyer Firmware"; do
  cp -a "$ROOT/mscc-ui/installer/windows/payload/$d" "$PAYLOAD/"
done
find "$PAYLOAD" -type f \( -name '*.cyacd' -o -name '*.hex' \) -print0 |
  while IFS= read -r -d '' f; do
    sed -i 's/\r$//; s/$/\r/' "$f"
  done

# 3) amd64 control file from the Pi's
python3 - "$PI_CONTROL" "$WORK/control" <<'EOF'
import sys
s = open(sys.argv[1], encoding="utf-8").read().replace("\r\n", "\n")
subs = [
    ("Architecture: arm64", "Architecture: amd64"),
    ("linux-headers-rpi-v8 | linux-headers-rpi-2712", "linux-headers-generic"),
    ("Description: MSCC stack for Multus/Proficio on Raspberry Pi OS (64-bit)",
     "Description: MSCC stack for Multus/Proficio on Ubuntu / Debian PC (amd64)"),
    (" Supported platform: Raspberry Pi OS 64-bit on Pi 4 / Pi 5 only.\n Not intended for other Linux distributions.\n",
     " Supported platform: Ubuntu / Debian on x86-64 PCs. Same contents and version as\n the Raspberry Pi package mscc_<version>_arm64.deb, built for amd64.\n"),
    (" Kernel headers (linux-headers-rpi-v8 or rpi-2712) needed for tty0tty.",
     " Kernel headers (linux-headers-generic) needed for tty0tty."),
]
for a, b in subs:
    if s.count(a) != 1:
        sys.exit(f"ERROR: Pi control changed, update build-mscc-deb-amd64.sh: {a[:60]!r}")
    s = s.replace(a, b)
open(sys.argv[2], "w", encoding="utf-8").write(s)
EOF

# 4) Ron's build-deb.sh with the amd64 substitutions
python3 - "$PI_SCRIPT" "$WORK/build-deb-amd64.sh" <<'EOF'
import sys
s = open(sys.argv[1], encoding="utf-8").read().replace("\r\n", "\n")
subs = [
    ('ROOT="$(cd "$(dirname "$0")" && pwd)"', 'ROOT="$MSCC_PI_DEB_DIR"'),
    ('BIN_SRC="$TREE/mscc-binaries"', 'BIN_SRC="$MSCC_AMD64_BIN"'),
    ('OUT="$ROOT/mscc_${VERSION}_arm64.deb"', 'OUT="$MSCC_AMD64_OUT"'),
    ("grep -qi 'AArch64\\|ARM aarch64'", "grep -qi 'X86-64\\|x86-64'"),
    ('echo "  arch:    AArch64 OK"', 'echo "  arch:    x86-64 OK"'),
    # Pi PortAudio is a CMake build (libportaudio.so.19.8), amd64 an autotools build
    # (libportaudio.so.2.0.0); both have the soname libportaudio.so.2 the servers load.
    ('need_file "$STAGE/pa/usr/local/lib/libportaudio.so.19.8"',
     'need_file "$STAGE/pa/usr/local/lib/libportaudio.so.2"'),
    ('PA_DEB="$TREE/mscc-portaudio/mscc-portaudio_19.8.3_arm64.deb"', 'PA_DEB="$MSCC_AMD64_PA_DEB"'),
    ('PKG="$STAGE/packaging"\n', 'PKG="$STAGE/packaging"\ncp "$MSCC_AMD64_CONTROL" "$PKG/DEBIAN/control"\n'),
    ('if [[ -x "$DROP" ]]; then', 'if false; then'),
    ('WIN_PAYLOAD="$TREE/../mscc-ui/installer/windows/payload"', 'WIN_PAYLOAD="$MSCC_AMD64_PAYLOAD"'),
]
for a, b in subs:
    if s.count(a) != 1:
        sys.exit(f"ERROR: rpi/mscc-deb/build-deb.sh changed, update build-mscc-deb-amd64.sh: {a[:60]!r}")
    s = s.replace(a, b)
open(sys.argv[2], "w", encoding="utf-8").write(s)
EOF
chmod 755 "$WORK/build-deb-amd64.sh"

echo "--- package (Ron's build-deb.sh steps)"
MSCC_PI_DEB_DIR="$RPI/mscc-deb" MSCC_AMD64_BIN="$BIN" MSCC_AMD64_OUT="$OUT" \
MSCC_AMD64_PA_DEB="$PA_DEB" MSCC_AMD64_CONTROL="$WORK/control" MSCC_AMD64_PAYLOAD="$PAYLOAD" TMPDIR="$WORK" \
  bash "$WORK/build-deb-amd64.sh" > "$WORK/package.log" 2>&1 \
  || { tail -40 "$WORK/package.log" >&2; echo "ERROR: package build failed" >&2; exit 1; }
grep -E '^  (firmware|keyer|portaudio|mscc-init|mscc-volume|flash-tools|arch):' "$WORK/package.log" || true

[[ "$(dpkg-deb -f "$OUT" Architecture)" == "amd64" ]] || { echo "ERROR: package is not amd64" >&2; exit 1; }
echo
echo "OK: $OUT ($(du -h "$OUT" | cut -f1))"
echo "Install:  sudo apt install -y ./installers/linux/mscc_${VERSION}_amd64.deb"
