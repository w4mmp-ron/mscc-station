#!/bin/bash
# usb-snap.sh - snapshot of the pill's USB audio state + server logs.
# For the 11.5 s spike hunt (Ron + Claude, 2026-10-24). Run while the fault
# is showing, BEFORE restarting anything:
#   bash ~/utilities/usb-snap.sh bad      (or: good, test1, ...)
# Saves to ~/usb-snaps/<date>_<time>_<label>/  (V:\usb-snaps on the PC).

label="${1:-snap}"
dir="$HOME/usb-snaps/$(date +%Y-%m-%d_%H%M%S)_$label"
mkdir -p "$dir" || exit 1

# Card number can change between boots: find the pill by name.
card=$(grep -i 'Multus' /proc/asound/cards | head -1 | awk '{print $1}')
out="$dir/usb.txt"

{
    echo "label: $label"
    date
    uptime
    echo
    echo "== servers"
    pgrep -a 'ms-sdr|sdrcore' || echo "(none running)"
    echo
    if [ -z "$card" ]; then
        echo "Multus card NOT FOUND"
        cat /proc/asound/cards
    else
        echo "== card $card"
        cat /proc/asound/card$card/stream0
        for d in pcm0p pcm0c; do
            echo "== $d hw_params"
            cat /proc/asound/card$card/$d/sub0/hw_params
            echo "== $d status"
            cat /proc/asound/card$card/$d/sub0/status
        done
        # Feedback + fill levels over 10 s (one line per 0.5 s), to see the trend
        echo
        echo "== 10 s trace: time | playback momentary freq | capture momentary freq | p.delay p.avail | c.delay c.avail"
        for i in $(seq 20); do
            f=$(grep 'Momentary freq' /proc/asound/card$card/stream0 | awk '{print $4}' | tr '\n' ' ')
            p=$(awk '/^delay|^avail /{printf "%s ", $3}' /proc/asound/card$card/pcm0p/sub0/status 2>/dev/null)
            c=$(awk '/^delay|^avail /{printf "%s ", $3}' /proc/asound/card$card/pcm0c/sub0/status 2>/dev/null)
            echo "$(date +%T.%N | cut -c1-12) | $f| $p| $c"
            sleep 0.5
        done
    fi
    echo
    # Pill audio counters (firmware 3.11+; older firmware just says so)
    echo
    echo "== pill audio counters since power-up"
    python3 "$HOME/utilities/audio-stats.py"
    echo
    echo "== pill audio counters, 15 s watch"
    python3 "$HOME/utilities/audio-stats.py" watch 15
    echo
    echo "== kernel USB messages (last 20)"
    journalctl -k --no-pager | grep -i usb | tail -20
} > "$out" 2>&1

cp "$HOME"/.local/mscc/*.log "$HOME"/.local/mscc/user_controls.ini "$dir"/ 2>/dev/null

echo "Saved to $dir"
ls "$dir"
