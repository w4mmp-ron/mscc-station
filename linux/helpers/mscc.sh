#!/bin/bash
# Start/stop MSCC Linux servers
#   mscc.sh              start (default)
#   mscc.sh start|stop|status|restart
# Order start: sdrcore-recv, sdrcore-trans, then ms-sdr
# Order stop:  ms-sdr, then sdrcore-trans, sdrcore-recv
set -e

# Default work tree: $HOME/mscc (override with MSCC_DIR if needed)
if [[ -z "${HOME:-}" || "$HOME" == "/" ]]; then
  echo "ERROR: HOME is not set to a normal user home directory." >&2
  echo "       Run as a non-root login user, or set HOME / MSCC_DIR." >&2
  exit 1
fi
MSCC_DIR="${MSCC_DIR:-$HOME/mscc}"
LOG_DIR="${LOG_DIR:-$MSCC_DIR/logs}"
mkdir -p "$LOG_DIR"
cd "$MSCC_DIR"

# Pulse-capable PortAudio. Prefer /usr/local (system MSCC build + ldconfig/rpath).
# Fall back to $HOME/portaudio-install for older layouts.
# .bashrc is NOT loaded by nohup/desktop — keep this for binaries without rpath.
if [[ -z "${PORTAUDIO_PREFIX:-}" ]]; then
  if [[ -e /usr/local/lib/libportaudio.so || -e /usr/local/lib/libportaudio.so.2 ]]; then
    PA_INSTALL=/usr/local
  else
    PA_INSTALL="${HOME}/portaudio-install"
  fi
else
  PA_INSTALL="$PORTAUDIO_PREFIX"
fi
if [[ -d "$PA_INSTALL/lib" ]]; then
  export LD_LIBRARY_PATH="$PA_INSTALL/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  if [[ -d "$PA_INSTALL/lib/pkgconfig" ]]; then
    export PKG_CONFIG_PATH="$PA_INSTALL/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  fi
fi

# Stop order (ms-sdr first so cores are not left orphaned mid-session)
SERVERS_START=(sdrcore-recv sdrcore-trans ms-sdr)
SERVERS_STOP=(ms-sdr sdrcore-trans sdrcore-recv)

need() {
  if [[ ! -x "$MSCC_DIR/$1" ]]; then
    echo "ERROR: missing executable: $MSCC_DIR/$1" >&2
    echo "Expected binaries in \$HOME/mscc (or MSCC_DIR)." >&2
    exit 1
  fi
}

is_running() {
  pgrep -x "$1" >/dev/null 2>&1
}

start_one() {
  local name="$1"
  if is_running "$name"; then
    echo "$name already running (pid $(pgrep -x "$name" | xargs echo))"
    return 0
  fi
  echo "Starting $name ..."
  nohup "$MSCC_DIR/$name" >>"$LOG_DIR/$name.stdout" 2>&1 &
  local pid=$!
  sleep 0.3
  if kill -0 "$pid" 2>/dev/null || is_running "$name"; then
    echo "  $name started (pid ${pid})"
  else
    echo "  WARNING: $name may have exited — check $LOG_DIR/$name.stdout and ~/*.log" >&2
  fi
}

stop_one() {
  local name="$1"
  if ! is_running "$name"; then
    echo "$name not running"
    return 0
  fi
  echo "Stopping $name (pid $(pgrep -x "$name" | xargs echo)) ..."
  pkill -x "$name" 2>/dev/null || true
  local i
  for i in 1 2 3 4 5 6 7 8 9 10; do
    if ! is_running "$name"; then
      echo "  $name stopped"
      return 0
    fi
    sleep 0.2
  done
  echo "  $name still running — sending SIGKILL"
  pkill -9 -x "$name" 2>/dev/null || true
  sleep 0.2
  if is_running "$name"; then
    echo "  WARNING: could not stop $name" >&2
  else
    echo "  $name killed"
  fi
}

# ---------------------------------------------------------------------------
# ALSA operator levels: re-apply 100%/unmute on every start.
# Do NOT use ~/.asoundrc (unreliable on Pi OS). Do NOT rely on alsactl store
# alone (USB card order / desktop audio often resets mixers after boot).
# MSCC Init writes operator-*.ini + operator-alsa-cards.txt; we re-run amixer.
# ---------------------------------------------------------------------------
MSCC_CFG="${HOME}/.local/mscc"

alsa_set_card_100() {
  local card="$1"
  local ctl line
  [[ "$card" =~ ^[0-9]+$ ]] || return 1
  command -v amixer >/dev/null 2>&1 || return 1
  # Every simple control on the card — open path fully; MSCC owns AF gain.
  while IFS= read -r line; do
    ctl=$(sed -n "s/.*'\\([^']*\\)'.*/\\1/p" <<<"$line")
    [[ -n "$ctl" ]] || continue
    amixer -c "$card" sset "$ctl" 100% unmute >/dev/null 2>&1 \
      || amixer -c "$card" sset "$ctl" 100% >/dev/null 2>&1 \
      || amixer -c "$card" sset "$ctl" unmute >/dev/null 2>&1 \
      || true
  done < <(amixer -c "$card" scontrols 2>/dev/null || true)
  echo "  ALSA card $card → 100%/unmute (amixer)"
}

