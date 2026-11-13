#!/bin/bash
# MSCC desktop helper — start/stop/status for menu entries (run as login user).
# Usage: mscc-desktop-ctl start|stop|status|restart
set -euo pipefail

notify() {
  local title="$1" body="$2" icon="${3:-dialog-information}"
  # notify-send truncates long bodies — keep short
  if command -v notify-send >/dev/null 2>&1; then
    notify-send -a "MSCC" -i "$icon" "$title" "$body" 2>/dev/null && return 0
  fi
  # No notification service (X11 desktop, openbox + lxpanel-pi, since 2026-10-12):
  # show a small Tk window instead; OK button, closes by itself after 5 s (warnings / errors stay).
  if [[ -n "${DISPLAY:-}" ]] && command -v python3 >/dev/null 2>&1; then
    python3 - "$title" "$body" "$icon" >/dev/null 2>&1 <<'PYEOF' &
import sys, tkinter as tk
title, body, icon = sys.argv[1], sys.argv[2], sys.argv[3]
err = icon == "dialog-error"
root = tk.Tk()
root.title(title)
root.resizable(False, False)
root.attributes("-topmost", True)
fg = "#b00020" if err else ("#8a5a00" if icon == "dialog-warning" else "black")
tk.Label(root, text=body, fg=fg, font=("TkDefaultFont", 12), padx=24, pady=16,
         wraplength=420, justify="left").pack()
tk.Button(root, text="OK", width=8, command=root.destroy).pack(pady=(0, 12))
root.bind("<Return>", lambda e: root.destroy())
root.bind("<Escape>", lambda e: root.destroy())
root.update_idletasks()
x = (root.winfo_screenwidth() - root.winfo_width()) // 2
y = (root.winfo_screenheight() - root.winfo_height()) // 3
root.geometry(f"+{x}+{y}")
if icon == "dialog-information":
    root.after(5000, root.destroy)
root.mainloop()
PYEOF
    disown 2>/dev/null || true
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

# Short server name for messages: sdrcore-recv -> recv, sdrcore-trans -> trans
short() { echo "${1#sdrcore-}"; }

# Join the short names of the servers whose mscc.sh line matches a pattern ("" if none).
# $1 = mscc output, $2 = sed -n pattern capturing the server name as \1
names() {
  local list="" n
  while read -r n; do
    [[ -n "$n" ]] && list="${list:+$list, }$(short "$n")"
  done < <(echo "$1" | sed -n "$2")
  echo "$list"
}

# Window text for "mscc start" output (mscc.sh start_one lines). Sets MSG + ICON.
start_message() {
  local out="$1" started already bad
  started="$(names "$out" 's/^[[:space:]]*\([a-z-]*\) started (pid.*/\1/p')"
  already="$(names "$out" 's/^[[:space:]]*\([a-z-]*\) already running.*/\1/p')"
  bad="$(names "$out" 's/.*WARNING: \([a-z-]*\) may have exited.*/\1/p')"
  ICON=dialog-information
  if [[ -n "$bad" ]]; then
    MSG="$bad may have exited. Check: mscc status"
    ICON=dialog-warning
  elif [[ -n "$started" && -n "$already" ]]; then
    MSG="Started: $started. Already running: $already."
  elif [[ -n "$already" ]]; then
    MSG="Servers already running ($already)."
  else
    MSG="Servers started (${started:-recv, trans, ms-sdr})."
  fi
}

# Window text for "mscc stop" output (mscc.sh stop_one lines). Sets MSG + ICON.
stop_message() {
  local out="$1" stopped notrun bad
  stopped="$(names "$out" 's/^[[:space:]]*\([a-z-]*\) \(stopped\|killed\)$/\1/p')"
  notrun="$(names "$out" 's/^[[:space:]]*\([a-z-]*\) not running$/\1/p')"
  bad="$(names "$out" 's/.*WARNING: could not stop \([a-z-]*\).*/\1/p')"
  ICON=dialog-information
  if [[ -n "$bad" ]]; then
    MSG="Could not stop: $bad. Check: mscc status"
    ICON=dialog-error
  elif [[ -n "$stopped" && -n "$notrun" ]]; then
    MSG="Stopped: $stopped. Were not running: $notrun."
  elif [[ -z "$stopped" && -n "$notrun" ]]; then
    MSG="Servers were not running."
  else
    MSG="Servers stopped."
  fi
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
    start_message "$out"
    notify "MSCC" "$MSG" "$ICON"
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
    stop_message "$out"
    notify "MSCC" "$MSG" "$ICON"
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
