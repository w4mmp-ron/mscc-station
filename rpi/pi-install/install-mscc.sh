#!/bin/bash
# Install an mscc_*.deb on Raspberry Pi OS without the scary apt notice:
#   Notice: Download is performed unsandboxed as root as file '.../mscc_....deb'
#           couldn't be accessed by user '_apt'
#
# Cause: apt's _apt user cannot read files under a typical $HOME (mode 700/750).
# Fix:   copy the .deb to a world-readable temp path, then apt install from there.
#
# Usage (from the directory with the .deb, as your normal user):
#   chmod +x install-mscc.sh
#   ./install-mscc.sh
#   ./install-mscc.sh ./mscc_1.0.8_arm64.deb
#   ./install-mscc.sh --reinstall
#   ./install-mscc.sh --reinstall ./mscc_1.0.8_arm64.deb
#
set -euo pipefail

REINSTALL=0
DEB_ARG=""

usage() {
  echo "Usage: $0 [--reinstall] [path/to/mscc_VERSION_arm64.deb]" >&2
  exit 1
}

for a in "$@"; do
  case "$a" in
    -h|--help) usage ;;
    --reinstall|-r) REINSTALL=1 ;;
    -*)
      echo "ERROR: unknown option: $a" >&2
      usage
      ;;
    *)
      if [[ -n "$DEB_ARG" ]]; then
        echo "ERROR: extra argument: $a" >&2
        usage
      fi
      DEB_ARG="$a"
      ;;
  esac
done

pick_deb() {
  if [[ -n "${1:-}" ]]; then
    echo "$1"
    return
  fi
  local f
  f="$(ls -1 mscc_*_arm64.deb 2>/dev/null | sort -V | tail -1 || true)"
  if [[ -z "$f" ]]; then
    echo "ERROR: no mscc_*_arm64.deb found in $(pwd)" >&2
    echo "Usage: $0 [--reinstall] [path/to/mscc_VERSION_arm64.deb]" >&2
    exit 1
  fi
  echo "$f"
}

DEB="$(pick_deb "$DEB_ARG")"
if [[ ! -f "$DEB" ]]; then
  echo "ERROR: not a file: $DEB" >&2
  exit 1
fi

# Resolve absolute path
DEB="$(cd "$(dirname "$DEB")" && pwd)/$(basename "$DEB")"

if [[ "$(id -u)" -eq 0 ]]; then
  echo "ERROR: run as your normal user (not root). The script will use sudo." >&2
  exit 1
fi

STAGING="/tmp/mscc-apt-install-$$.deb"
cleanup() { rm -f "$STAGING"; }
trap cleanup EXIT

echo "mscc installer"
echo "  package:   $DEB"
echo "  staging:   $STAGING  (world-readable so apt does not warn)"
echo "  reinstall: $REINSTALL"
cp -f "$DEB" "$STAGING"
chmod 644 "$STAGING"

echo
sudo apt update
if [[ "$REINSTALL" -eq 1 ]]; then
  # Same version already installed → force postinst to run again
  sudo apt install --reinstall -y "$STAGING"
else
  sudo apt install -y "$STAGING"
fi
echo
echo "Install finished. Next steps (in order):"
echo "  1. Log out/in (or reboot) — enables groups + virtual digi audio service"
echo "  2. pactl list short sinks | grep Virtual   # if empty: mscc-virtual-audio"
echo "  3. mscc-init     # digi speaker=VirtualA  digi mic=VirtualB.monitor"
echo "  4. mscc start"
echo "Pulse PortAudio: install to \$HOME/portaudio-install (mscc.sh uses it)"
