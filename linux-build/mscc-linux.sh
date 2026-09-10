#!/usr/bin/env bash
# Ubuntu x86_64 (this laptop) — special audio + icon install.
#
# Ron on the Pi does NOT use this. On the Pi:  cd TREE && make
# (default PORTAUDIO=mscc, rpath /usr/local).
#
# Cross-build Pi binaries on this laptop:  ./linux-build/cross-arm64.sh
#
#   ./linux-build/mscc-linux.sh all
#
# Audio:  PORTAUDIO=distro  (Ubuntu libportaudio2, Pulse)
# Icons:  ~/.local/share/applications
# Output: $HOME/mscc   (x86_64 — never copy these into rpi/mscc-binaries/)
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
# Ubuntu x86 sources — do not edit rpi/ for laptop changes.
LINUX="$ROOT/linux"
PORTAUDIO="distro"
ICONS="user"
BINDIR="${MSCC_DIR:-$HOME/mscc}"
CLEAN=1
CMD=()

usage() {
  cat <<'EOF'
Usage: linux-build/mscc-linux.sh [options] <command...>

Ubuntu x86_64 only. Same C sources as the Pi; extra flags are for this laptop.

Commands:
  show      print audio/icon parameters
  build     compile into $HOME/mscc (x86_64)
  helpers   copy mscc.sh, status-report, virtual-audio, desktop-ctl, bootloader-gui
  icons     user desktop Start / Stop / Status / Firmware
  all       build + helpers + icons

Options:
  --portaudio distro|mscc   default: distro
  --icons user|system       default: user
  --bindir DIR              default: $HOME/mscc
  --no-clean

Pi / arm64 (Ron's tree, guide only — do not edit for Ubuntu):
  Ron:     cd rpi/SDRcore-recv-linux && make
  Cross:   ./linux-build/cross-arm64.sh
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target)
      case "${2:-}" in
        ubuntu|x86|x86_64|amd64) shift 2 ;;
        pi|arm64)
          echo "ERROR: this script is Ubuntu x86 only." >&2
          echo "  Pi (Ron):  cd TREE && make" >&2
          echo "  arm64 on this laptop:  ./linux-build/cross-arm64.sh" >&2
          exit 2
          ;;
        *) echo "Unknown --target $2" >&2; exit 2 ;;
      esac
      ;;
    --portaudio) PORTAUDIO="${2:-}"; shift 2 ;;
    --icons) ICONS="${2:-}"; shift 2 ;;
    --bindir) BINDIR="${2:-}"; shift 2 ;;
    --no-clean) CLEAN=0; shift ;;
    -h|--help) usage; exit 0 ;;
    show|build|helpers|icons|all) CMD+=("$1"); shift ;;
    package)
      echo "ERROR: x86 binaries must not go in mscc-binaries/." >&2
      echo "  For Pi ELFs on this laptop:  ./linux-build/cross-arm64.sh stage" >&2
      exit 2
      ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ${#CMD[@]} -eq 0 ]]; then
  CMD=(show)
fi

HOST="$(uname -m)"
if [[ "$HOST" != "x86_64" ]]; then
  echo "ERROR: mscc-linux.sh is for Ubuntu x86_64 (this machine is $HOST)." >&2
  echo "  On the Pi use:  make" >&2
  exit 1
fi

MAKE_PA=(PORTAUDIO="$PORTAUDIO" BINDIR="$BINDIR")
log() { echo "mscc-linux: $*"; }

cmd_show() {
  echo "host:       Ubuntu x86_64 (special path)"
  echo "cpu:        $HOST"
  echo "bindir:     $BINDIR"
  echo "audio:      PORTAUDIO=$PORTAUDIO"
  echo "icons:      $ICONS"
  echo "repo:       $ROOT"
  echo "sources:    $LINUX"
}

cmd_build() {
  mkdir -p "$BINDIR/logs"
  local t
  for t in SDRcore-recv-linux SDRcore-trans-linux ms-sdr-linux mscc-init-linux psoc-usb-bootload-linux; do
    log "build $t"
    [[ "$CLEAN" -eq 1 ]] && make -C "$LINUX/$t" clean
    case "$t" in
      psoc-usb-bootload-linux)
        make -C "$LINUX/$t"
        install -m 755 "$LINUX/$t/bootloader" "$BINDIR/bootloader"
        ;;
      ms-sdr-linux)
        make -C "$LINUX/$t" BINDIR="$BINDIR"
        ;;
      *)
        make -C "$LINUX/$t" "${MAKE_PA[@]}"
        ;;
    esac
  done
  log "built x86_64 into $BINDIR"
  file "$BINDIR/ms-sdr"
  ldd "$BINDIR/sdrcore-recv" | grep -i portaudio || true
}

