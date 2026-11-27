  # --- mscc Zero build (added by rpi/mscc-zero/make-zero-deb.sh) -------------
  # The Zero 2 W can't run local operator audio (USB): always use the dummy
  # devices, so ms-sdr reports "no local operator audio" and the client greys
  # out Phones. Written on every install / upgrade.
  mkdir -p "$CFG_DIR"
  printf 'OperatorNull' > "$CFG_DIR/operator-speaker.ini"
  printf 'OperatorNullMic' > "$CFG_DIR/operator-microphone.ini"
  chown "$REAL_USER:" "$CFG_DIR" "$CFG_DIR/operator-speaker.ini" "$CFG_DIR/operator-microphone.ini" 2>/dev/null || true
  log "Zero build: operator audio set to OperatorNull / OperatorNullMic (no local audio)"
