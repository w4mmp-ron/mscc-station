#!/usr/bin/env bash
# Cross-publish MSCC Avalonia UI for Raspberry Pi OS 64-bit (linux-arm64).
# Runs on this Ubuntu x86_64 laptop. Does not touch linux-x64 publish.
#
#   ./linux-build/mscc-ui-arm64.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd -P)"
ROOT="$(cd "$HERE/.." && pwd -P)"
AVA="$ROOT/mscc-ui/Avalonia-Migration"
PUBLISH="$AVA/publish/linux-arm64-sc"
DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export DOTNET_ROOT
export PATH="$DOTNET_ROOT:$PATH"
export DOTNET_CLI_TELEMETRY_OPTOUT=1

if [[ ! -x "$DOTNET_ROOT/dotnet" ]]; then
  echo "Need .NET 9 at $DOTNET_ROOT" >&2
  exit 1
fi

echo "mscc-ui-arm64: publish linux-arm64 → $PUBLISH"
rm -rf "$PUBLISH"
dotnet publish "$AVA/src/MSCC.Avalonia/MSCC.Avalonia.csproj" \
  -c Release -r linux-arm64 --self-contained true \
  -p:PublishSingleFile=false \
  -o "$PUBLISH"

bin="$PUBLISH/MSCC.Avalonia"
[[ -f "$bin" ]] || { echo "ERROR: missing $bin" >&2; exit 1; }
file "$bin" | grep -qiE 'ARM aarch64|ARM64|aarch64' || {
  echo "ERROR: not arm64: $(file "$bin")" >&2
  exit 1
}
echo "OK: $(file "$bin")"
ls -lh "$bin"
