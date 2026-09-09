#!/usr/bin/env bash
# Cross-compile Pi (AArch64) servers on an amd64 Ubuntu laptop.
#
# Ron on the Pi does not use this. On the Pi he still runs:
#   cd SDRcore-recv-linux && make clean && make
#   (same for trans / ms-sdr)
#
# This writes $HOME/mscc-arm64  — it does NOT replace $HOME/mscc (x86 station).
# Linked like the Pi: PORTAUDIO=mscc, rpath /usr/local (mscc-portaudio).
#
#   ./linux-build/cross-arm64.sh prereqs   # print/install cross packages
#   ./linux-build/cross-arm64.sh build
#   ./linux-build/cross-arm64.sh stage     # copy AArch64 ELFs → mscc-binaries/
#   ./linux-build/cross-arm64.sh deb       # stage + mscc-deb/build-deb.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
BINDIR="${MSCC_ARM64_DIR:-$HOME/mscc-arm64}"
PA_STAGE="$HERE/.pa-arm64"
CLEAN=1
CMD=()

CC_X="${CROSS_CC:-aarch64-linux-gnu-gcc}"
CXX_X="${CROSS_CXX:-aarch64-linux-gnu-g++}"
AARCH_LIB="/usr/lib/aarch64-linux-gnu"

usage() {
  cat <<'EOF'
Usage: linux-build/cross-arm64.sh <command...>

Build Pi arm64 binaries on this Ubuntu x86 laptop. Does not change Ron's
plain `make` on the Pi. Does not overwrite $HOME/mscc (x86).

Commands:
  prereqs   show (and with --install, apt) cross-compiler + arm64 libs
  show      print tool/output paths
  build     compile AArch64 into $HOME/mscc-arm64
  stage     copy those ELFs into mscc-binaries/ (AArch64 check)
  deb       stage + ./mscc-deb/build-deb.sh

Then commit/push or copy the .deb to the Pi / pi-install/packages/.
EOF
}

INSTALL_PREREQS=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --install) INSTALL_PREREQS=1; shift ;;
    --bindir) BINDIR="${2:-}"; shift 2 ;;
    --no-clean) CLEAN=0; shift ;;
    -h|--help) usage; exit 0 ;;
    prereqs|show|build|stage|deb) CMD+=("$1"); shift ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done
