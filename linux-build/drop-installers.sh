#!/usr/bin/env bash
# Copy the newest release packages into installers/{rpi,linux,windows}/.
# History stays in the builder folders. Run after any kit update.
#
#   ./linux-build/drop-installers.sh          # all three
#   ./linux-build/drop-installers.sh linux    # one platform
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
DEST="$ROOT/installers"

latest() {
  # newest by version-sort of filename
  local dir="$1" pat="$2"
  local f
  f="$(ls -1 "$dir"/$pat 2>/dev/null | sort -V | tail -1 || true)"
  [[ -n "$f" && -f "$f" ]] || return 1
  echo "$f"
}

copy_latest() {
  local src_dir="$1" pat="$2" dest_dir="$3"
  local src dest base prefix
  src="$(latest "$src_dir" "$pat")" || {
    echo "MISSING: $src_dir/$pat"
    return 1
  }
  mkdir -p "$dest_dir"
  dest="$dest_dir/$(basename "$src")"
  cp -a "$src" "$dest"
  # Debian name_version_arch.deb — keep only this version in the current-kit folder
  base="$(basename "$src")"
  name="${base%%_[0-9]*}"
  shopt -s nullglob
  for old in "$dest_dir"/"${name}"_*.deb; do
    [[ "$(basename "$old")" == "$base" ]] && continue
    rm -f "$old"
  done
  shopt -u nullglob
  echo "OK  $(basename "$src")  →  ${dest_dir#"$ROOT/"}/"
}

copy_file() {
  local src="$1" dest_dir="$2"
  [[ -f "$src" ]] || { echo "MISSING: $src"; return 1; }
  mkdir -p "$dest_dir"
  cp -a "$src" "$dest_dir/"
  echo "OK  $(basename "$src")  →  ${dest_dir#"$ROOT/"}/"
}

drop_linux() {
  echo "=== installers/linux (Ubuntu amd64) ==="
  copy_latest "$ROOT/linux/mscc-portaudio" "mscc-portaudio_*_amd64.deb" "$DEST/linux" || true
  copy_latest "$ROOT/linux/mscc-deb" "mscc_*_amd64.deb" "$DEST/linux" || true
  copy_latest "$ROOT/mscc-ui/Release/avalonia/x86_64" "mscc-ui_*_amd64.deb" "$DEST/linux" \
    || copy_latest "$ROOT/mscc-ui/Avalonia-Migration" "mscc-ui_*_amd64.deb" "$DEST/linux" || true
  copy_latest "$ROOT/mscc-ui/Release/avalonia/x86_64" "mscc-init-gui_*_all.deb" "$DEST/linux" \
    || copy_latest "$ROOT/rpi/mscc-init-gui" "mscc-init-gui_*_all.deb" "$DEST/linux" || true
  copy_file "$ROOT/linux/mscc-deb/install-mscc.sh" "$DEST/linux" || true
}

drop_rpi() {
  echo "=== installers/rpi (Raspberry Pi arm64) ==="
  copy_latest "$ROOT/rpi/mscc-portaudio" "mscc-portaudio_*_arm64.deb" "$DEST/rpi" \
    || copy_latest "$ROOT/mscc-ui/Release/avalonia/arm64" "mscc-portaudio_*_arm64.deb" "$DEST/rpi" || true
  copy_latest "$ROOT/rpi/mscc-deb" "mscc_*_arm64.deb" "$DEST/rpi" || true
  copy_latest "$ROOT/rpi/mscc-init-gui" "mscc-init-gui_*_all.deb" "$DEST/rpi" || true
  copy_latest "$ROOT/mscc-ui/Release/avalonia/arm64" "mscc-ui_*_arm64.deb" "$DEST/rpi" \
    || copy_latest "$ROOT/mscc-ui/Avalonia-Migration" "mscc-ui_*_arm64.deb" "$DEST/rpi" || true
  copy_file "$ROOT/rpi/mscc-deb/install-mscc.sh" "$DEST/rpi" || true
}

drop_windows() {
  echo "=== installers/windows ==="
  local src dest_dir="$DEST/windows"
  src="$(ls -1 "$ROOT/mscc-ui/Release/windows-wpf"/mscc-net9-R*-install.exe 2>/dev/null | sort -V | tail -1 || true)"
  if [[ -z "$src" ]]; then
    echo "MISSING: mscc-ui/Release/windows-wpf/mscc-net9-R*-install.exe"
    return 1
  fi
  mkdir -p "$dest_dir"
  cp -a "$src" "$dest_dir/"
  shopt -s nullglob
  for old in "$dest_dir"/mscc-net9-R*-install.exe; do
    [[ "$(basename "$old")" == "$(basename "$src")" ]] && continue
    rm -f "$old"
  done
  shopt -u nullglob
  echo "OK  $(basename "$src")  →  installers/windows/"
}

WHICH="${1:-all}"
case "$WHICH" in
  all) drop_linux; drop_rpi; drop_windows ;;
  linux) drop_linux ;;
  rpi|pi) drop_rpi ;;
  windows) drop_windows ;;
  -h|--help)
    echo "Usage: linux-build/drop-installers.sh [all|linux|rpi|windows]"
    exit 0
    ;;
  *) echo "Usage: linux-build/drop-installers.sh [all|linux|rpi|windows]" >&2; exit 2 ;;
esac