cmd_helpers() {
  mkdir -p "$BINDIR"
  local src="$LINUX/helpers"
  install -m 755 "$src/mscc.sh" "$BINDIR/mscc.sh"
  install -m 755 "$src/mscc-status-report" "$BINDIR/mscc-status-report"
  install -m 755 "$src/mscc-virtual-audio.sh" "$BINDIR/mscc-virtual-audio.sh"
  install -m 755 "$src/mscc-desktop-ctl.sh" "$BINDIR/mscc-desktop-ctl.sh"
  install -m 755 "$src/bootloader-gui" "$BINDIR/bootloader-gui"
  cat > "$BINDIR/mscc-status-window.sh" << EOF
#!/bin/bash
set +e
"$BINDIR/mscc-desktop-ctl.sh" status
echo
read -r -p "Press Enter to close… "
EOF
  chmod 755 "$BINDIR/mscc-status-window.sh"
  log "helpers in $BINDIR"
}

install_icons_user() {
  local dest="$HOME/.local/share/applications"
  mkdir -p "$dest" "$HOME/.local/share/desktop-directories" \
           "$HOME/.config/menus/applications-merged"
  local f
  for f in mscc-start mscc-stop mscc-status mscc-bootloader; do
    sed "s|@BINDIR@|$BINDIR|g" "$HERE/desktop/$f.desktop.in" > "$dest/$f.desktop"
  done
  cat > "$HOME/.local/share/desktop-directories/mscc.directory" << 'EOF'
[Desktop Entry]
Type=Directory
Name=MSCC
Comment=Multus / Proficio MSCC controls
Icon=applications-other
Encoding=UTF-8
EOF
  cat > "$HOME/.config/menus/applications-merged/mscc.menu" << 'EOF'
<!DOCTYPE Menu PUBLIC "-//freedesktop//DTD Menu 1.0//EN"
  "http://www.freedesktop.org/standards/menu-spec/menu-1.0.dtd">
<Menu>
  <Name>Applications</Name>
  <Menu>
    <Name>MSCC</Name>
    <Directory>mscc.directory</Directory>
    <Include>
      <And>
        <Category>X-MSCC</Category>
      </And>
    </Include>
  </Menu>
</Menu>
EOF
  command -v update-desktop-database >/dev/null && \
    update-desktop-database "$dest" 2>/dev/null || true
  log "icons (user) → $dest"
}

cmd_icons() {
  cmd_helpers
  case "$ICONS" in
    user) install_icons_user ;;
    system)
      echo "ERROR: system icons are the Pi .deb layout. This laptop uses --icons user." >&2
      exit 2
      ;;
    *) echo "ERROR: --icons user" >&2; exit 2 ;;
  esac
}

for c in "${CMD[@]}"; do
  case "$c" in
    show) cmd_show ;;
    build) cmd_build ;;
    helpers) cmd_helpers ;;
    icons) cmd_icons ;;
    all) cmd_show; cmd_build; cmd_helpers; cmd_icons ;;
  esac
done
