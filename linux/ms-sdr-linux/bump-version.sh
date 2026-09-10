#!/bin/sh
# Bump VERSION_MINOR in source/version.h on every make (0..255, wraps).
# Linux equivalent of Windows PreBuildEvent for Core version (CMD 0xB3).
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
FILE="$ROOT/source/version.h"

if [ ! -f "$FILE" ]; then
    echo "bump-version: missing $FILE" >&2
    exit 1
fi

TMP="$FILE.tmp.$$"
awk '
  $1 == "#define" && $2 == "VERSION_MINOR" {
    n = $3 + 1
    if (n > 255) n = 0
    $3 = n
  }
  { print }
' "$FILE" > "$TMP"

MAJOR=$(awk '$1=="#define" && $2=="VERSION_MAJOR" { print $3; exit }' "$TMP")
MINOR=$(awk '$1=="#define" && $2=="VERSION_MINOR" { print $3; exit }' "$TMP")
mv "$TMP" "$FILE"
echo "ms-sdr: Core version bump -> ${MAJOR}.${MINOR}"
