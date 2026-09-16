#!/bin/bash
# MSCC Pulse/PipeWire virtual digi sinks (any user — no hard-coded home paths)
#
# Creates:
#   VirtualA / VirtualA_TX  @ 48 kHz  (recv digi path; matches PipeWire + WSJT-X)
#   VirtualB / VirtualB_TX  @ 48 kHz  (digi apps / trans mic)
#
# VirtualA must not be 96 kHz on this OS: PipeWire clock is 48 kHz only, so a
# 96 kHz sink is resampled every quantum 128 → 375 Hz comb on the waterfall.
#
# Each null-sink automatically gets a monitor *source*:
#   VirtualA.monitor  → digi app RX (e.g. WSJT Input)
#   VirtualB.monitor  → sdrcore-trans digi mic
#
# PortAudio lists the Pulse *description*, not the internal name. Default is
# "Monitor of VirtualB". We force description=VirtualB.monitor so mscc-init
# and digital-microphone.ini match what you see in the capture list.
#
# Do NOT cross-link A↔B (feedback loop). Recv plays VirtualA; WSJT-X/trans
# use VirtualB. Safe to re-run (unloads previous Virtual*).
#
# Manual:  mscc-virtual-audio
# Boot:    systemctl --user enable --now mscc-virtual-audio.service
#
set -e

log() { echo "mscc-virtual-audio: $*"; }
warn() { echo "mscc-virtual-audio: WARNING: $*" >&2; }

# Wait for Pulse/PipeWire socket (user session)
ready=0
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do
  if command -v pactl >/dev/null 2>&1 && pactl info >/dev/null 2>&1; then
    ready=1
    break
  fi
  sleep 1
done
if [[ "$ready" -ne 1 ]]; then
  warn "pactl not ready (is PipeWire/Pulse running for this user?)"
  exit 1
fi
sleep 1

if ! command -v pactl >/dev/null 2>&1; then
  warn "pactl not found — install pipewire-pulse or pulseaudio-utils"
  exit 1
fi

# Remove prior MSCC virtual null sinks + remap sources (avoid duplicates after re-run)
pactl list short modules 2>/dev/null | grep -E 'sink_name=Virtual(A|B|A_TX|B_TX)|source_name=Virtual(A|B)\.monitor|source_name=MSCC_Digi' | while read -r id rest; do
  [[ -n "$id" ]] && pactl unload-module "$id" 2>/dev/null || true
done
# Also drop modules that only mention our Virtual* in args (PipeWire wording varies)
pactl list short modules 2>/dev/null | grep -E 'Virtual(A|B)(_TX)?(\.monitor)?' | while read -r id rest; do
  case "$rest" in
    *null-sink*|*remap-source*|*virtual-source*)
      [[ -n "$id" ]] && pactl unload-module "$id" 2>/dev/null || true
      ;;
  esac
done

log "loading null sinks VirtualA/B (+ _TX)…"
pactl load-module module-null-sink \
  sink_name=VirtualA \
  sink_properties=device.description=VirtualA \
  rate=48000
pactl load-module module-null-sink \
  sink_name=VirtualB \
  sink_properties=device.description=VirtualB \
  rate=48000
pactl load-module module-null-sink \
  sink_name=VirtualA_TX \
  sink_properties=device.description=VirtualA_TX \
  rate=48000
pactl load-module module-null-sink \
  sink_name=VirtualB_TX \
  sink_properties=device.description=VirtualB_TX

sleep 1

pactl set-sink-volume VirtualA 100% 2>/dev/null || true
pactl set-sink-volume VirtualB 100% 2>/dev/null || true
pactl set-sink-volume VirtualA_TX 100% 2>/dev/null || true
pactl set-sink-volume VirtualB_TX 100% 2>/dev/null || true

# Force monitor *descriptions* so PortAudio / mscc-init show the seed names.
# (Internal Pulse names are already VirtualA.monitor / VirtualB.monitor.)
for mon in VirtualA.monitor VirtualB.monitor VirtualA_TX.monitor VirtualB_TX.monitor; do
  if pactl list short sources 2>/dev/null | awk '{print $2}' | grep -qx "$mon"; then
    pactl update-source-proplist "$mon" device.description="$mon" 2>/dev/null || true
    log "source description set: $mon"
  else
    warn "expected source missing: $mon"
  fi
done

# If PortAudio still will not list auto-monitors (some PipeWire builds), add an
# explicit remap source named for digi TX capture. Harmless if both exist.
if pactl list short sources 2>/dev/null | awk '{print $2}' | grep -qx 'VirtualB.monitor'; then
  if ! pactl list short sources 2>/dev/null | awk '{print $2}' | grep -qx 'MSCC_Digi_Mic'; then
    if pactl load-module module-remap-source \
        source_name=MSCC_Digi_Mic \
        master=VirtualB.monitor \
        channels=2 \
        source_properties=device.description=VirtualB.monitor \
        2>/dev/null; then
      log "remap source MSCC_Digi_Mic → VirtualB.monitor (description=VirtualB.monitor)"
    else
      warn "module-remap-source failed (optional; auto-monitor may still work)"
    fi
  fi
fi

# Intentionally no A↔B pw-link. Recv owns VirtualA, WSJT-X/trans own VirtualB.

log "done."
log "sinks (playback — digi speaker = VirtualA):"
pactl list short sinks 2>/dev/null | grep -E 'Virtual(A|B)' || warn "no Virtual* sinks"
log "sources (capture — digi mic = VirtualB.monitor):"
pactl list short sources 2>/dev/null | grep -E 'Virtual(A|B)|MSCC_Digi' || warn "no Virtual* sources — monitors missing"

# Hard fail if digi TX capture path is absent
if ! pactl list short sources 2>/dev/null | awk '{print $2}' | grep -Eqx 'VirtualB\.monitor|MSCC_Digi_Mic'; then
  warn "VirtualB.monitor not present — digi TX will not work"
  exit 1
fi

exit 0
