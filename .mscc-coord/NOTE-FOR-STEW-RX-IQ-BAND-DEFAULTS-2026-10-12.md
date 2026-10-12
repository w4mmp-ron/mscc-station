# For Stew — Receive I/Q: per-band defaults for bands the user cannot calibrate

From Ron, 2026-10-12. An idea, not a request. You decide whether and how.

## The problem

A receive I/Q calibration needs a strong, steady signal on the band. The SMSG covers 80, 40,
30 and 20 m only, and a strong enough signal is hard to find on the other bands. So in
practice a user can calibrate four bands and has nothing good for the rest.

Today the only way to fill the other bands is the RESET ALL button on the RX IQ tab, which
copies one slider value to every band. Ron's Pi shows it: all twelve bands hold 58, the
value that is right for 20 m.

## The idea

1. **A measured default per band, per radio model.** You measure the receive I/Q value on
   every band for each model you support, and those become the defaults.
2. **A mark for calibrated bands.** When the user presses APPLY, recv marks that band in
   `recv-iq.ini`, for example `RECORD=4,BAND=4,IQ_OFFSET=58,CAL=1`.
3. **RESET writes the defaults only to bands without the mark.** A band the user measured
   is never overwritten. RESET becomes a real reset, and the "copy to all bands" behaviour
   can go.

## What already exists

- Transmit I/Q works this way now: `factory/iq/<model>/iq.ini` for the six models
  (proficio, geminus and ultimus, each as legacy and mkii), and TX RESET ALL copies it in.
  Receive defaults could sit beside those files.
- Recv has a built-in table, `iq_defaults_PCB` in `iq.c`: -104, -97, -41, -41, -43, -33,
  -31, -9, -5, -3, -3, -3 for 10 m down to 2200 m. Ron thinks it comes from an earlier
  Proficio version. It does not fit his Proficio MKII: 20 m needs +58, the table says -43.
  It is still what recv writes when `recv-iq.ini` is missing.
- Windows recv still reads a per-PCB defaults file into that table; the Pi recv does not.
- `recv-iq.ini` is read by field name, so an extra `CAL=1` field does not break old files.
  All bands simply start unmarked.

## What it would take

- **You:** the measurements (every band, every model), the default files, and anything the
  client shows, such as which bands are calibrated.
- **Ron:** the Pi recv side: write and honour `CAL=1`, apply the defaults to unmarked bands.

## Things to know

- **Unit-to-unit spread is not known.** If two rigs of one model need very different
  values, a default is no better than 0. Measuring two units of one model once would show
  whether this is worth doing at all.
- **The value depends on frequency.** The old table runs from -3 on 160 m to -104 on 10 m,
  so an average of the calibrated bands, or one value for all, is a poor fill. That is why
  the idea is a default per band.
- **Measure with UP 24 kHz ticked.** The in-passband image is the one that matters, and it
  nulls at a different slider value than the far image (58 against about 87 on Ron's rig,
  20 m). See `NOTE-FOR-STEW-RX-IQ-TAB-2026-10-12.md`, point 4.
- Nothing has been changed for this, on the Pi or anywhere else.
