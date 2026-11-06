#!/bin/bash
# MSCC Avalonia: the client's own virtual audio pair for Remote Digital (WSJT-X on this computer).
#
#   MSCC_Remote_RX          Avalonia plays the radio's receive audio into it
#   MSCC_Remote_RX.monitor  WSJT-X Input
#   MSCC_Remote_TX          WSJT-X Output
#   MSCC_Remote_TX.monitor  Avalonia transmit audio (mic)
#
# Sink name and description must be the SAME. A description that differs from the
# name (e.g. "MSCC Remote RX" for mscc_remote_rx) crashed PortAudio's PulseAudio
# support in Avalonia (2026-11-06). 48 kHz like the server kit's VirtualA/B.
#
# Safe to re-run: a device that already exists is left alone (it may be in use).
# Login:  systemctl --user enable --now mscc-avalonia-remote-audio.service
# Manual: /usr/share/mscc-avalonia/bin/mscc-avalonia-remote-audio.sh

log() { echo "mscc-avalonia-remote-audio: $*"; }
warn() { echo "mscc-avalonia-remote-audio: WARNING: $*" >&2; }

if ! command -v pactl >/dev/null 2>&1; then
  warn "pactl not found (install pipewire-pulse or pulseaudio-utils)"
  exit 1
fi

# Wait up to 30 s for the user's PipeWire / Pulse server.
ready=0
for _ in $(seq 1 30); do
  if pactl info >/dev/null 2>&1; then
    ready=1
    break
  fi
  sleep 1
done
if [[ "$ready" -ne 1 ]]; then
  warn "PipeWire/Pulse not ready for this user"
  exit 1
fi

have_sink() {
  pactl list short sinks 2>/dev/null | awk '{print $2}' | grep -qx "$1"
}

for name in MSCC_Remote_RX MSCC_Remote_TX; do
  if have_sink "$name"; then
    log "$name already there"
    continue
  fi
  if pactl load-module module-null-sink sink_name="$name" rate=48000 \
       sink_properties=device.description="$name" >/dev/null; then
    pactl set-sink-volume "$name" 100% 2>/dev/null || true
    log "made $name (+ $name.monitor)"
  else
    warn "could not make $name"
  fi
done

rc=0
for name in MSCC_Remote_RX MSCC_Remote_TX; do
  have_sink "$name" || { warn "$name missing"; rc=1; }
done
exit $rc
