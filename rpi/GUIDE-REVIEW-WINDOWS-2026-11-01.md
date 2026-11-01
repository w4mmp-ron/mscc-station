# Windows Operator's Guide review (Claude, 2026-11-01)

Guide: `docs/manuals/MSCC-Operators-Guide-Windows-Release-0.pdf`, Release 0 rev 1, dated 10/8/26
(MSCC 10.8.1), 60 pages. Made with WeasyPrint (HTML/Markdown source), but the source is NOT in the
repo (only PDFs since the 2026-09-28 drafts). Checked against: client 10.31.2
(`mscc-ui/windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Wpf`), Stew's ms-sdr 3.188 / trans 3.153 /
recv 3.151, MKII PTT firmware source, `.mscc-coord/MANUAL-UPDATES-FROM-RON.md` (Ron's 9/29 answers).
All 60 pages viewed as images (PyMuPDF in the scratchpad).

## A. Wrong now (behaviour changed since 10/8)

1. **Calibration is stored in the radio.** QRP CAL accept, TX IQ APPLY and RX IQ APPLY now write the
   value into the radio (0xC8) at once (ms-sdr `Radio_cal_store_power`, `Radio_cal_store_iq`). At
   start the radio's values win; the PC file only fills bands the radio doesn't hold. AMP CAL and
   FREQ CAL are still PC files only. The guide says Save settings "parks TX IQ, QRP and AMP
   calibration on the PC" (p.4, 9, 19, 23-25, 31, 56-57). Rewrite: what the radio keeps, what the
   PC keeps, what Save settings is still for (AMP CAL + a backup copy).
   Good news for the guide: a radio moved to another PC brings its QRP / I/Q cal with it.
2. **TX IQ RESET ALL and Settings "Reset configuration" don't stick.** Both reset the PC files
   (ms-sdr `CMD_SET_IQ_DEFAULTS` reseeds iq.ini from factory) but the radio's stored values win at
   the next start. The guide promises factory values (p.31, 39, 57). The 0xC8 clear was removed in
   FW 3.239; Stew's open list has "reset to factory needs the 0xC8 clear back". Guide must say this
   or wait for the fix. **Code issue for Stew, not only a guide issue.**
3. **TX IQ tab has a PHASE slider** (client 10.31.2, +/-100 steps = +/-5 deg). Guide (2.10) and its
   pictures show OFFSET only. Procedure: OFFSET first, then PHASE one step at a time, recheck
   OFFSET (client tooltip). The on-tab instruction line in the client also still omits PHASE.
4. **RX IQ "RESET ALL" is now "APPLY ALL"** in the client (copies the current value to all bands).
   Guide 2.9 item 6 still says RESET ALL. "RX IQ values ... stay in recv-iq.ini" (p.28) is now
   also wrong: RX IQ is stored in the radio too.
5. **POTENTIA / QSK is described wrongly** (2.4 item 11, Appendix B). Guide: "PIN-diode T/R
   switching ... Full QSK is not supported." Firmware (MKII PTT cw.c / tx.c) and Ron's 9/29 answer:
   ON = Potentia 50/100 attached, AMP line and PA keyed together (full break-in, no shaping);
   OFF (default) = AMP line first, wait the key delay (taken from HOLD), then PA, so a relay amp can
   change over. Leave off unless a Potentia is attached. Ron's paste-ready text is in
   `.mscc-coord/MANUAL-UPDATES-FROM-RON.md`; it was never applied to the Windows guide.
   Also HOLD now sets the amp key delay for non-QSK, not only the hang time.
6. **Firmware table (p.47) is out of date.** Field versions now (Stew's notes 10-31): Proficio
   Legacy 1.236, MKII PTT 3.242, MKII ATU 4.243 (built, not flashed), Geminus MKII 2.153,
   Ultimus Legacy 6.236, Ultimus MKII PTT 8.242, MKII ATU 7.243; Geminus Legacy 5.120 unchanged.
   Example file name `...-20260822.cyacd` is old too. Which file ships = Stew.
7. **Broken reference:** 2.3 item 14 (Remote) "See Section 4": this guide has no Section 4 (it is
   the Remote guide's Section 4, copied over).
8. **TX IQ example shows no change:** "Before ... OFFSET is 6" and "After ... OFFSET is 6" (p.29-30).
9. Version examples are mixed and old: installer R9-26-6 (now R10.31.2), title bar 10.4.0 /
   Core 3.181 / FW 3.232 (p.8), setup-note shot 10.8.0, Debug Log shot shows client 9.25.0 (p.45).
10. ALC advice "just into the yellow" (p.22, 58, 60): trans 3.149+/3.152 changed the ALC ceiling
    and Stew's meter now shows peak against the ceiling. Ask Stew what the target reading is now.

Checked and RIGHT (my 11-01 low-effort pass guessed wrong): Thr 1-1009 and LO FINE -1000..+2009
are the client's real ranges; Lo / Hi / CW / Step cycles and values match the client; tab list
matches; Appendix D and F tables are fine on the page (only the text extraction was scrambled).

## B. Order / structure

1. Things needed first are at the back: "Firmware first" (Overview) -> Appendix A (p.47); install
   step 3 "install Zadig first" -> Appendix A.2 (p.51). Move both into the install chapter.
2. Save settings is explained in 2.2 "Connecting", before calibration exists. Move to calibration.
3. "Using this PC as a host" (p.10-11) is a remote topic; Remote guide, with a pointer here.
4. Calibration follows the tab strip (QRP, AMP, RX IQ, TX IQ, FREQ CAL), not job order. Suggested:
   FREQ CAL -> RX IQ -> QRP CAL -> TX IQ -> AMP CAL -> Save. The Overview names a third order
   (FREQ, QRP, TX IQ).
5. No operating chapter: 2.3 is a 62-item control list; no "make an SSB / CW / digital contact".
   Only WSJT-X has steps (Appendix F).
6. Repeats: keyer in 2.4 and Appendix B; install in 2.1 and Appendix C; AMP overdrive warning 4x.
7. History remarks that don't belong: "adapted from the 2022 guide", "as the 2022 guide said",
   "no separate Initialize.bat step anymore", "older versions refused INVALID BAND".
8. Overview item 3 uses Save settings / park before either is explained.

## C. Pictures and layout (seen on the page images)

1. **Numbered lists, un-numbered pictures.** Every control list (main window 1-62, each tab) uses
   numbers, but no screenshot has number callouts. The reader must match by guesswork. Biggest
   usability problem in the guide.
2. Full-window screenshots (p.3, 8, 11) are shrunk to ~half page width: controls unreadable.
3. A box glyph "☐" prints before every chapter heading (WELCOME, 1., 2., Appendix A-F): a missing
   icon font in WeasyPrint.
4. Bad page breaks: red "How to read the version name" box split over p.5-6; "2.12 SETTINGS Tab"
   heading alone at the foot of p.35; Appendix D table split p.56-57; mostly empty p.30, 53, 55.
5. AMP CAL screenshot cropped through the TX / CALIBRATE buttons (p.25).
6. Explorer screenshots (p.11, 57) are tiny with black redaction blocks; unreadable.
7. Firmware screenshots show an old install path (guide says so; better to retake).

## D. Mixed terms (pick one each)

QRP / AMP off / barefoot / "QRP" button; AMP on / QRO / "Full Power" / AMP table (Full Power
actually means LOWER drive for the amplifier: client label, note for Stew); TUN / TUNE / Tune /
TUNE POWER; host / connected host / radio computer / radio's PC; Phones / Operator /
Operator (phones); Digital / DIG-U / VAC / virtual audio cable / CABLE; USER / GEN / USER/GEN /
general coverage; MSCC for Windows / MSCC NET9 / MSCC WPF / MSCC-NET9; MKII vs MkII (title bar);
park / save / apply; OFFSET vs I/Q OFFSET; Pulse vs "pulse width".

## E. Missing

CAT port: "the other end of the virtual serial pair" is never explained (needs a com0com-style
pair: what to install, which end goes where). No glossary (DIG-U, ALC, I/Q, image, QRP/QRO, host,
CAT, VAC). No first-contact walkthrough. NB / NR: what the controls do and starting values.
FREQ CAL is not stored per radio or in the radio: switching radios on one PC keeps the old PPM
(Appendix D mentions it in one line; deserves a warning in FREQ CAL).

## F. Recommendation

Rearrange and edit, don't start over: the detail and most pictures are good.
1. What MSCC is + glossary
2. Before you start: firmware (Appendix A) + USB driver (A.2)
3. Install and first start (2.1 + Appendix C merged)
4. Operating: SSB, CW, digital short how-tos (WSJT-X steps move here)
5. The window, control by control (2.3-2.17, trimmed; numbered callouts on pictures)
6. Calibration in job order, one chapter, with A.1-A.2 facts (what lives in the radio)
7. Settings
8. Appendices: files, troubleshooting, using this PC as a remote host (or pointer to Remote guide)
Windows guide first; Linux (52 p., same structure) and Remote (24 p.) reuse chapters 1, 4-6.
Write for the client as it is; note to Stew for the confusing labels (Full Power, Phones vs
Operator, USER vs GEN, RX IQ APPLY ALL vs TX IQ RESET ALL) and for item A.2 (reset doesn't stick).
Blocker: no editable source. Ask Stew for the WeasyPrint source (HTML/Markdown) before rewriting;
otherwise rebuild it as Markdown in `docs/manuals/`.
