#!/usr/bin/env bash
# Run the Ubuntu Avalonia UI from this repo (no .deb). Uses user-local .NET 9.
#
#   ./linux-build/run-avalonia.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd -P)"
ROOT="$(cd "$HERE/.." && pwd -P)"
PROJ="$ROOT/mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/MSCC.Avalonia.csproj"
BIN="$ROOT/mscc-ui/Avalonia-Migration/src/MSCC.Avalonia/bin/Release/net9.0/MSCC.Avalonia"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export PATH="$DOTNET_ROOT:$PATH"
export DOTNET_CLI_TELEMETRY_OPTOUT=1

if [[ ! -x "$DOTNET_ROOT/dotnet" ]]; then
  echo "Need .NET 9 at $DOTNET_ROOT (not Ubuntu's apt SDK)." >&2
  echo "  ./linux-build/mscc-ui-x64.sh   # installs SDK + publish" >&2
  exit 1
fi

if [[ ! -x "$BIN" ]]; then
  echo "Building Release…"
  dotnet build "$PROJ" -c Release --nologo -v q
fi
echo "Starting $BIN"
exec "$BIN"