# Match PortAudio/ini name fragment to ALSA card via aplay/arecord -l or hw:N
alsa_card_from_name() {
  local name="$1"
  local card line id label token
  [[ -z "$name" ]] && return 1
  if [[ "$name" =~ hw:([0-9]+) ]]; then
    echo "${BASH_REMATCH[1]}"
    return 0
  fi
  # tokens from name (skip fluff)
  local -a toks=()
  local cleaned
  cleaned=$(sed 's/(.*//; s/USB//g; s/Audio//g; s/Device//g; s/PnP//g; s/Sound//g' <<<"$name")
  read -r -a toks <<<"$(echo "$cleaned" | tr -cs 'A-Za-z0-9._-' ' ')"
  local best=-1 bestc=""
  local score
  while IFS= read -r line; do
    [[ "$line" =~ ^card[[:space:]]+([0-9]+):[[:space:]]*([^[:space:]]+)[[:space:]]*\[([^\]]*)\] ]] || continue
    card="${BASH_REMATCH[1]}"
    label="${BASH_REMATCH[2]} ${BASH_REMATCH[3]}"
    # skip HDMI unless operator name mentions it
    if [[ "${label,,}" == *hdmi* && "${name,,}" != *hdmi* ]]; then
      continue
    fi
    score=0
    for token in "${toks[@]}"; do
      [[ ${#token} -lt 2 ]] && continue
      [[ "${token,,}" == "card" || "${token,,}" == "default" ]] && continue
      if [[ "${label,,}" == *"${token,,}"* ]]; then
        score=$((score + ${#token}))
      fi
    done
    if (( score > best )); then
      best=$score
      bestc=$card
    fi
  done < <( { aplay -l 2>/dev/null; arecord -l 2>/dev/null; } | sort -u )
  if (( best >= 3 )) && [[ -n "$bestc" ]]; then
    echo "$bestc"
    return 0
  fi
  return 1
}

apply_alsa_levels_100() {
  local cards_file="${MSCC_CFG}/operator-alsa-cards.txt"
  local sp_ini="${MSCC_CFG}/operator-speaker.ini"
  local mic_ini="${MSCC_CFG}/operator-microphone.ini"
  local -A seen=()
  local card name

  if ! command -v amixer >/dev/null 2>&1; then
    echo "NOTE: amixer not found (install alsa-utils) — cannot set ALSA levels" >&2
    return 0
  fi

  echo "ALSA operator levels (re-apply 100% every start — not .asoundrc):"

  # 1) Cards remembered by MSCC Init
  if [[ -f "$cards_file" ]]; then
    while IFS= read -r card || [[ -n "$card" ]]; do
      card=$(tr -d ' \t\r' <<<"$card")
      [[ "$card" =~ ^[0-9]+$ ]] || continue
      seen[$card]=1
    done <"$cards_file"
  fi

  # 2) Re-resolve from operator ini names (USB index may have changed)
  for f in "$sp_ini" "$mic_ini"; do
    [[ -f "$f" ]] || continue
    name=$(head -n1 "$f" | tr -d '\r')
    [[ -z "$name" ]] && continue
    if card=$(alsa_card_from_name "$name"); then
      seen[$card]=1
    fi
  done

  if [[ ${#seen[@]} -eq 0 ]]; then
    echo "  (no operator ALSA cards known — run MSCC Init once)"
    return 0
  fi

  for card in "${!seen[@]}"; do
    alsa_set_card_100 "$card" || true
  done
}

do_start() {
  need sdrcore-recv
  need sdrcore-trans
  need ms-sdr

  echo "MSCC start — $MSCC_DIR"
  apply_alsa_levels_100
  local s
  for s in "${SERVERS_START[@]}"; do
    start_one "$s"
    sleep 0.5
  done

  echo
  echo "Running:"
  pgrep -a -x sdrcore-recv || true
  pgrep -a -x sdrcore-trans || true
  pgrep -a -x ms-sdr || true
  echo
  echo "Stdout logs: $LOG_DIR/"
  echo "App logs:    ~/sdrcore-recv.log  ~/sdrcore-trans.log  ~/ms-sdr.log"
}

do_stop() {
  echo "MSCC stop — $MSCC_DIR"
  local s
  for s in "${SERVERS_STOP[@]}"; do
    stop_one "$s"
  done
  echo
  echo "Still running (if any):"
  pgrep -a -x sdrcore-recv || true
  pgrep -a -x sdrcore-trans || true
  pgrep -a -x ms-sdr || true
}

# Full install/runtime report (config, PortAudio match, CAT, Virtual*, USB).
# Lives in package: /usr/share/mscc/bin/mscc-status-report
run_status_report() {
  local r
  for r in \
    /usr/share/mscc/bin/mscc-status-report \
    "${MSCC_DIR}/mscc-status-report" \
    "$(dirname "$0")/mscc-status-report"
  do
    if [[ -x "$r" ]]; then
      # Do not let set -e abort status on FAIL (report exits 1 when issues found)
      "$r" || true
      return 0
    fi
    if [[ -f "$r" ]]; then
      python3 "$r" || true
      return 0
    fi
  done
  echo
  echo "NOTE: mscc-status-report not found — process list only."
  echo "      Reinstall mscc package for full config / PortAudio / CAT check."
}

do_status() {
  echo "MSCC status — $MSCC_DIR"
  local s
  for s in sdrcore-recv sdrcore-trans ms-sdr; do
    if is_running "$s"; then
      echo "  $s: running (pid $(pgrep -x "$s" | xargs echo))"
    else
      echo "  $s: stopped"
    fi
  done
  run_status_report
}

usage() {
  echo "Usage: $0 [start|stop|status|restart]"
  echo "  start   (default) start recv, trans, then ms-sdr"
  echo "  stop    stop ms-sdr, then trans, recv"
  echo "  status  servers + config / audio / CAT install check"
  echo "  restart stop then start"
}

cmd="${1:-start}"
case "$cmd" in
  start)   do_start ;;
  stop)    do_stop ;;
  status)  do_status ;;
  restart) do_stop; sleep 0.5; do_start ;;
  -h|--help|help) usage ;;
  *)
    echo "Unknown option: $cmd" >&2
    usage >&2
    exit 1
    ;;
esac
