#!/usr/bin/env bash
# Build PortAudio (Pulse+ALSA) for Ubuntu x86_64 and pack mscc-portaudio_*_amd64.deb.
# Does not install to the system — that is the .deb's job.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
PA_SRC="$ROOT/portaudio"
OUT_DIR="$ROOT/linux/mscc-portaudio"
PKG_SRC="$OUT_DIR/packaging"
BUILD="$OUT_DIR/.build"

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "ERROR: amd64 PortAudio .deb is built on an x86_64 host." >&2
  exit 1
fi

VERSION="$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_SRC/DEBIAN/control" | head -1 | tr -d '\r')"
[[ -n "$VERSION" ]] || { echo "ERROR: no Version in packaging control" >&2; exit 1; }
OUT="$OUT_DIR/mscc-portaudio_${VERSION}_amd64.deb"

need() { [[ -e "$1" ]] || { echo "ERROR: missing $1" >&2; exit 1; }; }
need "$PA_SRC/configure"
need "$PKG_SRC/DEBIAN/control"
command -v dpkg-deb >/dev/null || { echo "ERROR: dpkg-deb missing" >&2; exit 1; }

if ! pkg-config --exists libpulse; then
  echo "libpulse-dev is required to build PortAudio with Pulse."
  echo "Installing build deps (pkexec)…"
  pkexec apt-get install -y libpulse-dev pkg-config
fi
pkg-config --exists libpulse || { echo "ERROR: libpulse still missing" >&2; exit 1; }
pkg-config --exists alsa || { echo "ERROR: alsa (libasound) missing — apt install libasound2-dev" >&2; exit 1; }

echo "=== mscc-portaudio amd64 builder ==="
echo "  source:  $PA_SRC"
echo "  version: $VERSION"
echo "  out:     $OUT"

rm -rf "$BUILD"
mkdir -p "$BUILD"
(
  cd "$BUILD"
  bash "$PA_SRC/configure" \
    --prefix=/usr/local \
    --with-alsa \
    --with-pulseaudio \
    --without-jack \
    --enable-shared \
    --disable-static
  mkdir -p qa examples bin lib test
  make -j"$(nproc)" lib/libportaudio.la
  make DESTDIR="$BUILD/destdir" install
)

LIB="$BUILD/destdir/usr/local/lib"
INC="$BUILD/destdir/usr/local/include"
need "$LIB/libportaudio.so.2"
need "$INC/portaudio.h"

if command -v readelf >/dev/null 2>&1; then
  if ! readelf -h "$LIB/libportaudio.so.2" 2>/dev/null | grep -qi 'X86-64\|x86-64'; then
    echo "ERROR: libportaudio.so.2 is not x86-64" >&2
    file "$LIB/libportaudio.so.2" >&2
    exit 1
  fi
  echo "  arch:    x86-64 OK"
fi
PULSE_N=$(strings "$LIB/libportaudio.so.2" 2>/dev/null | grep -ci Pulse || true)
ALSA_N=$(strings "$LIB/libportaudio.so.2" 2>/dev/null | grep -ci ALSA || true)
echo "  strings: Pulse~$PULSE_N  ALSA~$ALSA_N"
if [[ "${PULSE_N:-0}" -lt 1 || "${ALSA_N:-0}" -lt 1 ]]; then
  echo "ERROR: this PortAudio build is missing Pulse or ALSA" >&2
  exit 1
fi

STAGE="${TMPDIR:-/tmp}/mscc-portaudio-amd64-$$"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$PKG_SRC" "$STAGE/packaging"
PKG="$STAGE/packaging"
mkdir -p "$PKG/usr/local/lib" "$PKG/usr/local/include" "$PKG/usr/local/lib/pkgconfig" \
         "$PKG/etc/ld.so.conf.d" "$PKG/usr/share/doc/mscc-portaudio"

cp -a "$LIB"/libportaudio.so* "$PKG/usr/local/lib/"
cp -a "$INC"/*.h "$PKG/usr/local/include/"
if [[ -f "$LIB/pkgconfig/portaudio-2.0.pc" ]]; then
  sed -e 's|^prefix=.*|prefix=/usr/local|' \
      -e 's|^exec_prefix=.*|exec_prefix=${prefix}|' \
      -e 's|^libdir=.*|libdir=${prefix}/lib|' \
      -e 's|^includedir=.*|includedir=${prefix}/include|' \
      "$LIB/pkgconfig/portaudio-2.0.pc" > "$PKG/usr/local/lib/pkgconfig/portaudio-2.0.pc"
fi
echo "/usr/local/lib" > "$PKG/etc/ld.so.conf.d/mscc-portaudio.conf"
cat > "$PKG/usr/share/doc/mscc-portaudio/README" <<'EOF'
mscc-portaudio — PortAudio with Pulse+ALSA for Multus MSCC on Ubuntu amd64.
Installs to /usr/local. postinst runs ldconfig.
MSCC servers (mscc_*_amd64.deb) are linked with rpath /usr/local/lib.
EOF

chmod 755 "$PKG/DEBIAN"
chmod 755 "$PKG/DEBIAN/postinst" "$PKG/DEBIAN/prerm" "$PKG/DEBIAN/postrm"
chmod 644 "$PKG/DEBIAN/control" "$PKG/etc/ld.so.conf.d/mscc-portaudio.conf"
chmod 755 "$PKG/usr/local/lib"/libportaudio.so* 2>/dev/null || true
find "$PKG" -type d -exec chmod 755 {} \;

SIZE_KB=$(du -sk "$PKG" | awk '{print $1}')
if grep -q '^Installed-Size:' "$PKG/DEBIAN/control"; then
  sed -i "s/^Installed-Size:.*/Installed-Size: $SIZE_KB/" "$PKG/DEBIAN/control"
else
  echo "Installed-Size: $SIZE_KB" >> "$PKG/DEBIAN/control"
fi

rm -f "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT"
rm -rf "$STAGE"
echo "OK: $OUT"
ls -lh "$OUT"
file "$OUT"

if [[ -x "$HERE/drop-installers.sh" ]]; then
  "$HERE/drop-installers.sh" linux
fi
