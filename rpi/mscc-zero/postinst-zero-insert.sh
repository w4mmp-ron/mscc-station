  # --- mscc Zero build (added by rpi/mscc-zero/make-zero-deb.sh) -------------
  # The Zero 2 W can't run local operator audio (USB): always use the dummy
  # devices, so ms-sdr reports "no local operator audio" and the client greys
  # out Phones. Written on every install / upgrade.
  mkdir -p "$CFG_DIR"
  printf 'OperatorNull' > "$CFG_DIR/operator-speaker.ini"
  printf 'OperatorNullMic' > "$CFG_DIR/operator-microphone.ini"
  chown "$REAL_USER:" "$CFG_DIR" "$CFG_DIR/operator-speaker.ini" "$CFG_DIR/operator-microphone.ini" 2>/dev/null || true
  log "Zero build: operator audio set to OperatorNull / OperatorNullMic (no local audio)"
  # Remote mic buffer 200 ms (trans 3.165+): the Zero's WiFi holds packets back
  # ~100 ms, more than the default 100 ms buffer. Added once; a BUFFER_MS line
  # the user already has is kept.
  RMI="$CFG_DIR/remote-mic.ini"
  if [ ! -f "$RMI" ]; then
    printf '# MSCC remote operator mic (Pi sdrcore-trans)\n# PORT = UDP listen port; BUFFER_MS = remote mic buffer, 50..300 ms (default 100)\nPORT=9101\n' > "$RMI"
  fi
  if ! grep -q '^BUFFER_MS=' "$RMI"; then
    printf 'BUFFER_MS=200\n' >> "$RMI"
    log "Zero build: remote mic buffer BUFFER_MS=200 added to $RMI"
  fi
  chown "$REAL_USER:" "$RMI" 2>/dev/null || true
