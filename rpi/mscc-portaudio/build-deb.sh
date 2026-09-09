#!/bin/bash
# Build mscc-portaudio_*_arm64.deb from sibling portaudio tree (Pi AArch64 build).
#
# Default source:
#   libs:    ../portaudio/build/libportaudio.so*
#   headers: ../portaudio/include/*.h
#   pc:      ../portaudio/build/portaudio-2.0.pc (optional)
#
# Override: PORTAUDIO_ROOT=/path/to/portaudio ./build-deb.sh
# Or legacy layout: PORTAUDIO_INSTALL=/path (expects lib/ + include/)
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TREE="$(cd "$ROOT/.." && pwd)"
PKG_SRC="$ROOT/packaging"
VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
OUT="$ROOT/mscc-portaudio_${VERSION}_arm64.deb"

# Resolve library + header locations
if [[ -n "${PORTAUDIO_INSTALL:-}" ]]; then
  LIB_SRC="${PORTAUDIO_INSTALL}/lib"
  INC_SRC="${PORTAUDIO_INSTALL}/include"
  PC_SRC="${PORTAUDIO_INSTALL}/lib/pkgconfig/portaudio-2.0.pc"
  SRC_NOTE="$PORTAUDIO_INSTALL (PORTAUDIO_INSTALL)"
else
  PA_ROOT="${PORTAUDIO_ROOT:-$TREE/portaudio}"
  if [[ -f "$PA_ROOT/build/libportaudio.so.2" ]]; then
    LIB_SRC="$PA_ROOT/build"
    INC_SRC="$PA_ROOT/include"
    PC_SRC="$PA_ROOT/build/portaudio-2.0.pc"
    SRC_NOTE="$PA_ROOT (build + include)"
  elif [[ -f "$PA_ROOT/lib/libportaudio.so.2" ]]; then
    LIB_SRC="$PA_ROOT/lib"
    INC_SRC="$PA_ROOT/include"
    PC_SRC="$PA_ROOT/lib/pkgconfig/portaudio-2.0.pc"
    SRC_NOTE="$PA_ROOT (prefix layout)"
  else
    echo "ERROR: no libportaudio.so.2 under $PA_ROOT/build or $PA_ROOT/lib" >&2
    echo "  Build PortAudio on the Pi first, then copy AArch64 .so into the tree." >&2
    exit 1
  fi
fi

echo "=== mscc-portaudio deb builder ==="
echo "  version: $VERSION"
echo "  source:  $SRC_NOTE"
echo "  libs:    $LIB_SRC"
echo "  headers: $INC_SRC"
echo "  out:     $OUT"

need() { [[ -e "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }
need "$LIB_SRC/libportaudio.so.2"
need "$INC_SRC/portaudio.h"
command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb not found" >&2; exit 1; }

# Sanity: arm64
if command -v readelf >/dev/null 2>&1; then
  if ! readelf -h "$LIB_SRC/libportaudio.so.2" 2>/dev/null | grep -qi 'AArch64\|ARM aarch64'; then
    echo "ERROR: libportaudio.so.2 is not AArch64 — refuse to package" >&2
    readelf -h "$LIB_SRC/libportaudio.so.2" | grep -E 'Class|Machine' || true
    exit 1
  fi
  echo "  arch:    AArch64 OK"
fi

# Pulse + ALSA present?
PULSE_N=$(strings "$LIB_SRC/libportaudio.so.2" 2>/dev/null | grep -ci Pulse || true)
ALSA_N=$(strings "$LIB_SRC/libportaudio.so.2" 2>/dev/null | grep -ci ALSA || true)
echo "  strings: Pulse~$PULSE_N  ALSA~$ALSA_N"
if [[ "${PULSE_N:-0}" -lt 1 || "${ALSA_N:-0}" -lt 1 ]]; then
  echo "WARNING: weak Pulse/ALSA string counts — verify this is the MSCC build" >&2
fi

STAGE="${TMPDIR:-/tmp}/mscc-portaudio-build-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"

mkdir -p "$PKG/usr/local/lib" "$PKG/usr/local/include" "$PKG/usr/local/lib/pkgconfig" \
         "$PKG/etc/ld.so.conf.d" "$PKG/usr/share/doc/mscc-portaudio"

# Shared library + soname links
cp -a "$LIB_SRC"/libportaudio.so* "$PKG/usr/local/lib/"
# Headers (PortAudio public set)
cp -a "$INC_SRC"/*.h "$PKG/usr/local/include/"

# pkg-config (rewrite prefix to /usr/local)
if [[ -f "$PC_SRC" ]]; then
  sed -e 's|^prefix=.*|prefix=/usr/local|' \
      -e 's|^exec_prefix=.*|exec_prefix=${prefix}|' \
      -e 's|^libdir=.*|libdir=${prefix}/lib|' \
      -e 's|^includedir=.*|includedir=${prefix}/include|' \
      "$PC_SRC" > "$PKG/usr/local/lib/pkgconfig/portaudio-2.0.pc"
else
  cat > "$PKG/usr/local/lib/pkgconfig/portaudio-2.0.pc" <<'EOF'
prefix=/usr/local
exec_prefix=${prefix}
libdir=${prefix}/lib
includedir=${prefix}/include

Name: PortAudio
Description: Portable audio I/O (MSCC Pulse+ALSA)
Version: 19.8
Libs: -L${libdir} -lportaudio
Libs.private: -lm -lpthread
Cflags: -I${includedir} -DPA_USE_ALSA=1 -DPA_USE_PULSEAUDIO=1
Requires.private: alsa libpulse
EOF
fi

echo "/usr/local/lib" > "$PKG/etc/ld.so.conf.d/mscc-portaudio.conf"

cat > "$PKG/usr/share/doc/mscc-portaudio/README" <<'EOF'
mscc-portaudio — PortAudio with Pulse+ALSA for Multus MSCC on Raspberry Pi OS.

Installs to /usr/local/lib and /usr/local/include.
postinst runs ldconfig.

Source tree: portaudio/build (libs) + portaudio/include (headers).
MSCC C binaries (recv/trans/mscc-init) should be built with:
  PORTAUDIO_PREFIX=/usr/local
  -Wl,-rpath,/usr/local/lib

See portaudio/BUILD-MSCC.txt in the MSCC worktrees.
EOF

chmod 755 "$PKG/DEBIAN"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/prerm" "$PKG/DEBIAN/postrm"
chmod 644 "$PKG/DEBIAN/control"
chmod 644 "$PKG/etc/ld.so.conf.d/mscc-portaudio.conf"
chmod 755 "$PKG/usr/local/lib/libportaudio.so"* 2>/dev/null || true
find "$PKG" -type d -exec chmod 755 {} \;

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control" 2>/dev/null \
    || sed -i '' "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
else
  echo "Installed-Size: $SIZE_KB" >> "$PKG/DEBIAN/control"
fi

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo
echo "OK: $OUT"
ls -la "$OUT"
echo
echo "Verify package contents:"
echo "  dpkg-deb -c $OUT | grep libportaudio"
echo
echo "Install on Pi (before or with mscc):"
echo "  sudo apt install -y ./mscc-portaudio_${VERSION}_arm64.deb"
echo "  ldconfig -p | grep portaudio"