[[ ${#CMD[@]} -eq 0 ]] && CMD=(show)

log() { echo "cross-arm64: $*"; }

find_pa_deb() {
  local f
  for f in \
    "$ROOT/pi-install/packages/mscc-portaudio_19.8.2_arm64.deb" \
    "$ROOT/mscc-portaudio/mscc-portaudio_19.8.2_arm64.deb"
  do
    [[ -f "$f" ]] && { echo "$f"; return 0; }
  done
  return 1
}

prereq_list() {
  cat <<'EOF'
sudo dpkg --add-architecture arm64
sudo apt update
sudo apt install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
  libusb-1.0-0-dev:arm64 \
  libhidapi-dev:arm64
EOF
}

cmd_prereqs() {
  echo "Cross-compile packages (Ubuntu amd64 host → arm64 ELFs):"
  echo
  prereq_list
  echo
  echo "PortAudio for the Pi is taken from mscc-portaudio_*_arm64.deb in this repo"
  echo "(link here, rpath /usr/local on the Pi — same as Ron's native build)."
  if [[ "$INSTALL_PREREQS" -eq 1 ]]; then
    log "installing via pkexec…"
    pkexec bash -c 'dpkg --add-architecture arm64 && apt-get update && apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libusb-1.0-0-dev:arm64 libhidapi-dev:arm64'
  fi
}

need_tools() {
  command -v "$CC_X" >/dev/null || {
    echo "ERROR: $CC_X not found. Run: ./linux-build/cross-arm64.sh prereqs --install" >&2
    exit 1
  }
  command -v "$CXX_X" >/dev/null || {
    echo "ERROR: $CXX_X not found." >&2
    exit 1
  }
  [[ -d "$AARCH_LIB" ]] || {
    echo "ERROR: $AARCH_LIB missing (enable arm64 multiarch). See prereqs." >&2
    exit 1
  }
  [[ -e "$AARCH_LIB/libusb-1.0.so" || -e "$AARCH_LIB/libusb-1.0.so.0" ]] || {
    echo "ERROR: libusb arm64 not installed (libusb-1.0-0-dev:arm64)." >&2
    exit 1
  }
}

extract_pa() {
  local deb
  deb="$(find_pa_deb)" || {
    echo "ERROR: no mscc-portaudio_*_arm64.deb in pi-install/packages or mscc-portaudio/" >&2
    exit 1
  }
  rm -rf "$PA_STAGE"
  mkdir -p "$PA_STAGE"
  dpkg-deb -x "$deb" "$PA_STAGE"
  [[ -e "$PA_STAGE/usr/local/lib/libportaudio.so" || -e "$PA_STAGE/usr/local/lib/libportaudio.so.2" ]] || {
    echo "ERROR: extracted PortAudio has no libportaudio under usr/local/lib" >&2
    exit 1
  }
  if command -v readelf >/dev/null && ! readelf -h "$PA_STAGE/usr/local/lib"/libportaudio.so* 2>/dev/null | grep -qi 'AArch64\|ARM aarch64'; then
    echo "ERROR: mscc-portaudio .deb is not AArch64" >&2
    exit 1
  fi
  log "PortAudio (arm64) from $deb"
}

cmd_show() {
  echo "host:       $(uname -m)  (cross → aarch64)"
  echo "CC:         $CC_X"
  echo "bindir:     $BINDIR   (not \$HOME/mscc)"
  echo "audio:      PORTAUDIO=mscc  PREFIX=extracted mscc-portaudio  RPATH=/usr/local/lib"
  echo "stage to:   $ROOT/mscc-binaries/"
  echo "pa deb:     $(find_pa_deb 2>/dev/null || echo MISSING)"
}

cmd_build() {
  need_tools
  extract_pa
  mkdir -p "$BINDIR/logs"
  local extra=(
    CC="$CC_X"
    CXX="$CXX_X"
    BINDIR="$BINDIR"
    PORTAUDIO=mscc
    PORTAUDIO_PREFIX="$PA_STAGE/usr/local"
    PORTAUDIO_RPATH=/usr/local/lib
    LDFLAGS="-L$AARCH_LIB"
  )
  local t
  for t in SDRcore-recv-linux SDRcore-trans-linux ms-sdr-linux mscc-init-linux psoc-usb-bootload-linux; do
    log "build $t (aarch64)"
    [[ "$CLEAN" -eq 1 ]] && make -C "$ROOT/$t" clean
    case "$t" in
      psoc-usb-bootload-linux)
        make -C "$ROOT/$t" CC="$CC_X" LDFLAGS="-L$AARCH_LIB"
        install -m 755 "$ROOT/$t/bootloader" "$BINDIR/bootloader"
        ;;
      ms-sdr-linux)
        make -C "$ROOT/$t" CC="$CC_X" BINDIR="$BINDIR" LDFLAGS="-L$AARCH_LIB"
        ;;
      *)
        make -C "$ROOT/$t" "${extra[@]}"
        ;;
    esac
  done
  log "AArch64 binaries in $BINDIR"
  local b
  for b in ms-sdr sdrcore-recv sdrcore-trans mscc-init bootloader; do
    file "$BINDIR/$b"
    readelf -h "$BINDIR/$b" | grep -E 'Machine:' || true
  done
}

assert_aarch64() {
  local f="$1"
  readelf -h "$f" 2>/dev/null | grep -qi 'AArch64\|ARM aarch64' || {
    echo "ERROR: $f is not AArch64" >&2
    file "$f" >&2
    exit 1
  }
}

cmd_stage() {
  local dest="$ROOT/mscc-binaries"
  mkdir -p "$dest"
  local b
  for b in ms-sdr sdrcore-recv sdrcore-trans mscc-init bootloader; do
    [[ -x "$BINDIR/$b" ]] || { echo "ERROR: missing $BINDIR/$b — run build first" >&2; exit 1; }
    assert_aarch64 "$BINDIR/$b"
    cp -a "$BINDIR/$b" "$dest/$b"
    log "staged $b → $dest/"
  done
  echo "Ready for:  cd $ROOT/mscc-deb && ./build-deb.sh"
}

cmd_deb() {
  cmd_stage
  ( cd "$ROOT/mscc-deb" && ./build-deb.sh )
}

for c in "${CMD[@]}"; do
  case "$c" in
    prereqs) cmd_prereqs ;;
    show) cmd_show ;;
    build) cmd_build ;;
    stage) cmd_stage ;;
    deb) cmd_deb ;;
  esac
done
