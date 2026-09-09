#!/usr/bin/env bash
# Build MSCC Avalonia UI for Ubuntu x86_64 only.
# Does not touch linux-arm64 publish or mscc-ui_*_arm64.deb (Ron's Pi UI).
#
#   ./linux-build/mscc-ui-x64.sh
#
# Needs user-local .NET 9 SDK ($HOME/.dotnet). Project stays net9.0.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
AVA="$ROOT/mscc-ui/Avalonia-Migration"
PUBLISH="$AVA/publish/linux-x64-sc"
UIDIR="${MSCC_UI_DIR:-$HOME/mscc-ui}"
DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export DOTNET_ROOT
export PATH="$DOTNET_ROOT:$PATH"
export DOTNET_CLI_TELEMETRY_OPTOUT=1

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "ERROR: this script is Ubuntu x86_64 only." >&2
  echo "  Pi UI:  PowerShell build-mscc-ui-deb.ps1 (linux-arm64)" >&2
  exit 1
fi

if [[ ! -x "$DOTNET_ROOT/dotnet" ]]; then
  echo "Installing .NET 9 SDK to $DOTNET_ROOT (user-local, not Ubuntu's .NET 10)…"
  curl -fsSL https://dot.net/v1/dotnet-install.sh -o /tmp/dotnet-install.sh
  bash /tmp/dotnet-install.sh --channel 9.0 --install-dir "$DOTNET_ROOT"
fi

echo "mscc-ui-x64: publish linux-x64 → $PUBLISH"
dotnet publish "$AVA/src/MSCC.Avalonia/MSCC.Avalonia.csproj" \
  -c Release -r linux-x64 --self-contained true \
  -p:PublishSingleFile=false \
  -o "$PUBLISH"

bin="$PUBLISH/MSCC.Avalonia"
[[ -x "$bin" ]] || { echo "ERROR: missing $bin" >&2; exit 1; }
file "$bin" | grep -q 'x86-64' || { echo "ERROR: not x86-64: $(file "$bin")" >&2; exit 1; }

rm -rf "$UIDIR"
mkdir -p "$UIDIR"
cp -a "$PUBLISH"/. "$UIDIR/"
chmod 755 "$UIDIR/MSCC.Avalonia"

cat > "$UIDIR/mscc-ui" << EOF
#!/bin/sh
APPDIR="$UIDIR"
export LD_LIBRARY_PATH="\$APPDIR\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
cd "\$APPDIR" || exit 1
exec "\$APPDIR/MSCC.Avalonia" "\$@"
EOF
chmod 755 "$UIDIR/mscc-ui"

mkdir -p "$HOME/.local/bin"
ln -sfn "$UIDIR/mscc-ui" "$HOME/.local/bin/mscc-ui"

icon_src="$AVA/packaging/mscc-ui/icons/mscc-ui-256.png"
if [[ -f "$icon_src" ]]; then
  mkdir -p "$HOME/.local/share/icons/hicolor/256x256/apps"
  cp -a "$icon_src" "$HOME/.local/share/icons/hicolor/256x256/apps/mscc-ui.png"
fi
if [[ -f "$AVA/packaging/mscc-ui/icons/mscc-ui-48.png" ]]; then
  mkdir -p "$HOME/.local/share/icons/hicolor/48x48/apps"
  cp -a "$AVA/packaging/mscc-ui/icons/mscc-ui-48.png" \
    "$HOME/.local/share/icons/hicolor/48x48/apps/mscc-ui.png"
fi

mkdir -p "$HOME/.local/share/applications"
sed "s|@UIDIR@|$UIDIR|g" "$HERE/desktop/mscc-ui.desktop.in" \
  > "$HOME/.local/share/applications/mscc-ui.desktop"
command -v update-desktop-database >/dev/null && \
  update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

echo "mscc-ui-x64: installed $UIDIR (x86_64)"
echo "  run:  mscc-ui     or Super, type MSCC UI"
echo "  host: 127.0.0.1 port 8888  (MSCC Start first)"
echo "  arm64 publish/deb: untouched"
