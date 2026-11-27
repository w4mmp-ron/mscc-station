#!/bin/bash
# Zero version of the mscc package: the normal mscc .deb plus the dummy operator
# audio devices (OperatorNull speaker, OperatorNullMic microphone) for a Pi with
# no sound card, and MSCC Init lists them. Same package name, version <ver>+zero<N>.
#
#   bash rpi/mscc-zero/make-zero-deb.sh installers/rpi/mscc_1.0.108_arm64.deb [N] [outdir]
#
# N = Zero build number for the same mscc version (default 1); raise it for a
# rebuild so apt sees a newer version.
#
# Default outdir: installers/rpi/zero. Run in WSL or on a Pi (needs dpkg-deb).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
src="${1:?usage: make-zero-deb.sh <mscc_VER_arm64.deb> [N] [outdir]}"
rev="${2:-1}"
out="${3:-$here/../../installers/rpi/zero}"
[[ "$rev" =~ ^[0-9]+$ ]] || { echo "ERROR: N must be a number: $rev" >&2; exit 1; }
conf_rel="etc/pipewire/pipewire.conf.d/50-mscc-operator-null.conf"

[[ -f "$src" ]] || { echo "ERROR: no such file: $src" >&2; exit 1; }
[[ "$(dpkg-deb -f "$src" Package)" == "mscc" ]] || { echo "ERROR: $src is not the mscc package" >&2; exit 1; }
ver="$(dpkg-deb -f "$src" Version)"
arch="$(dpkg-deb -f "$src" Architecture)"
[[ "$ver" == *+zero* ]] && { echo "ERROR: $src is already a Zero build" >&2; exit 1; }
zver="${ver}+zero${rev}"
deb="$out/mscc_${zver}_${arch}.deb"
[[ -e "$deb" ]] && { echo "ERROR: $deb exists" >&2; exit 1; }

stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
dpkg-deb -R "$src" "$stage/pkg"
p="$stage/pkg"

# the dummy devices (CR stripped: a Windows checkout may have CRLF)
install -d -m 755 "$p/$(dirname "$conf_rel")"
tr -d '\r' < "$here/$conf_rel" > "$p/$conf_rel"
chmod 644 "$p/$conf_rel"
echo "/$conf_rel" >> "$p/DEBIAN/conffiles"
chmod 644 "$p/DEBIAN/conffiles"

# MSCC Init: add the two dummies to its speaker / microphone lists
dev_py="$p/usr/share/mscc-init-gui/mscc_init_gui/devices.py"
[[ -f "$dev_py" ]] || { echo "ERROR: no $dev_py in the package" >&2; exit 1; }
grep -q "mscc Zero build" "$dev_py" && { echo "ERROR: devices.py already has the Zero lines" >&2; exit 1; }
tr -d '\r' < "$here/devices-zero-append.py" >> "$dev_py"
find "$p/usr/share/mscc-init-gui" -name "__pycache__" -type d -prune -exec rm -rf {} +

# postinst: always set the operator audio to the dummies (end of the configure branch)
pi="$p/DEBIAN/postinst"
grep -q "mscc Zero build" "$pi" && { echo "ERROR: postinst already has the Zero lines" >&2; exit 1; }
abort_ln="$(grep -n '^abort-upgrade|abort-remove|abort-deconfigure)' "$pi" | cut -d: -f1)"
[[ "$abort_ln" =~ ^[0-9]+$ ]] || { echo "ERROR: postinst layout changed (no abort-upgrade line)" >&2; exit 1; }
end_ln=$((abort_ln - 1))
[[ "$(sed -n "${end_ln}p" "$pi")" == "  ;;" ]] || { echo "ERROR: postinst layout changed (no ;; before abort-upgrade)" >&2; exit 1; }
tr -d '\r' < "$here/postinst-zero-insert.sh" > "$stage/insert.sh"
sed -i "$((end_ln - 1))r $stage/insert.sh" "$pi"
chmod 755 "$pi"
bash -n "$pi" || { echo "ERROR: postinst has a syntax error after the Zero lines" >&2; exit 1; }

# control: Zero version, one more description line, Installed-Size recounted
sed -i "s/^Version: .*/Version: ${zver}/" "$p/DEBIAN/control"
# (Description is the last field, so the new lines go at the end)
tail -1 "$p/DEBIAN/control" | grep -q '^ ' || { echo "ERROR: control does not end with the Description" >&2; exit 1; }
printf ' .\n Zero build: also adds the dummy operator audio devices OperatorNull and\n OperatorNullMic and always uses them (the Zero has no local audio).\n' >> "$p/DEBIAN/control"
size="$(du -sk "$p" | cut -f1)"   # build-deb.sh counts DEBIAN too
sed -i "s/^Installed-Size: .*/Installed-Size: ${size}/" "$p/DEBIAN/control"

mkdir -p "$out"
dpkg-deb --root-owner-group --build "$p" "$deb" >/dev/null
echo "built $deb (from $(basename "$src"))"
