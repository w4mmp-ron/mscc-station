#!/bin/bash
# MSCC Pulse/PipeWire virtual digi sinks (any user — no hard-coded home paths)
#
# Creates:
#   VirtualA / VirtualA_TX  @ 96 kHz  (recv digi path)
#   VirtualB / VirtualB_TX  @ default rate (often 48 kHz, digi apps)
#
# Each null-sink automatically gets a monitor *source*:
#   VirtualA.monitor  → digi app RX (e.g. WSJT Input)
#   VirtualB.monitor  → sdrcore-trans digi mic
#
# PortAudio lists the Pulse *description*, not the internal name. Default is
# "Monitor of VirtualB". We force description=VirtualB.monitor so mscc-init
# and digital-microphone.ini match what you see in the capture list.
#
# Optional bidirectional links (A↔B, A_TX↔B_TX) when pw-link is available.
# Safe to re-run (unloads previous Virtual* modules first).
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
  rate=96000
pactl load-module module-null-sink \
  sink_name=VirtualB \
  sink_properties=device.description=VirtualB
pactl load-module module-null-sink \
  sink_name=VirtualA_TX \
  sink_properties=device.description=VirtualA_TX \
  rate=96000
pactl load-module module-null-sink \
  sink_name=VirtualB_TX \
  sink_properties=device.description=VirtualB_TX

sleep 1

# Default Virtual* levels, then overlay sticky saves from MSCC Volume GUI
pactl set-sink-volume VirtualA 60% 2>/dev/null || true
pactl set-sink-volume VirtualB 60% 2>/dev/null || true
pactl set-sink-volume VirtualA_TX 60% 2>/dev/null || true
pactl set-sink-volume VirtualB_TX 60% 2>/dev/null || true

_mscc_restore_volumes() {
  local cfg="${HOME}/.local/mscc/volume-levels.conf"
  local key kind name pct mute
  # Prefer Python restore (handles operator devices + name rematch)
  if command -v python3 >/dev/null 2>&1; then
    local pyrest=""
    for cand in \
      /usr/bin/mscc-volume-restore \
      /usr/share/mscc-init-gui/mscc-volume-restore \
      "${HOME}/mscc/mscc-volume-gui/mscc-volume-restore"
    do
      if [[ -x "$cand" ]]; then
        log "restoring sticky volumes via $cand"
        "$cand" 2>/dev/null || true
        return 0
      fi
      if [[ -f "$cand" ]]; then
        pyrest="$cand"
        break
      fi
    done
    if [[ -n "$pyrest" ]]; then
      log "restoring sticky volumes via $pyrest"
      python3 "$pyrest" 2>/dev/null || true
      return 0
    fi
  fi
  # Shell fallback: apply lines that name Virtual* (or any present sink/source)
  [[ -f "$cfg" ]] || return 0
  log "restoring sticky volumes from $cfg"
  while IFS='|' read -r key kind name pct mute || [[ -n "${key:-}" ]]; do
    [[ -z "${key:-}" || "$key" =~ ^# ]] && continue
    [[ "$kind" == "sink" || "$kind" == "source" ]] || continue
    [[ -n "${name:-}" && -n "${pct:-}" ]] || continue
    if [[ "$kind" == "sink" ]]; then
      pactl list short sinks 2>/dev/null | awk '{print $2}' | grep -qx "$name" || continue
      pactl set-sink-volume "$name" "${pct}%" 2>/dev/null || true
      if [[ "${mute:-0}" == "1" ]]; then
        pactl set-sink-mute "$name" 1 2>/dev/null || true
      else
        pactl set-sink-mute "$name" 0 2>/dev/null || true
      fi
    else
      pactl list short sources 2>/dev/null | awk '{print $2}' | grep -qx "$name" || continue
      pactl set-source-volume "$name" "${pct}%" 2>/dev/null || true
      if [[ "${mute:-0}" == "1" ]]; then
        pactl set-source-mute "$name" 1 2>/dev/null || true
      else
        pactl set-source-mute "$name" 0 2>/dev/null || true
      fi
    fi
  done < "$cfg"
}
_mscc_restore_volumes

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

if command -v pw-link >/dev/null 2>&1; then
  log "linking A↔B and A_TX↔B_TX (pw-link)…"
  # Receive path
  pw-link VirtualA:monitor_FL    VirtualB:playback_FL    2>/dev/null || true
  pw-link VirtualA:monitor_FR    VirtualB:playback_FR    2>/dev/null || true
  pw-link VirtualB:monitor_FL    VirtualA:playback_FL    2>/dev/null || true
  pw-link VirtualB:monitor_FR    VirtualA:playback_FR    2>/dev/null || true
  # Transmit path
  pw-link VirtualB_TX:monitor_FL VirtualA_TX:playback_FL 2>/dev/null || true
  pw-link VirtualB_TX:monitor_FR VirtualA_TX:playback_FR 2>/dev/null || true
  pw-link VirtualA_TX:monitor_FL VirtualB_TX:playback_FL 2>/dev/null || true
  pw-link VirtualA_TX:monitor_FR VirtualB_TX:playback_FR 2>/dev/null || true
else
  warn "pw-link not found — sinks created without cross-links (optional)"
fi

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
