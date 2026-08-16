#!/bin/bash
# MSCC desktop helper — start/stop/status for menu entries (run as login user).
# Usage: mscc-desktop-ctl start|stop|status|restart
set -euo pipefail

notify() {
  local title="$1" body="$2" icon="${3:-dialog-information}"
  # notify-send truncates long bodies — keep short
  if command -v notify-send >/dev/null 2>&1; then
    notify-send -a "MSCC" -i "$icon" "$title" "$body" 2>/dev/null || true
  fi
}

find_mscc() {
  if command -v mscc >/dev/null 2>&1; then
    echo "mscc"
    return 0
  fi
  if [[ -x "${HOME:-}/mscc/mscc.sh" ]]; then
    echo "${HOME}/mscc/mscc.sh"
    return 0
  fi
  return 1
}

cmd="${1:-}"
if [[ -z "$cmd" ]]; then
  echo "Usage: $0 start|stop|status|restart" >&2
  exit 2
fi

if ! MSCC_BIN="$(find_mscc)"; then
  msg="mscc not found. Install the mscc package and log in as the install user."
  echo "ERROR: $msg" >&2
  notify "MSCC" "$msg" dialog-error
  exit 1
fi

case "$cmd" in
start)
  # Best-effort digi sinks (ignore failure if Pulse not up)
  if command -v mscc-virtual-audio >/dev/null 2>&1; then
    mscc-virtual-audio >/dev/null 2>&1 || true
  elif [[ -x /usr/share/mscc/bin/mscc-virtual-audio.sh ]]; then
    /usr/share/mscc/bin/mscc-virtual-audio.sh >/dev/null 2>&1 || true
  fi
  if out="$("$MSCC_BIN" start 2>&1)"; then
    echo "$out"
    notify "MSCC" "Servers started (recv, trans, ms-sdr)."
    exit 0
  else
    ec=$?
    echo "$out" >&2
    notify "MSCC" "Start failed. Try Terminal: mscc start" dialog-error
    exit "$ec"
  fi
  ;;
stop)
  if out="$("$MSCC_BIN" stop 2>&1)"; then
    echo "$out"
    notify "MSCC" "Servers stopped."
    exit 0
  else
    ec=$?
    echo "$out" >&2
    notify "MSCC" "Stop failed. Try Terminal: mscc stop" dialog-error
    exit "$ec"
  fi
  ;;
restart)
  "$0" stop || true
  sleep 0.5
  exec "$0" start
  ;;
status)
  # Full report to the terminal (desktop entry uses Terminal=true + pause).
  # Short notify only — full text is too long for the notification bubble.
  set +e
  out="$("$MSCC_BIN" status 2>&1)"
  set -e
  echo "$out"
  summary="$(echo "$out" | sed -n 's/^[[:space:]]*\(MSCC status:.*\)$/\1/p' | tail -1)"
  if [[ -z "$summary" ]]; then
    if echo "$out" | grep -q ' FAIL '; then
      summary="Issues found — see Terminal window"
    else
      summary="See Terminal for full report"
    fi
  fi
  if echo "$out" | grep -q 'FAIL  '; then
    notify "MSCC status" "$summary" dialog-warning
  else
    notify "MSCC status" "$summary"
  fi
  # Exit 0 always: FAILs are in the report; non-zero makes some Terminals close hard.
  exit 0
  ;;
*)
  echo "Usage: $0 start|stop|status|restart" >&2
  exit 2
  ;;
esac
