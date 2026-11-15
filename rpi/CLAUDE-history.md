# CLAUDE-history.md — sdrcore (Pi servers), archived notes

Moved out of `rpi/CLAUDE.md` on 2026-10-31 to keep that file short. Verbatim, in the original
order: the old Open list entries and resume points, then the dated sections.

**Resume point 2026-10-05 (end):** item 17 (spectrum drops carriers). Server half
(`SDRcore-recv-linux/sources/dsputils.c`) built on the Pi, works, committed with the note for
Stew `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-CARRIER-2026-10-05.md` (client fix WPF + Avalonia,
server port to Windows / Ubuntu recv). Client cause confirmed by Ron's window-resize test.
**mscc 1.0.56 built 2026-10-05** (WSL, usual recipe): same 110 files / modes as 1.0.55, only
`sdrcore-recv` (Ron's Pi build 2026-10-05 16:26, largest bin per display point) and the control
version differ. In `rpi/mscc-deb/` and `installers/rpi/` (1.0.55 removed there). Not installed
on the Pi yet (the Pi runs the same hand-built binary). The 14.074 / 14.075 spurs are a rig birdie (Si5351 vs 25 MHz crystal),
not sdrcore. The rig is on the PSoC daughter board.

**2026-10-07: the Pi crashed, Ron booted his backup SD card** and installed `mscc_1.0.56_arm64.deb`
+ `mscc-init_1.0.18_all.deb` on it; servers run (Ron). So the Pi now runs the packaged 1.0.56
binaries, not hand-built ones. A restored backup can carry old packages: this one had
`mscc-init-gui` older than 1.0.15, whose `/usr/bin/mscc-volume-restore` set VirtualA 82% /
VirtualB 86% at every `mscc start` from `~/.local/mscc/volume-levels.conf`. mscc-init 1.0.18
removed the tool, but `mscc-virtual-audio.sh` has a shell fallback that reads that file itself
when the sinks are created, so the file had to go too (Ron deleted it). Without it: 100%.
**Tailscale (2026-10-07):** Stew's client reached the Pi over Tailscale: control, CAT and
spectrum work, receive audio does not (Remote Phones and R-Digital; transmit not reported).
Pi side is right: recv log shows `HOST → 100.113.54.105`, `CTRL enable=1`, no sendto errors.
Suspected, not proven: Windows Firewall on Stew's PC (inbound UDP 9100 on the Tailscale
adapter). Stew is looking at his end; Ron wants no changes until he reports. Check on the Pi:
`sudo tcpdump -ni tailscale0 udp port 9100 or udp port 9101`. Found while reading the code:
recv keeps sending (and keeps the local phones muted) until it gets `CTRL enable=0` or a
restart; nothing notices a client that vanished. Multi-client: ms-sdr is single-session by
design (`Session_Reject`); Ron only asked about the effort, nothing planned.
Tailscale is not on the backup card image: Ron reinstalled it 2026-10-07 (`curl -fsSL
https://tailscale.com/install.sh | sh`, `sudo tailscale up`) and sent Stew the new link, so
the Pi is a new Tailscale device (address may differ from the one Stew used before).
**SD cards that stop booting (2026-10-07, second or third card).** Proven on the Pi: the
Raspberry Pi SD Card Copier leaves `/boot/firmware` (`mmcblk0p1`) UNMOUNTED when it finishes
(`findmnt /boot/firmware` empty, folder looks empty); after a reboot it is mounted again.
Suspected cause of the dead cards, not proven: `apt full-upgrade` run in that state puts the
new kernel in the empty folder on the root partition, the real boot partition keeps the old
one, next boot fails. Rule: after the copier, reboot (or `sudo mount /boot/firmware`) and
check `findmnt /boot/firmware` before any `apt upgrade`. Also: the copy has the same
PARTUUIDs (21382c14) as the SD card, so never boot with both plugged in. Power check
`vcgencmd get_throttled` was 0x0 (short uptime).
Same day on the backup card: a `full-upgrade` had stopped half way with the folder empty;
after the reboot `sudo dpkg --configure -a` copied kernel 6.18.50+rpt to `/boot/firmware`,
`full-upgrade` finished without errors, Ron: "back in business". The card copy made before
that holds the half-finished upgrade; Ron was told to make a new one.
**Resume point 2026-10-08: item 19 (spectrum dB labels squeezed 2:1).** Ron's S9 / S1 test
done: squeeze proven, exact factor not. The note for Stew (with the readings) was committed
and pushed 2026-10-08 with these notes. Waiting for Stew's step test and client fix.
**Later 2026-10-08: item 20 (trans no-mic port of cmd-067) built on the Pi, works; mscc 1.0.57
built, committed afc9af9 (pushed).**
**Then item 22 (S meter average power) built on the Pi, kept; mscc 1.0.58 built; committed and
pushed with these notes.**
**2026-10-09: Ron removed from the list: FREQ CAL STOP test (old item 1), rig receive drops /
J5 (old item 21), the failed SD card.**
**2026-10-09: the Si5351 birdie is on Ron's list, not Stew's (Ron). Next step, not decided:
one-line firmware test (2 mA drive), see `rpi/si5351-drive.md` (has where the birdie will be).**
**2026-10-12: one-line firmware test done: 2 mA drive changes nothing (both lines within 1 dB
of the 8 mA baseline). Drive level ruled out, the `mscc.ini` / opcode plan dropped, line put
back to `0x4F`; normal firmware rebuilt and flashed the same day (Ron). Result is in
`rpi/si5351-drive.md`, not committed. Ron: keep it on his side, NOT told to Stew. Left: hardware
(74ACT74 / 5 V side or the TCXO) or the workarounds (Hi 3.0 kHz, shifting the LO offset).**
**Resume point 2026-10-09 (end):** all Pi work is committed and pushed (ce21277; mscc 1.0.60
installed on the Pi). The notes (`rpi/CLAUDE.md` list changes above, `rpi/si5351-drive.md`
2026-10-09 review + "Where the birdie will be") were committed and pushed later the same day.
**Rule from Ron 2026-10-09: every commit / push carries the current "With Stew" list in the
commit message, in plain words (no bare item / cmd numbers).** Ron's own list now: (a) spectrum level shift / dB CAL after the full-FFT fix,
not measured; (b) Si5351 birdie: decide on the one-line firmware test. Firmware not touched.
Ron asks for "the list, concise" as three groups: Yours / With Stew / Parked.
**2026-10-09 (later): private WPF test build `rpi/wpf-spectrum-test/` (Ron: prove the spectrum
needs no calibration; Stew disagrees with the 2:1 analysis). NEVER commit it: the folder has its
own `.gitignore` with `*`, so git does not see it (ripgrep / the Grep tool skip it too; use
grep in the shell). Copy of the tracked WPF source at 8564843 (R10.8.1) + 5 edits:
`UdpRadioService.cs` `RawYToDb` scale 150 -> 75, bias 40 -> 80, clamp 80 -> 140 (true dB, same
wire data); `SpectrumColorSettings.cs` centre -91.3 -> -86.9, worked out from the recv code
(S meter = 20log(A) - 20; spectrum = 20log(A) + 66.9, Hamming 4096 sum 2212), trim slider +/-20
kept; `SpectrumWaterfallSettings.cs` ini keys `SPECTRUM_DB20_OFFSET` (+ `_HF`, `_LF`) so the
normal client offsets (-91.12 in Ron's ini) are not read or overwritten; tooltips; csproj
`CopyToMsccNet9` target off unless `-p:CopyToMsccNet9=true`. Built (dotnet 9, Release, 0
errors): `src/MSCC.Wpf/bin/Release/net9.0-windows/MSCC.Wpf.exe`, shows version 10.9.0.
`C:\mscc-net9` not touched. NOT run yet. Expected with trim 0: SMSG S9 carrier peak near -73
(carrier between two FFT bins reads up to 1.7 dB low). Ron's 2026-10-08 reading (-87 at
offset -91.12) would instead call for about -81, i.e. the peak would read about -79: 5 to 6 dB
from the calculated value, not explained; the test will show which is right.**
**Run by Ron 2026-10-09, "this is the way the spectrum should look"** (`Downloads\mscc-1.wmv`
old client 10.4.0, `mscc-2.wmv` test build 10.9.0; both on air, 21.074 USB FT8, Pi host, 2
minutes apart). Old: floor about -118, strongest peaks about -102 (16 dB above the floor). New:
floor about -136, peaks about -97 (about 39 dB above the floor). So signals stand about twice
as far out of the noise and the floor reads lower, as intended. NOT checked in these videos:
S9 carrier = -73 (no SMSG carrier in them), so the -86.9 offset is still unproven.
Third video `Downloads/Video_2026-10-09_165009.wmv` (test build, SMSG carrier 14.0672 CW, S
meter S9, Ron: "the slider starts at zero and progresses up ... It is working"). Read frame by
frame: the visible top of the yellow carrier is about -107 for the first 20 s, then rises and
settles at about -87 (floor about -141) from 40 s on. NOT a valid peak reading: the carrier sits
exactly under the white tuning marker, which hides the thin tip of the spike, and the S/W window
(trim value) is not in the picture. Asked Ron for the trim value and to retune a few hundred Hz
so the tip shows beside the marker. Offset -86.9 still unproven; if -87 at trim +20 is the true
peak the fixed offset is about 34 dB too low, which would need explaining.
**Fourth video `Downloads/Video_2026-10-09_165828.wmv` = the valid one** (VFO 14.0669 CW so the
carrier is beside the marker; Ron moved dB CAL 0 -> far left -> far right -> 0). Measured every
2.5 s, counting the white trace line as well as the yellow fill (the third video was read on
the fill only, so its numbers are too low; ignore them). Trim 0: peak -84 to -86.5 (about -85),
floor about -143. Trim -20: peak about -105, floor off the bottom. Trim +20: peak off the top
of the pane (-73), floor -123. So the trim moves everything 1:1 and the scale is linear, but
**at trim 0 the S9 carrier reads about -85, 12 dB under -73: the calculated offset -86.9 is
wrong by about 12 dB (a factor of 4 in amplitude), cause not found.** Checked: ms-sdr passes
the S meter through, the client adds nothing. Note the needle shows S9 for -78 and up (integer
division in `Db_to_Smeter`), so the meter may be up to 5 dB under -73. Offset that would put
S9 at -73: about -75. Not changed yet. S9 vs S1 tracking not tested yet.
**Fifth video `Downloads/Video_2026-10-09_170518.wmv`: SMSG low setting = S4 (Ron: the manual
says S4, not S1), same slider routine.** Meter S4. Trim 0: peak about -110 (-109 to -110.6),
floor -143. Trim -20: -130. Trim +20: -91. So S9 -> -85 and S4 -> -110: the trace moved 25 dB
for 5 S units (30 dB nominal). On the old half scale the same step would be about 12.5 dB.
`Db_to_Smeter` shows S9 for -78..-73 and S4 for -108..-103, so the real meter step is anywhere
from 25 to 35 dB: 25 fits only at the edge (-78 and -103, which would make the offset 7 dB low,
not 12). Cannot be settled from the needle. Proposed to Ron: show the S meter dBm number in the
test build. Not done yet.
**Ron 2026-10-09: no. Stew is adding the dBm number to the S meter (as it was before the WPF
migration). "For now this is good enough": his goal was that the spectrum mirrors the S meter;
the S meter itself needs work, later. Test build left as it is (offset -86.9, reads 7 to 12 dB
low). When Stew's dBm readout exists: read S9 and S4 again, set the constant from the two
pairs, and check they track.**
Results added to `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-DB-SCALE-2026-10-07.md` and pushed
(b9df610); Ron also sent Stew the two videos. Ron: fine for users as it is; only the most
observant will notice the offset.
**Resume point 2026-10-09 (end of day):** everything is committed and pushed except this
paragraph (Ron: no commit / push, just remember). Ron's list: (a) Si5351 birdie, decide on the
one-line firmware test; (b) spectrum centre offset, once the S meter shows dBm (this also
covers the old "level shift / dB CAL after the full-FFT fix" entry); not urgent. With Stew,
new today: put the tested dB scale fix into the real client; dBm number on the S meter.
`rpi/wpf-spectrum-test/` stays on disk, ignored by git.
Rig "dead ears" (old item 21, off the list): Ron 2026-10-09 saw receive go down after a TX,
suspects heat, not sure; it never recovers by itself, a tap on the rig brings it back. He is looking at it himself: nothing for Claude to do, not on the list.
**Resume point 2026-10-12:** pulled Stew's 5 commits (to 547b9ad; Windows client 10.11.3, recv
3.147). He adopted: true dB spectrum scale (scale 75 / bias 80), strongest point per pixel,
spectrum smoothing (Pi `panadapter.c` as it is), LO point levelling + after-TX hold + gentle DC
blocker, average-power S meter, dBm on the S meter (click the readout). His own: dB CAL centre
-83.6 from a generator step test (not our -86.9), resolution fixed at 3200 (selector gone),
smooth needle, S units rounded to nearest. Compared Windows recv with the Pi: `panadapter.c`
identical, spectrum / S meter code in `dsputils.c` / `sdrcore.c` the same, wire format
unchanged = no Pi port needed (Pi recv still version 143; the new client is NOT run against
the Pi yet). Not in the pull: ALC button state in the client, REFRESH as smoothing control.
Stew's note `.mscc-coord/NOTE-FOR-RON-LO-POINT-LEVELLING-2026-10-11.md` (on hold, nothing to
do): the levelled slice shows as a lighter waterfall band with edge lines; a signal at the LO
point is pushed down in about 10 s and leaves a hole for about 20 s; a fixed narrow fill was
discussed, not adopted (hump width differs per shack). Parked.
Still NOT committed: the MSCC Volume work (item 28: `rpi/mscc-volume/`, mscc 1.0.63,
mscc-volume 1.0.0, `mscc.sh`, `mscc-virtual-audio.sh`, the deleted Grok volume files); waits
for Ron's Save / Forget reboot test. Ron's list: that test; Si5351 birdie firmware test
decision; spectrum centre offset against Stew's dBm readout. Same day: ALC review 2 and the
SWR meter ideas (ALC outer loop, automatic power cal), under item 23.
**Ron will install Stew's WPF 10.11.3 on 2026-10-13** (`mscc-net9-R10-11-3-install.exe`). Told
him to expect: first start resets dB CAL, GRID MIN and waterfall LOW / HIGH once (floor near
-140, signals further out of the noise); the resolution selector is gone (fixed 3200); click
the S meter readout for dBm; the Pi needs nothing new (mscc 1.0.63 has the matching code), but
this is the first run of that client against the Pi. Check asked for: S9 generator carrier
about -73 dBm on the S meter and about the same on the spectrum = settles "spectrum reads
lower than the S meter" on Ron's list. Results not in yet.
MSCC Volume Save / Forget buttons: still NOT tested (a stray "test done, works" line appeared
in a prompt on 2026-10-12; Ron did not run it). In lists say "Save and Forget buttons".
**Resume point 2026-10-12 (end, second session):** five notes for Stew pushed today: 4cf98ef
(REFRESH becomes SMOOTHING), b2ec47f (10.11.3 spectrum / S meter check), cbd6dea (RX IQ tab,
four client points), 7254510 (receive I/Q defaults per band, idea), 6bc3628 (BASELINE per
band, idea). Ron's list: the MSCC Volume Save and Forget buttons; optional receive I/Q on
80 / 40 / 30 m with UP 24 kHz. 20 m receive I/Q is 58 (checked). Local only, NOT committed (Ron: "don't update anything"):
the MSCC Volume work, `rpi/si5351-drive.md` (birdie test result), the Post Build fix in
`radio-psoc-firmware/Proficio-MKII-PTT` (`.cyprj`, `POST-BUILD.txt`), PSoC Creator's build
output and new `Release` files, and the notes in this file written after b2ec47f.
**RX IQ tab reviewed 2026-10-12 (Ron asked how to use it; code only, nothing run, nothing
changed).** Path: WPF `MainViewModel.cs` `StartRxIq` (0x58 band, 0x55 RX, main freq) ->
ms-sdr `iq.c` -> recv `udp_thread.c` / `iq.c`. Correction is amplitude only: `IQ_calc`,
0.05% per step on I (value >= 0) or Q (value < 0), slider +/-200 = 10%, applied in
`dsputils.c` `framesToComplex`; no phase term. Slider acts live (recv RAM), APPLY (0x57)
writes `~/.local/mscc/recv-iq.ini` for that band; an unsaved value stays in use until a band
change or restart. ZERO saves 0 at once; RESET ALL copies the slider value to all bands.
UP 24 kHz puts the passband on the image; LO FINE then needs about 2 x the tone pitch.
The session leaves the dial where it was tuned. **Possible fault, not tested:** START sends
the band (0x58) before the RX / TX select (0x55), not awaited; after a TX IQ session ms-sdr
still has `rx_tx` = TX, so the band would take the TX path (mode 'T', cal frequency, sent
to trans) and recv would not get the band. Not told to Stew.
**Done on 20 m with Ron, 2026-10-12 (WPF 10.11.3, Pi host, SMSG S9, dial 14.067 CW):** image
of the 14.0672 carrier at 14.0415. Saved value 58 gave -119 (43 dB under the carrier at
-76); the null is flat from about 76 to 99 at -130 to -132 (54 to 56 dB, at the noise
peaks); 87 applied. `recv-iq.ini` record 4 (20 m) = 87, confirmed in the file and the recv
log. All 11 other records are 58 (one value once copied to all bands), not calibrated per
band. A line at 14.0423 (-120) does not move with the slider: rig spur. Client faults seen:
the START button never shows ACTIVE (local `Content="START"` in the XAML beats the style
trigger; it only goes dim); the APPLY box defaults to No. The Pi's `~/.local/mscc` is
readable from the PC as `V:\ini-files` (ini files and the three server logs).
**CORRECTION, same day: 87 was the wrong value; 58 is right and is saved again (file and
logs checked, 18:14:53 UTC; all 12 records are 58).** My steps had Ron null the image of an
in-passband carrier, which falls 25 kHz below and never reaches the audio. The image that
matters is the one that lands in the passband, from a signal below the LO point: that is
what UP 24 kHz is for (dial +24 kHz; LO FINE +1200 in CW puts it on the pitch).
`Downloads/rx-iq-3.wmv` (UP 24 kHz, LO FINE 1200, dial 14.0922, sweep -127..112): in-passband
image -126 at 52..59 (50 dB under the carrier), -116 at 87 (40 dB), -111.5 at 0 (36 dB);
60..67 not read (pointer over the slider). So the two sides null at different values (87
against 58); cause not known (amplitude-only correction). **Correct procedure: START, tick
UP 24 kHz, LO FINE about 2 x the pitch, null the line at the tuning marker (or the S meter),
APPLY with the mouse (box defaults to No), check `recv-iq.ini`.** Ron also pressed ZERO twice
on the way (saves 0 at once; ms-sdr log 18:03 and 18:04 UTC).
**Idea, Ron 2026-10-12 ("I will think about it", nothing designed or changed):** strong
signals are hard to find on every band (SMSG has 80 / 40 / 30 / 20 only), so fill the
uncalibrated bands from the calibrated ones. Ron proposed the average; my answer: the built-in
table `iq_defaults_PCB` trends with frequency (-3 on 160 m to -104 on 10 m), so nearest
calibrated band is the better rule; test first = calibrate 80 / 40 / 30 and see whether the
values trend. Needed either way (Ron): a mark for which bands have been calibrated; none
exists. My suggestion: recv adds `,CAL=1` to that band's line in `recv-iq.ini` on APPLY
(the reader finds fields by name, so old files still load; all bands start unmarked).
Ron's second version, same day (still thinking): Stew measures a default per band, RESET
writes the defaults to the bands without `CAL=1`. My answer: better than average / nearest
band, and it fits what exists: TX already has `factory/iq/<line>/iq.ini` per radio model,
and Windows once had per-PCB `rev-XXX/recv-iq.ini` (the built-in `iq_defaults_PCB` is such a
table). Open: unit-to-unit spread (Ron's 20 m is +58, the built-in table says -43, table's
origin unknown), so Stew would need two units of a model to see if a default is worth it.
Ron: he thinks the built-in table is from an earlier Proficio version; Stew would have to
measure every model he will support. Written up for Stew as an idea ("he can decide"),
committed alone and pushed 2026-10-12 as 7254510:
`.mscc-coord/NOTE-FOR-STEW-RX-IQ-BAND-DEFAULTS-2026-10-12.md`.
**Idea, Ron 2026-10-12: S/W BASELINE kept per band. Note for Stew committed alone and
pushed as 6bc3628: `.mscc-coord/NOTE-FOR-STEW-BASELINE-PER-BAND-2026-10-12.md`.** Ron
re-adjusts only BASELINE when going 20 m -> 15 m (noise higher on 15). From the code:
BASELINE is client only (`SpectrumColorSettings.GetDisplayGridWindow`: raises the bottom of
the dB window, 0.3 dB per step on Proficio, labels stay true), one value per radio family
(HF / LF bank), not per band; the per-band last-used record would be the place. Ron's dummy
load check: noise almost identical on 20 and 15 m = the rig is the same on both, the extra
15 m noise comes in through the antenna.
**Private note, Ron + Claude only, NOT for Stew (Ron 2026-10-12): receive I/Q phase
correction. Idea only, nothing built, not planned.** Today's correction is amplitude only
and the in-passband image stops at about 50 dB down (20 m, 58). Adding phase: recv
`dsputils.c` `framesToComplex` one extra line (Q += p x I, next to `iMult` / `qMult`); a
second value per band in `recv-iq.ini` read by name (0 = as today); one new opcode through
ms-sdr like 0x52; client second slider on the RX IQ tab under the same APPLY (Stew);
Windows / Ubuntu recv the same lines (Stew). One gain + one phase value per band is enough
because the passband is always 12 kHz from the LO point. Harder parts: two knobs interact
(null one, then the other, repeat); gain unknown, the image is 50 dB down and the noise
about 62 dB down with an S9 carrier, so at most 12 dB to see without a stronger signal.
My estimate, not measured: an afternoon for the Pi side. Cheap trial offered and turned
down for now: phase value from the ini only, no opcode, no client change.
Ron: the RX IQ RESET ALL button should be labelled APPLY ALL (TX IQ RESET ALL is a real
reset, keep). Ron also wants START to tick UP 24 kHz by default. Note for Stew with these four client
points, committed alone and pushed 2026-10-12 as cbd6dea (this file was NOT in that commit:
it holds the birdie test notes Ron wants kept local):
`.mscc-coord/NOTE-FOR-STEW-RX-IQ-TAB-2026-10-12.md`. The possible START-after-TX-IQ fault is
NOT in that note (not tested).

2. cmd-046: TUNE power separate (Windows/Ubuntu trans + WPF). With Stew; no Pi change.
3. RF check of remote TX audio (Stew, spectrum analyzer).
4. cmd-048 (Stew): port Pi QRP/QRO power cal to Ubuntu/Windows; Windows factory-seed choice -> Ron.
   What it is (Ron asked 2026-10-12, he had forgotten; say it this way): the Pi power cal
   rework of 2026-09-28. trans owns `power_cal.ini` (before, ms-sdr wrote it and trans
   reloaded everything per slider step); a cal slider step changes RAM only and the file is
   saved once the slider is still (1 log line per step, was about 70 + 7 file operations);
   same for the amplifier (QRO) cal slider, `amplifier.ini` dropped. Ron's Pi test: 2 bands,
   "power response very good", QRP + QRO cal smoother; in mscc 1.0.50+. Windows / Ubuntu
   still the old way, same file format. Details: the two 2026-09-28 sections at the end.
5. cmd-050 (Stew): Ubuntu kit/docs/drop-installers.sh for the mscc-init rename (below).
6. Done 2026-09-29: mscc-init 1.0.18 installed on the Pi, works (Ron).
8. Spectrum -12 kHz DC-spur notch sized in FFT bins (2026-09-29, `SDRcore-recv-linux/sources/panadapter.c`,
   was fixed 2 px, spur showed at PAN RESOLUTION 1600/3200). MP_HALF_BINS 3 tested on the Pi: better, a
   trace remained -> 4: still a ~5 dB skirt bump + faint waterfall line (screenshot) -> 6 (2026-09-29):
   **tested on the Pi, OK (Ron; "a bit blotchy but OK").** Committed b08af65. Stew brief cmd-053 (Windows + Ubuntu recv) pushed fe5cabb. PAN -> SPECTRUM label rename: cmd-054 (WPF only, label text).
   Blotchy fix (2026-09-29): 4 px avg each side tested = better, not enough -> A+B: fill level smoothed over frames (MP_LEVEL_ALPHA 0.2) + noise texture mirrored from neighbours; tested on the Pi 2026-09-29, Ron OK. Committed ef1d612; cmd-053 brief already describes this fill (checked 2026-09-30).
   Stew status (2026-09-30): Windows recv 3.142 done with the Pi fill (live spectrum smoke not run, not pushed); Ubuntu `linux/` running.
9. Stew's `.mscc-coord/QUESTIONS-FOR-RON.md` answered (ce731ee, 522ea21). Ron's position (#4): trans
   owns power_cal.ini. #6 (2026-09-30): Ron said YES to detection-driven auto-restore for
   cmd-055 (Stew swaps Ultimus/Geminus often) instead of the manual switch-radio tool; trans
   still owns power_cal.ini, ms-sdr must not overwrite it. #5 second-STOP: Stew pushed
   cmd-056 (ms-sdr 3.177); ported 2026-09-30 to rpi/ms-sdr-linux calibrate.c (abort while
   cal_abort_pending = log only, drain kept). **Built on the Pi, works (Ron 2026-09-30).** Committed 8889d77.
10. mscc-portaudio 19.8.3 (2026-10-01): `apt full-upgrade` showed "ldconfig: /usr/local/lib/libportaudio.so.2
   is not a symbolic link". 19.8.2 shipped 3 full copies (links lost via Windows). `build-deb.sh` now ships
   `libportaudio.so.19.8` + makes the `.so.2` / `.so` links; same binary (sha c4f6c2b9...). Build needs
   `PORTAUDIO_ROOT=<worktrees>/portaudio` (default `rpi/portaudio` doesn't exist). In installers/rpi
   (19.8.2 removed). **Installed on the Pi 2026-10-01, `sudo ldconfig` silent (Ron).** Committed 316d005, pushed.
11. **Per-radio cal (park / restore) ported to the Pi 2026-10-03, Ron: "just make it work".**
   Stew built it his way on Windows (cmd-055/059/062, ms-sdr 3.181, WPF 10.2.0) and Ubuntu (cmd-061):
   ms-sdr copies the files; client "Save settings" sends `0x29 CMD_SET_PARK_CAL_SETTINGS` to the host.
   Pi port = Ubuntu `factory_seed.c/.h` copied into `rpi/ms-sdr-linux/source/` + the same hooks
   (Makefile, `usbavrcmd.h` 0x29, `main-controller.c` case, `main.c` seed at version read + reload
   when the network is up). One Pi difference, in `factory_seed.c`: Save settings first sends
   trans `CMD_SET_SDRCORE_TRANS_INITIALIZE` + 300 ms wait, so trans writes any pending slider
   value before the copy.
   **`CMD_SET_IQ_DEFAULTS` now works as on Windows (Ron 2026-10-03: "exactly like Windows"):**
   trans `udp_thread.c` no longer deletes iq.ini on 0x8D, it re-reads the live file (built-in
   only if missing); ms-sdr `iq.c` TX Reset All copies `factory/iq/<line>/iq.ini` over the live
   file first; the reload after a radio swap sends 0x8D. So **ms-sdr and sdrcore-trans must be
   updated together** (new ms-sdr + old trans = swapped-in iq.ini deleted), and TX Reset All
   needs `/usr/share/mscc/factory` (mscc 1.0.51+ package); without it Reset All does nothing.
   **Rest of the Windows state ported too (Ron 2026-10-03: "same state as Windows"):** ms-sdr
   `calibrate.c` CAL_RESET loads `factory/freq/<line>/freq_cal.ini` (fallback `Create_PPM_ini`);
   trans no longer creates power_cal.ini when missing (`Create_power_cal_file` removed from
   `main.c` / `power.c` / `extern.h`; built-in table in RAM until ms-sdr seeds the file and
   sends INITIALIZE). sdrcore-recv unchanged.
   **Pi-only safety additions in `factory_seed.c`** (Ron: "make sure it does not trash anything"):
   `copy_file` writes `<dst>.tmp` then renames (no half-written file); when a live file replaces a
   different parked file (radio-swap stash or Save settings) the old parked file is kept as
   `<leaf>.bak`. Windows / Ubuntu have neither.
   Trash audit, run in a WSL harness with the real `factory_seed.c` (first start after upgrade,
   same radio, Save, swap to a new radio, swap back, resets, LAST_LINE.txt lost, no factory tree):
   user cal survived every case except one in Stew's design: **if `cal/LAST_LINE.txt` is lost
   while the live files belong to a different radio than the one connected, the live files are
   taken as the connected radio's, and at the next radio swap they overwrite that radio's parked
   copy.** On the Pi the `.bak` keeps the good copy; on Windows/Ubuntu it is gone. Not told to
   Stew (Ron's choice). Also: with no factory tree, a never-seen radio type keeps the previous radio's
   iq/power files live (nothing lost, wrong cal until calibrated).
   Files: live `~/.local/mscc/`, parked `~/.local/mscc/cal/<line>/` (iq.ini, power_cal.ini,
   amplifier_cal.ini), `cal/LAST_LINE.txt`, factory `/usr/share/mscc/factory/` (in the mscc .deb
   since Stew's 1.0.51 packaging change; without it only new-radio seeding is skipped).
   **Built on the Pi 2026-10-03 (ms-sdr + sdrcore-trans), Ron's Proficio MKII: first start logged
   `Factory_seed. major=3 line=proficio-mkii` + 3 `cal bootstrap` lines; WPF Save settings logged
   `Factory_park. done line=proficio-mkii major=3 files=3`.** Radio swap not testable (Ron has one
   rig). Pi still has mscc 1.0.50, so "factory tree missing" is logged (resets fall back / do
   nothing until a package with `factory/` is installed).
   **mscc 1.0.52 built 2026-10-03** (WSL; LF export of the git index + the new Pi ms-sdr and
   sdrcore-trans; sdrcore-recv as 1.0.49+; same file list and modes as Stew's 1.0.51, incl.
   `factory/`). In `rpi/mscc-deb/` and `installers/rpi/` (1.0.51 removed there). **Installed on
   the Pi 2026-10-03 (Ron): log shows `cal keep live` x3 + `keep live freq_cal.ini`, no "factory
   tree missing".** Resets not pressed (would reset Ron's cal).
   **mscc 1.0.53 (2026-10-03):** same servers as 1.0.52; only change = packaged
   `usr/share/mscc/bin/bootloader-gui` now has Stew's cmd-063 "Load File opens in
   /usr/share/mscc/firmware" (copied from `rpi/psoc-usb-bootload-linux/bootloader-gui.py`; the
   package copy is separate and Stew had not updated it). In `rpi/mscc-deb/` and `installers/rpi/`
   (1.0.52 removed there). Not installed on the Pi yet.
   `installers/rpi`: `mscc-init-gui_1.0.17_all.deb` removed again (Stew re-added it);
   `mscc-firmware_1.0.0_all.deb` (Stew, files only under /usr/share/mscc/firmware/<Radio>/, 8 radios)
   kept, INSTALL.md lists it as optional; not installed on the Pi yet.
   Note for Stew: `.mscc-coord/NOTE-FOR-STEW-UBUNTU-IQ-INI-2026-10-03.md` = ONLY the Ubuntu
   iq.ini delete (Ron 2026-10-03). Ron chose NOT to tell Stew about the LAST_LINE overwrite
   case or about his three commits in rpi/ + installers/rpi; don't raise them with Stew. Build recipe that works: `git -c core.autocrlf=false checkout-index` of
   mscc-deb/build-deb.sh + packaging, mscc-binaries, mscc-init-files-linux, tty0tty-master/module
   and factory into a temp dir, copy to WSL /tmp, files 644 / dirs 755 / index-755 files 755,
   run build-deb.sh. Committed c3c48a5 (port + 1.0.52), 0882e3f (1.0.53).
   To test: build ms-sdr AND sdrcore-trans on the Pi; log should show `Factory_seed. major=N line=...` and
   `cal bootstrap live→line=...` on first start; WPF Save settings -> `Factory_park. done ... files=3`.
   Then a new mscc .deb (1.0.52) with the new ms-sdr binary.
   Stew's push also touched `rpi/` (mscc-deb build-deb.sh + control, mscc_1.0.51_arm64.deb,
   psoc-usb-bootload-linux default folder) and put `mscc-init-gui_1.0.17_all.deb` back in
   installers/rpi (we replaced it with mscc-init 1.0.18) and added `mscc-firmware_1.0.0_all.deb`.
   Ron has not ruled on those.
12. ms-sdr thread names (2026-10-03, Ron asked): `Set_Thread_Name()` in `platform_linux.c` /
   `platform.h` (`pthread_setname_np`, max 15 chars), called first thing in each of the 11 thread
   functions: cmd-processor, gui-send (main-controller.c); log-flusher, last-used, ptt-switch,
   key-status (main.c); temperature; freq-queue; cat-port, cat-pin-check (comm-port.c); swr-meter.
   **The main thread is NOT named on purpose:** its name is the process name that `mscc.sh`
   finds with `pgrep -x ms-sdr` / `pkill -x`. See them with `top -H -p $(pidof ms-sdr)` or
   `ps -T -p $(pidof ms-sdr)`. Syntax-checked + helper run in WSL (names show in `ps -T`).
   **Built on the Pi 2026-10-03 (Ron): `ps -T` shows ms-sdr, libusb_event (libusb's own) and 10
   named threads; `last-used` is not in the list because its `pthread_create` in main.c (~1228) is commented out: that thread is never started.** Committed 67d4260.
   sdrcore-trans / recv threads not named (not asked).
13. **-12 kHz spectrum spur removed in the DSP (2026-10-04).** `SDRcore-recv-linux/sources/dsputils.c`
   `framesToComplex`: DC blocker on raw I and Q, `y = x - x_prev + DC_BLOCK_A * y_prev`,
   `DC_BLOCK_A 0.98f` (-3 dB at ~300 Hz). `panadapter.c`: pixel notch + fill code REMOVED (Ron
   2026-10-04; old code is in git, b08af65 / ef1d612). **Built on the Pi with the notch switched
   off, Ron: FT8 on 21.074 OK, "no spike, no discernible dips"; still clean with the full FFT.**
   Committed 8f09661 (pushed), with the full FFT and the cleanup (14).
   Why 0.98: 0.9999 (1.5 Hz) was tested first, spur came back with the notch off. Raw I/Q recording
   (servers stopped, `parec` from the Proficio source, dummy load, 8 s; `sdrcore-recv` holds the
   device directly, so parec gives 0 bytes while it runs): signal ~1 count rms of 16 bit, DC -0.5
   count, a comb at 16.08 Hz + harmonics (source unknown), noise rising toward DC, still above the
   floor ~300 Hz out. Emulated display: hump +27 dB at DC, 0.9999 -> +23, 0.99 -> +5, 0.98 -> ~0.
   Blanker (`blanker.c`, envelope-relative) not affected; audio / FREQ CAL 12 kHz away.
   **Spectrum FFT was half empty, fixed 2026-10-04 (Ron OK'd).** Blocks are 2048 frames but
   `doPanadapter` copied 4096 from `incplx`; the upper 2048 were never written (zeros), so the 4096
   Hamming was cut off at its peak -> wide skirts on every strong signal (-21 dB at 6 bins vs -43 or
   better). Now `pan_hist[4096]` in `doPanadapter` keeps the previous block (updated every call,
   also on the early return). Display only; audio/S-meter/cal use `fastconv`. Expected: FFT
   magnitude x2 on carriers, x1.41 on noise = about +3 / +1.5 on the client scale (client dB is
   `Y/150 - 40` = 10*log10(mag), `RawYToDb`), so NOT the 6 / 3 dB first told to Ron -> client dB
   CAL redo. Not measured yet. **Built on the Pi, Ron: "signals look
   narrower, audio OK".** In 8f09661. Windows/Ubuntu recv have the same code.
   2026-10-04 "no signals on the spectrum" after an `mscc.sh stop` / start was a bad server start,
   fixed by restarting again; not the blocker.
   Notch removal built on the Pi 2026-10-04, Ron: "spectrum looks the same".
   Open: level shift / dB CAL not measured yet (Ron, or Stew per cmd-064).
15. **mscc 1.0.54 + brief cmd-064 (2026-10-04).** `mscc_1.0.54_arm64.deb` built in WSL with the
   usual recipe (LF export of the index): new `sdrcore-recv` (Ron's Pi build 2026-10-04: DC
   blocker + full FFT, no notch; checked with nm / DWARF) and new `ms-sdr` (thread names, Ron's
   Pi build 2026-10-03, he had forgotten to copy it for 1.0.52/53); `sdrcore-trans` unchanged.
   Same 110 files / modes as 1.0.53, only those two binaries differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.53 removed there). Not installed on the Pi yet (the Pi runs the same
   hand-built binaries). Pi recv is still `VERSION_MINOR 141` (Windows/Ubuntu 142, 143 after
   cmd-064); not bumped, Ron not asked yet.
   Brief `.mscc-coord/briefs/cmd-064.md` + COMMANDS.yaml entry: Stew ports the DC blocker, the
   full FFT and the notch removal to Windows + Ubuntu recv (3.143), measures the level shift.
14. **SDRcore-recv-linux cleanup (2026-10-04, Ron: .o files, anything Windows, clutter).** Tree is now
   Linux only: Makefile, 2 .md, udp_smoke.sh, `sources/` = the 15 built .c + mscc_resampler +
   `resampler/` + their headers + `portaudio.h` (kept: WSL syntax check falls back on it; the Pi
   uses /usr/local/include). Deleted (54 tracked files, restorable from git, plus 24 untracked .o):
   pthreads-win32 `pthread.h`/`sched.h`/`semaphore.h`, `sdrcore-recv.c` (old Windows main),
   `tonegen-old.c`, `panadapter - Copy.c`, `getwav.c`, `sendwav.c`, `wavfmt.h`, `mfc.h`,
   `portaudio_stub.c`, `log_msg.c/.h`, all `.bak-*` / `.copy`, `backup-nr/`, unused duplicate
   `sources/include/`, root `tonegen.c`, empty `core`, x86-64 `sdrcore-recv` binary.
   Windows branches stripped from built files: `extern.h`, `platform.h`, `platform_linux.c`,
   `main.c` (My_getenv, host API pick, speaker list), `print-utils.c`, `udp_thread.c` (bad recv =
   continue), `sdrcore.h` + `dsputils.c` (WIN32 / WINDBG), `_CRT_*` defines, Makefile comment.
   Kept on purpose: the Linux versions of Windows names in `platform.h` (Sleep, MessageBoxA,
   SOCKET, WSAStartup...), still used all over. All 15 .c + 9 .cpp syntax-check clean in WSL with
   the Makefile flags. **Built on the Pi 2026-10-04, Ron: "spectrum and audio OK".** In 8f09661.
   SDRcore-trans-linux has the same kind of leftovers (VS project files, `libs/` Windows libs,
   pthreads-win32 headers, .bak files): only listed, nothing removed (Ron said stop; he meant recv).
16. **800-bin spectrum fix ported from Ubuntu (2026-10-05, Stew's `.mscc-coord/NOTE-FOR-RON-rpi-pan-800.md`).**
   `ms-sdr-linux/source/user_controls.c` `CMD_GET_SET_PANADAPTER_REFRESH`: was `< 1` -> 6, so index 0
   (800 bins) never reached recv; now only `> 10` -> 6 (0/1/2 = 800/1600/3200, 3-10 = refresh blocks).
   Recv unchanged. Settings load (line ~406) still turns a saved 0 into 6, same as Ubuntu.
   **Built on the Pi 2026-10-05, Ron: "800 works".** Committed 10d90bf (pushed).
   **mscc 1.0.55 built 2026-10-05** (WSL, usual recipe): same 110 files / modes as 1.0.54, only
   `ms-sdr` (Ron's Pi build 2026-10-05) and the control version differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.54 removed there). Committed accd904 (pushed). Not installed on the Pi yet.
17. **Spectrum: a steady carrier vanishes at some dial settings (Ron, 2026-10-05, video
   `Video_2026-10-05_160243.wmv`, Pi host, 800 bins, generator carrier ~14.067212, dummy load).**
   Seen: carrier strong at VFO 14.066 / .071 / .073 / .074 / .076, weak at .067 / .069, gone at
   14.070 and 14.064 (only its noise pedestal left), steady for 20 s, so not a tuning transient.
   **Cause found in the code, not yet proven by a test:** `doPanadapter` (`dsputils.c`) takes ONE
   FFT bin per display point: `fstep = (nfft - 1024) / pixels` = 3.84 bins per point at 800,
   `bpos = (int)fpos`, the bins in between are never looked at. A carrier is about 4 bins wide
   (Hamming), so it can sit between two sampled bins. Python model of that loop: up to 49 dB
   loss at 800, up to 7 dB at 1600, none at 3200; it gives "gone" at 14.070 but does not match
   every setting (it is sensitive to about 10 Hz of carrier frequency). Visible only since the
   full-FFT fix of 2026-10-04 (item 13): the half-empty FFT gave skirts wide enough to hide it.
   **Second video (`Video_2026-10-05_161614.wmv`: pass 1 at 3200, pass 2 at 800): the carrier
   also vanishes at 3200** (gone at 14.046, .049, .064, .070, weak at .054, .057, .059, .067),
   so the 3200 prediction above was wrong as stated. What holds in both passes: every "gone"
   setting puts the carrier at 212 Hz above a multiple of 3 kHz from the LO point (39.2, 33.2,
   30.2, 15.2, 12.2, 9.2 kHz), and 3 kHz is exactly 128 FFT bins (96000 / 4096 = 23.4375 Hz),
   so there the carrier sits dead centre on a bin = its narrowest (3 bins). Only the FFT grid
   knows about 3 kHz, so it is still a sampling artifact, not the radio. Reading: at 800 the
   server skips bins; at 3200 the server sends every bin, so the skipping must then be in the
   CLIENT (2400 points drawn across about 676 pixels, apparently one point per pixel, no max).
   **Confirmed in the WPF source** (`MSCC.Wpf/Controls/SpectrumDisplayControl.xaml.cs`, draw
   loop about line 725): one data point per screen pixel, `idx = (int)(dataFrac * (data.Length
   - 1))`, `data[idx]` only, no max over the points in between. Fix therefore has two halves: server (max over the bins of a
   point) and WPF / Avalonia client (max over the points of a pixel; Stew).
   **Server half made 2026-10-05, NOT built on the Pi yet, not committed:** `dsputils.c`
   `doPanadapter` now takes the largest of all the bins a display point covers (3.84 at 800,
   1.92 at 1600, 1 at 3200). WSL syntax check clean. Python model of old vs new loop, 350
   random carriers in the displayed span: old at 800 = 132 of them down more than 10 dB (worst
   -76), new = none down at all, at every resolution. Side effect to expect: at 800 / 1600 the
   noise floor reads a little higher (largest of several noise bins), so the level differs
   slightly between resolutions; not measured. Client half (Stew) not done, so 3200 still drops
   carriers until the client is fixed.
   **Built on the Pi 2026-10-05, Ron's video `signal.wmv` (800 bins): carrier now shows at
   14.046, .049, .064, .067, .070 (all gone before). Still lost at 14.040.** Reason worked out,
   not tested: the 800 points are the 72 kHz on screen (bins 1024-4095), and the client draws
   them across about 676 pixels with one point per pixel, so about 124 of the 800 points are
   never drawn (roughly every 6th). At 14.040 the carrier is at point 702, and for a 676 pixel
   wide plot point 702 is one of the skipped ones (pixel 593 -> point 701, pixel 594 -> 703).
   Which points are skipped depends on the window width, so resizing the window should bring it
   back. **Tested by Ron 2026-10-05 (PSoC board, 800 bins, VFO 14.040): resizing the client
   window makes the carrier come and go = the client cause is confirmed.** Client fix (Stew) is the real cure; a server-side workaround would be to spread each
   point's maximum into its neighbours. Also: `doPanadapter`'s shuffle never uses FFT bin 0 (DC)
   and uses bin nfft/2 twice; harmless, noted only.
   Old test idea: PAN resolution 3200, carrier must show at 14.070 and 14.064. Proposed fix (not made):
   per display point take the largest magnitude of all the bins it covers. Windows / Ubuntu recv
   have the same loop (Stew). Still unexplained in the same video: two spurs 16 kHz either side
   of the LO point at VFO 14.074 / 14.075 only (one just right of the passband).
   **Spur pair explained, by fit, not yet proven (Ron's video `spurs-800.wmv`, 2026-10-05, rig
   back on the PSoC board, FW 3.232, 800 bins, 1 kHz steps 14.040 -> 14.079).** So it is NOT the
   pill. Measured from the LO point (VFO - 12 kHz): VFO 14.073 one spur at +47.25 kHz; 14.074 a
   pair at -15.4 / +15.2; 14.075 a pair at -16.8 / +16.6; 14.076 one at +47.25; none at any
   other setting. Level -105 to -108, about the same as the carrier (-104). All four fit one
   line: spur = 32 x (VFO - 14.074478 MHz), shown only while under 48 kHz (above that the codec
   filters it out), i.e. VFO within about 1.5 kHz of 14.0745. Zero beat is LO = 14.0625 MHz =
   25 MHz x 9/16: the Si5351 output there (4 x LO, `si5351a.c`) is 56.25 MHz = 9/4 of its 25 MHz
   crystal, and 8 x 56.25 = 18 x 25 = 450 MHz. So a synthesizer / crystal harmonic beat (birdie),
   32 kHz of movement per 1 kHz of dial. The pair is symmetric about the LO point = a real
   (not quadrature) signal in the I/Q, so it does not come in as RF through the mixer. The pill
   video's 16.0 kHz at 14.074 fits too. **Ron, 2026-10-05: generator off at 14.074, the spurs
   are still there = made in the rig, not a product of the test signal.** Not a display or
   server fault; nothing to fix in sdrcore. Predictions to test: stays with the generator off; in
   100 Hz steps it moves 3.2 kHz per step and crosses the LO point near 14.0745; same kind of
   birdie on other bands where LO x 16 = 25 MHz x n.
   **Third video (`more-spurs.wmv`, 2026-10-05, PSoC board, 3200, CW mode, 100 Hz steps
   14.060 -> 14.086, generator about 20 dB stronger, plot 745 pixels wide): "spurs coming and
   going" = four different things, measured against the carrier's position in every frame.**
   (a) Mirror image of the carrier about the LO point (carrier + image = constant in all 11
   samples), about 22 dB under the carrier: receive I/Q balance, real. (b) A copy of the carrier
   24.2 kHz above it (constant in 12 samples; 24.0 = a quarter of the 96 kHz sample rate, within
   the scale error), about 25 dB under: cause NOT known (generator sideband or something with a
   4-sample period in the I/Q stream). (c) The 14.0745 birdie racing across between VFO 14.0737
   and 14.0756, 32 kHz per kHz as predicted; in CW its zero beat is at VFO 14.0751 (LO point is
   VFO - 12.6 kHz in CW). (d) The coming and going itself: 2400 points on 745 pixels, the client
   draws 1 point in 3, so every narrow line (and the carrier: it reads anywhere from -87 to
   -109) flickers as it moves = the client fault in the note for Stew. Also two weak fixed lines
   at 7.0 and 12.0 kHz below the LO point (-113), not looked into.
18. **Rig spurs: separate line of investigation (Ron, 2026-10-05). Parked, nothing to fix in
   sdrcore.** All real signals in the I/Q, seen on the PSoC board (FW 3.232); measurements and
   videos are under item 17. Not the spectrum display fault (that one is item 17, client side).
   - **Birdie at VFO 14.0745** (14.0751 in CW): Si5351 (4 x LO = 56.25 MHz) against its 25 MHz
     crystal, 32 kHz per kHz of dial, stays with the generator off. Hardware (Stew). Open: how
     it gets into the audio; the list of other bands / frequencies (16 x LO = 25 MHz x n).
     **2026-10-06 video `Video_2026-10-06_215238.wmv` (generator off, dummy load, PSoC board,
     VFO 14.074 USB): pair at 15.27 kHz either side of the LO point, steady, about 10 dB over the
     noise = not off air, not the generator.** The upper one (14.0773) is inside the passband
     with Hi 4.0 kHz = a tone at about 3.3 kHz audio on 20 m FT8. Prediction not tested: the
     slope of 32 means 32 x LO = 25 MHz x k, so LO = 0.78125 MHz x k; in-band USB dial settings
     (LO + 12 kHz): 7.04325 (k 9), 21.10575 (k 27), 28.137 (k 36), 28.918 (k 37), 29.6995 (k 38).
     **Ron 2026-10-06: 40 m USB dial 7.043, the pair is there too** (position not measured;
     15 m / 10 m not tested). Options given to Ron: Hi 3.0 kHz workaround; software = move the
     12 kHz LO offset by about 3 kHz inside the +/-1.5 kHz windows (ms-sdr, recv, trans, client
     scale, all platforms; not designed, nothing changed); hardware. Note for Stew written:
     `.mscc-coord/NOTE-FOR-STEW-SI5351-BIRDIE-2026-10-06.md`.
     Ron's idea for new rigs: a different TCXO (part swap + crystal value in firmware). Table
     of in-band birdie points per reference added to the note (calculated only): 26 MHz best
     (21.137, 28.450, 29.262 only); 27 MHz bad (10.137 = 30 m FT8).
     Drive level: firmware runs CLK0 at 8 mA, the maximum (`si5351a.c:203` `0x4F`; the 2 mA in
     `si5351.c:267` is overwritten at the first tune). Ron's idea: new opcode + `mscc.ini` key
     to set it, no client change. **Plan only, nothing changed. Ron's name for the plan: "si5351-drive" = `rpi/si5351-drive.md`.**
     Schematic read 2026-10-06 (`Downloads\Schematic_Proficio-Mark-II-Rev-7_2026-10-06.pdf` +
     `Backend_2026-10-06.net`; title blocks say REV 6, 2024-02-10): CLK0 (U13-10) goes straight
     to the two clock pins of U12 74ACT74 (pins 3, 11), no series resistor, nothing else on the
     net = capacitive load only, drive level only sets edge speed. X1 is a 25 MHz TCXO
     (ECS-TXO-3225 or I538), AC-coupled by C147 into XA, not a bare crystal. One 3.3 V rail
     (U3) feeds Si5351 VDD + VDDO, the TCXO, the PCM3060 digital VDD and the PSoC connector;
     only C24 0.1u at the Si5351 / TCXO, no bead. One 5 V rail (U2) feeds the 74ACT74, PCM3060
     analog VCC, RX op-amp U8, both mixers, U16; 0.1u each, no bead. So two leak paths exist on
     paper (3.3 V and 5 V rails); which one is real is not known.
   - **Carrier image about the LO point, only about 22 dB down.** Open: is RX IQ calibrated on
     this rig / band; level read off a flickering display, so re-measure once the client draws
     every point.
   - **Copy of the carrier 24 kHz above it, about 25 dB down.** Open: generator sideband or a
     4-sample pattern in the I/Q stream. Test not run: strong on-air carrier (WWV), generator
     off, look 24 kHz above it. Also not known: same on the pill?
   - **Two weak fixed lines 7.0 and 12.0 kHz below the LO point** (-113). Not looked into.
   - Pill-only, in the blackpill notes: the moving spur at VFO 14.048-14.061 (7 x LO against
     4 x MCLK).
19. **Spectrum dB labels far too low on a strong signal (Ron, 2026-10-07: WWV 10 MHz, CW, S
   meter about S9 = -73 dBm, spectrum peak "nowhere near -73").** Ron wants the peak much
   higher WITHOUT raising the apparent noise floor.
   **Cause found in the code, not yet proven by a test:** the S meter is true dB
   (`sdrcore.c:253`, `20*log10(peakmag) - 20`); the spectrum is `10*log10` of the FFT
   magnitude, a voltage (`dsputils.c:220`), = half of true dB. The client undoes the packing
   exactly (`MSCC.Core/Services/UdpRadioService.cs` `RawYToDb`, `Y/150 - 40`) and adds one
   offset (dB CAL, centre -91.3, trim +/-20). So a real 20 dB step shows as 10 dB and one
   offset is right at one level only. Same line in the Ubuntu and Windows recv.
   **Proposed fix, client only, nothing changed:** `RawYToDb` scale 150 -> 75, bias 40 -> 80
   (same wire data: `(10log+40)*150` = `(20log+80)*75`), raise its upper clamp (80 -> about
   140), then find the dB CAL centre again against a known level (estimate near -63, outside
   today's -111.3..-71.3 range); saved ini offsets, grid max and waterfall contrast move too.
   No server change on any platform.
   Note for Stew: `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-DB-SCALE-2026-10-07.md`, committed
   and pushed 2026-10-08 (after Ron's test below).
   **Ron's test 2026-10-08** (Multus SDR SMSG signal generator, settings S9 and S1 only, 20 m,
   CW, 800 points, window wide): generator S9 -> S meter S9, spectrum peak about -87 (between
   -85 and -89); generator S1 -> S meter S3, peak about -100, noise floor almost -120;
   generator off -> S meter S2. **Squeeze proven** (signal down 36 dB by the meter / 48 by the
   generator, trace moved 13 dB). **Exact 2:1 NOT proven:** 13 on screen = 26 dB by the code,
   less than either ruler. The S meter's own floor is S2, so its S3 is not trustworthy; the
   generator may leak at S1. Whole path rechecked (server max over bins, Y packing, frame
   averaging in `panadapter.c`, client `RawYToDb`, offset, trace + label mapping in
   `SpectrumDisplayControl.xaml.cs`, `Db_to_Smeter` 6 dB per unit): all linear. So either the
   rulers are off or a second cause exists, not found. Both added to the note for Stew with a
   request for a clean 20 dB step test (step attenuator) before and after his change.
   After the fix the S9 setting can set dB CAL (peak = -73).
   Same day, all four SMSG bands (80, 40, 30, 20 m): S meter S9 and the peak about the same
   low reading (around -87) on each = not band dependent, one correction covers all bands.
   (In the note for Stew too.)
20. **trans starts with no operator mic (2026-10-08, port of Stew's cmd-067 Part B1, Ubuntu trans
   3.142, commit 22f6f5b).** Before: `SDRcore-trans-linux/sources/main.c` logged "NO MICROPHONE
   DEVICE FOUND" and exited. Now: logs "No operator mic set: TX voice off" and opens the I/Q
   output only (`manage_stream(1, -1, 2)`, log "OUTPUT-ONLY I/Q (no mic)"); the digital-mic
   fallback copies the operator record only when the operator index is valid; `udp_thread.c`
   CMD_SET_AUDIO_DEVICE with no operator mic goes to output-only I/Q instead of "abort switch"
   (DIGITAL with a valid digital mic, OPERATOR, REMOTE). Stew's hunks applied unchanged (the Pi
   code was the same as Ubuntu's before his change). `extern.h` VERSION_MINOR 140 -> 142.
   WSL syntax check clean. **Built on the Pi 2026-10-08, Ron: "works" (normal operation, mic
   set). The no-mic start itself is NOT tested on the Pi; Ron 2026-10-08: assume it works, he
   will not test it and will report if it fails. Same for the 1.0.57 .deb install.**
   **mscc 1.0.57 built 2026-10-08** (WSL, usual recipe; index export + the new binary and
   control laid over it): same 110 files / modes as 1.0.56; only `sdrcore-trans` (Ron's Pi
   build 2026-10-08), the control version and `factory/README.md` (doc text from the repo)
   differ. In `rpi/mscc-deb/` and `installers/rpi/` (1.0.56 removed there). Not installed on
   the Pi yet (the Pi runs 1.0.56 + the hand-built trans). Committed afc9af9 (pushed). recv / ms-sdr unchanged:
   recv with an unmatched speaker uses the default output or the first one found; with no
   output device at all it takes `Audio_Device_Error` (no input-only stream exists).
   Test on the Pi: empty `~/.local/mscc/operator-microphone.ini`, start: trans stays up, RX
   works, TUNE gives RF; restore the mic: SSB TX audio works.
   Note for Stew (for his records, nothing to do): `.mscc-coord/NOTE-FOR-STEW-PI-NO-MIC-2026-10-08.md`.
22. **S meter: average power, not block peak (2026-10-08, Ron OK'd). Built on the Pi
   2026-10-08 (binary 22:51), Ron: SSB, dummy load, floor was "about 2.5" (flickering S2 / S3),
   now a steady S2; "don't see much difference"; he keeps it. Smaller than the model's 6.7 dB;
   my guess, not measured: the dummy-load floor (about 1 count) has steady tones that do not
   drop. Ron later: "a bit less bouncy". S9 generator check not reported.**
   **mscc 1.0.58 built 2026-10-08** (WSL, usual recipe): same 110 files / modes as 1.0.57,
   only `sdrcore-recv` (Ron's Pi build 2026-10-08 22:51, has the `meter_db` symbol) and the
   control version differ. In `rpi/mscc-deb/` and `installers/rpi/` (1.0.57 removed there).
   Pi recv `VERSION_MINOR` 141 -> 143 on 2026-10-09 (Ron; 143 = Ubuntu recv, Windows is 144):
   built on the Pi 2026-10-09 (binary 11:59; byte compare with the old one: only the build id,
   the build time and 141 -> 143 differ).
   **mscc 1.0.60 built 2026-10-09** (WSL, usual recipe): same 110 entries / modes as 1.0.59,
   only `sdrcore-recv` and the control version differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.59 removed there). **Installed on the Pi 2026-10-09 (Ron).** Committed and pushed 2026-10-09 with these notes.
   Was: `sdrcore.c` `fastconv` took the largest sample magnitude of one
   2048-sample block (21 ms), `20*log10(peak) - 20`; the send thread (`udp_thread.c`, every
   100 ms) sent the latest block only, cast toward zero; ms-sdr passes it through (its
   `Smeter_Average` is commented out); client `Db_to_Smeter` (`MainViewModel.cs:418`) S9 = -73,
   6 dB per unit, integer division toward zero (S steps up to 5 dB early: -78 shows S9; Stew's
   side, not touched, not told). A peak reads noise high and jumpy; a carrier reads true.
   Now: `sdrcore.c` sums `mag*mag`, block average power, `10*log10 - 20` (same constant, so a
   steady carrier reads the same), floor `METER_MIN_POWER` (no log10(0)), smoothed in dB:
   up `METER_RISE_ALPHA` 0.5 per block, down `METER_FALL_SEC` 0.3 s. `udp_thread.c`: rounds
   to the nearest dB. Variable name `peakRxSignalDbm` kept. WSL syntax check clean.
   Python model of old vs new (band-limited noise, 96 kHz, 2048 blocks), not a measurement:
   noise reads lower by 6.7 dB at 2700 Hz, 4.1 dB at 400 Hz, 2.8 dB at 200 Hz; jitter 1.0-2.1 dB
   -> 0.2-0.7 dB; carrier 0.00 dB change. Expect: resting needle about 1 S unit lower in SSB,
   SSB voice a few dB lower, steady carriers the same (generator S9 must still read S9).
   Possible link to item 19's test (generator off = S2, S1 setting = S3): the peak-raised
   floor, not proven. Windows / Ubuntu recv have the old code.
23. **TX ALC button state stored in ms-sdr (2026-10-09, Ron: the client is the wrong place; ini key
   named after the opcode). Built on the Pi 2026-10-09, Ron: "works". Committed and pushed
   2026-10-09 with these notes.**
   **mscc 1.0.59 built 2026-10-09** (WSL, usual recipe): same 110 entries / modes as 1.0.58,
   only `ms-sdr` (Ron's Pi build 2026-10-09 11:45) and the control version differ. In
   `rpi/mscc-deb/` and `installers/rpi/` (1.0.58 removed there). **Installed on the Pi 2026-10-09 (Ron).**
   Was: WPF button starts lit
   and sends nothing at connect, ms-sdr only forwarded `CMD_SET_ALC_MULTIPLIER` (0x23), trans
   starts with ALC on, so the button was wrong after a client restart with ALC off (Ron saw it).
   Now ms-sdr: `extern.h` `User_Controls.Alc_Multiplier`; `user_controls.c` key
   `CMD_SET_ALC_MULTIPLIER=0|1` in `user_controls.ini` (default 1, also when an older file has
   no key), saved on change (`User_Controls_Process`), sent to trans in
   `User_Controls_Apply_To_Cores` (start + client connect) and to the client in
   `User_Controls_Send_To_Gui` (plain opcode 0x23, value 0/1); `main-controller.c` case now calls
   `User_Controls_Process`. trans unchanged. WSL syntax check clean.
   **Client half (Stew), not done:** WPF has no handler for an incoming 0x23, so the button stays
   wrong until it sets `AlcOn` from that report (without sending it back). Windows / Ubuntu
   ms-sdr have the old pass-through. Note for Stew:
   `.mscc-coord/NOTE-FOR-STEW-ALC-BUTTON-STATE-2026-10-09.md` (also asks that the client stop
   sending its own ALC state at connect, as Avalonia does).
   ALC review (same day, code only): limiter in AM / LSB / USB after the filter, target =
   drive x 1.1 = a fixed 1.1 before the drive; TUNE is 0.8 x sqrt(2) = 1.13 on that scale, so
   SSB peaks are held to about 94% of TUNE power, which is rated power only after a QRP cal.
   Ron: review only, nothing to fix.
   **ALC review 2 (2026-10-12, Ron asked; code only, nothing measured, nothing changed).**
   - When: `sdrcoretx.c:110` runs `doALC` in AM / LSB / USB only, after the TX filter, when
     `G_Do_ALC` (the ALC button, 0x23). `dsputils.c` `doALC`: peak of |I| (1 ms decay) x gain
     >= `G_ALC_limit` x `G_ALC_multiplier` = drive x 1.1 (`driver.c:255` sets the limit to the
     drive of the band / mode). Attack 1 ms, release 3 s, gain 1.0 down to 0.1 (20 dB at most).
   - Effect on the I/Q: one gain on I and Q (phase, sideband suppression, balance unchanged);
     only turns down; one peak holds the level down for seconds (3 s release); the 1 ms attack
     is amplitude modulation applied after the filter, so hard ALC widens the signal; the
     detector looks at |I| only, not sqrt(I^2 + Q^2), so a peak mostly in Q can pass; overdrive
     beyond 20 dB passes unlimited.
   - Meter: `alc.c` `ALC_Meter_thread` sends 0x4F every 50 ms while transmitting with the ALC
     button on, value 0..100 from the gain g at that instant (`alc_float_to_meter`), no
     averaging in trans, ms-sdr passes it through, the client shows a short average. Mic audio
     (local / remote): linear, (1 - g) / 0.9 x 100. Digital audio (local / remote): dB, 5 counts
     per dB (comment in the code: on purpose for VAC / WSJT, reason not known). With the ALC
     button off nothing is sent: the meter sits at 0 with no limiting.
   - Client zones (`MSCC.Wpf/Controls/AnalogAlcMeterControl.xaml.cs`): green 0-20, yellow
     20-50, red 50-100. In dB of reduction: mic 20 = 1.7, 50 = 5.2, 100 = 20; digital 20 = 4,
     50 = 10, 100 = 20. Told Ron: voice in the green is fine, peaks into the yellow normal,
     red costs average power and widens the signal; for digital the zones are forgiving (green
     while ALC works), the right reading there is 0. I did not recommend making digital use
     the mic scale; no change asked for.
   - **Guidance for users (Ron 2026-10-12): in digital modes the ALC meter must read 0, no ALC
     action.** Wording given to Ron: keep the ALC button on and adjust the audio level (WSJT-X
     Pwr slider or the digital mic gain) until the ALC meter reads 0 while transmitting; any
     reading above 0 means the audio is too high. Set the power with the power control, not
     with audio. Leave the ALC button on: with it off the meter always reads 0 and tells
     nothing. (0 does not mean full power: check the power reading.) Where it gets published
     (manual / client text) not decided.
   - Purpose (Ron asked 2026-10-12): keep loud audio from overdriving the finals (distortion,
     splatter). Unlike a traditional rig this ALC does not measure the finals (no feedback
     from the rig): it only limits the I/Q to 1.1 x the set drive before it goes to the rig,
     so it protects the finals only as well as the drive level is set; the power calibration
     is what ties that level to the real output.
   - **Idea, Ron 2026-10-12 (discussed only, nothing designed or changed): use the SWR meter
     readings to assist ALC.** Answer given: yes as a slow correction, not as the ALC itself.
     The readings come from the WiFi SWR meter over UDP (`ms-sdr-linux/source/
     swr_wifi_meter.c`, GUI updates deliberately kept slow), far too slow for a limiter that
     must act in about 1 ms, so `doALC` stays. Possible outer loop: forward power slowly trims
     the ALC target (or the drive) so peak output settles on the set power = the feedback from
     the finals that ALC lacks; reflected power / high SWR could pull the drive back. Plumbing
     half there: ms-sdr already sends forward power to trans (`CMD_SET_FOWARD_POWER_VALUE`
     0x38, `swr_wifi_meter.c:160`); trans stores it in `G_watts` (`udp_thread.c:645`) and
     nothing reads it. Open: only for users who have the meter; clean fallback when the meter
     is offline (3 s timeout zeroes the readings); does the meter report peak or average power
     on voice (NOT checked); actual report rate not measured.
   - **Idea, Ron 2026-10-12 (discussed only): the SWR meter could also assist the power
     calibration.** Answer given: yes, a better fit than the ALC use. Cal uses a steady TUNE
     carrier, so the slow report rate and peak / average do not matter. Could be automatic:
     trans keys TUNE into a dummy load, steps the cal value until forward power = the target,
     saves, next band (today the user moves the cal slider per band reading a wattmeter).
     Fits: trans already gets forward power (0x38) and owns `power_cal.ini`. Limits: only as
     accurate as the meter on each band; needs a dummy load and someone present (it
     transmits); only for users with the meter, so the manual slider stays.
     **Ron's rule for it (2026-10-12, "something to think about", nothing designed): when a
     calibration is requested, do it automatically only if trans is receiving power data;
     if not, no automatic calibration (manual as today).** Needed for that gate (my notes):
     trans has no way today to tell fresh data from old (`G_watts` just holds the last value,
     no time stamp; ms-sdr sends zeros once when the meter times out), so trans needs a
     "last report" time; and the reading must rise when TUNE is keyed, otherwise stop and
     fall back to manual (meter on another antenna / rig, or stuck).
24. **Spectrum smoothing: running average in recv (2026-10-10, Ron OK'd). Built on the Pi, Ron
   uses `PANADAPTER_AVERAGE=9` (0.225 s) with REFRESH 1; in mscc 1.0.61, committed 2026-10-10.** Ron's complaint (video
   `Downloads/Video_2026-10-10_112351.wmv`, test client 10.9.0): REFRESH 1 is "nervous", higher
   values are choppy. Measured: redraws per second = 15.6 / REFRESH. REFRESH is client only
   (`MainViewModel.cs:2070` draws 1 frame in N, drops the rest; the waterfall gets one row per
   drawn frame); the client pins the Pi at 3 blocks per frame and never sends the smoothing
   opcode, so recv always ran its 2-frame average (ms-sdr sends `PANADAPTER_AVERAGE` from
   `user_controls.ini`, 0, at start; no range check there).
   Change, `SDRcore-recv-linux/sources/panadapter.c` only: the 4-frame history is gone;
   `PANADAPTER_AVERAGE` 0 = this frame + previous / 2 (as before), 1..100 = running average with
   time constant value x 0.025 s (was 0.1 s / 1..30 at first: Ron found 5 = 0.5 s too slow, 2 to 3 "much better", wanted finer steps) (`alpha = 1 - exp(-frame time / tau)`, frame time from
   `G_Panadapter_Blocks`); after a clear the first real frame loads the history. Tuning = edit
   the ini on the Pi, restart, no rebuild. Known trade: the waterfall is smoothed too (same
   frames). Built on the Pi 2026-10-10: 0 looks as before (Ron). Test: REFRESH 1, try 8 to 12. Not told to Stew yet; Windows / Ubuntu recv have the old code.
   Unexplained in the video: waterfall blank + refill at 5.7 s and 8.6 s (Ron not asked again).
25. **LO point: hole instead of hump (2026-10-10, Ron: "the DC removal is far too aggressive").**
   Measured in the same video: hole about 1 kHz wide at the LO point (2 middle pixels at the
   bottom, skirts to 500 Hz), = the DC blocker at 0.98 (-3 dB at 300 Hz). Ron turned down a
   0.99 test and chose "gentle blocker for true DC". **Built on the Pi 2026-10-10, Ron: "Super"; in
   mscc 1.0.61, committed 2026-10-10:** `SDRcore-recv-linux/sources/dsputils.c`
   `DC_BLOCK_A` 0.98 -> 0.9999 (1.5 Hz), and new levelling in `doPanadapter` after the
   shuffle: the 16 FFT bins each side of the LO point (375 Hz, `PAN_DC_HALF_BINS`) are scaled
   down so each bin's slow average (`PAN_DC_ALPHA` 0.01 per FFT, about 2 s) equals that of the
   16 bins just outside (`PAN_DC_REF_BINS`, quieter side); never scaled up. Keeps the noise
   texture (the old pixel notch + fill was blotchy). A steady signal inside the slice is
   flattened. Synthetic model only (hump + DC + 16 Hz comb, not the rig's real numbers): slice
   comes out level with its neighbours, no hole. To watch on the Pi: shoulders outside 375 Hz
   (then widen HALF_BINS), a few seconds of odd level after a band change.
   **mscc 1.0.61 built 2026-10-10** (WSL, usual recipe): same 110 entries / modes as 1.0.60,
   only `sdrcore-recv` (Ron's Pi build 2026-10-10 12:04, has `dc_bin_avg` and
   `pan_average_primed`) and the control version differ. In `rpi/mscc-deb/` and
   `installers/rpi/` (1.0.60 removed there). **Installed on the Pi 2026-10-10, works (Ron).**
   Note for Stew, both changes:
   `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-SMOOTHING-2026-10-10.md`. Committed 8d7eca4, pushed.
   **mscc 1.0.62 built 2026-10-10** (WSL, usual recipe): same 110 entries / modes as 1.0.61, only
   `sdrcore-recv` (Ron's Pi build 2026-10-10 13:21, has `dc_hold` / `dc_count`: the after-TX
   follow-up below) and the control version differ. In `rpi/mscc-deb/` and `installers/rpi/`
   (1.0.61 removed there). Not installed on the Pi yet (the Pi runs the same hand-built binary).
   Committed a8094ee, pushed. SSH from the PC to the Pi fails (host key changed with the backup card); left alone.
   **Follow-up 2026-10-10 (Ron: after TX the dip at the LO point is obvious and fades too
   slowly). Built on the Pi 2026-10-10, Ron: "much better" (TX length makes no difference); in
   mscc 1.0.62, committed 2026-10-10.** Cause from
   the code, not measured: the low-frequency thump on return from transmit drove the per-bin
   averages far up, so the slice was scaled far down until the 2 s average came back (many
   time constants). `dsputils.c` `doPanadapter`: no learning for `PAN_DC_HOLD_FFTS` (47, about
   1 s) after `G_tx_mode` and while `G_Pause_Panadapter` (the scaling learned before stays in
   use); one FFT counts as at most `PAN_DC_MAX_STEP` (4) x the current average; the first
   `PAN_DC_PRIME_FFTS` (47) are a plain average instead of priming from one FFT.
   **Stew on board with items 24 and 25 (Ron, 2026-10-10). The port to Windows / Ubuntu
   recv is still his to do.**
26. **Idea, open (noted 2026-10-10, un-parked by Ron the same day after Stew came on board
   with items 24 / 25; nothing changed yet): turn the client REFRESH control into the
   smoothing control.** Client work, so Stew's call. No new opcode is
   needed for items 24 / 25: smoothing rides on the existing `CMD_GET_SET_PANADAPTER_SMOOTHING`
   (ms-sdr stores it as `PANADAPTER_AVERAGE`, sends it to recv at start and has it in the
   settings report to the client); the LO point levelling has no setting. Client change would
   be three places: remove the frame dropping (`MainViewModel.cs` `SpectrumUpdated`,
   `_spectrumFrameCounter`); `PanadapterControlsWindow` REFRESH sends the smoothing opcode
   (constant missing in the client `Opcodes.cs`); range 0 to about 30 instead of the 1..10 list
   (Ron's 9 = 0.225 s), value kept by ms-sdr, not the client ini. To settle: name (SMOOTHING /
   AVERAGE), waterfall then always runs at full speed, does nothing on Windows / Ubuntu servers
   until `panadapter.c` is ported. Ron considered having it done in the private copy
   `rpi/wpf-spectrum-test/` and decided no: he does not want to drift from Stew's
   client. Checked 2026-10-10: no WPF source commits since 8564843, so the
   private copy differs from Stew's only by the five dB scale edits. If it is ever done there:
   three marked edits + a patch file against Stew's source.
   **2026-10-12: Ron runs WPF 10.11.3 against the Pi, "working well", and asked for this; label
   settled: SMOOTHING.** Note for Stew, committed and pushed 2026-10-12 with these notes:
   `.mscc-coord/NOTE-FOR-STEW-SMOOTHING-CONTROL-2026-10-12.md`. Checked for it: Windows recv
   `panadapter.c` = the Pi's (same 0..100 range), Windows ms-sdr handles 0xDA the same way, the
   10.11.3 client still drops frames (`MainViewModel.cs` about 2100) and has no 0xDA constant.
   **Later 2026-10-12: Ron now runs `PANADAPTER_AVERAGE=4` (0.1 s) with 10.11.3, "a bit
   better" than 9. The pushed note for Stew still says 9 (not changed).**
   **Generator check with WPF 10.11.3 against the Pi, 2026-10-12
   (`Downloads/Video_2026-10-12_123922.wmv`, 13.6 s, 20 m CW, VFO 14.0667, SMSG "S9" setting,
   which Ron says now gives about S8).** S meter readout -77 dBm, steady in every sampled
   frame. Spectrum carrier tip measured in all 409 frames (grid lines 38 pixels per 20 dB, so
   0.5 dB per pixel): about -75 to -76, range -74 to -77. So the spectrum reads 1 to 2 dB
   above the S meter = they agree within the reading error; "spectrum reads lower than the S
   meter" is settled at this level. dB CAL was at 0 (Ron).
   **S4 setting, same day (`Downloads/Video_2026-10-12_124321.wmv`, 11.6 s, same setup):** S
   meter -104 dBm, steady. Carrier tip in all 349 frames, same method: about -102, range -99
   to -104. Step S9 setting -> S4 setting: S meter 27 dB, spectrum about 26 dB = they track
   within 1 dB; the spectrum reads about 2 dB above the S meter at both levels. Entry
   "spectrum centre offset against the dBm readout" is DONE: Stew's -83.6 is right within
   about 2 dB, true dB scale confirmed on the Pi. (The generator's own step is 27 dB, not the
   nominal 30.) Note for Stew (for his records, nothing to do), committed and pushed 2026-10-12 with these notes:
   `.mscc-coord/NOTE-FOR-STEW-SPECTRUM-CHECK-10-11-3-2026-10-12.md`. Old NOT done: the S4 setting (do they track), the true generator level.
27. **Dropped (Ron 2026-10-10: "I don't like the side effects"): fixed noise floor on the
   spectrum.** Was: recv shifts the trace so the floor sits at the same height on every band.
   Side effects: dB scale becomes relative to the floor, band-to-band and preamp / local noise
   differences no longer show. Nothing was built. Do not propose again.
28. **Volume window for the digital virtual devices (2026-10-10, Ron: like the Windows Sound
   shortcut; Grok's `rpi/mscc-volume-gui` "really screwed things up"). Run on the Pi by copying
   the script (Ron 2026-10-10: window opens with both at 100%, VirtualA slider moves the WSJT-X
   receive meter; Save / Forget NOT tested yet). Not committed.** `rpi/mscc-volume/mscc-volume.py`, one file, tkinter + `pactl`, run
   with `python3 mscc-volume.py`, not packaged. Two sliders only (Ron's choice; a Playback /
   Recording tab layout was offered and turned down): VirtualA sink (receive, recv -> WSJT-X)
   and VirtualB sink (transmit, WSJT-X -> trans); percent + dB (`60*log10(pct/100)`), mute,
   100% button. Opening changes nothing (reads the live level once a second); a slider move
   sets the level and keeps nothing; Save writes `~/.local/mscc/volume-levels.conf` (same
   `key|kind|name|pct|mute` lines the installed `mscc-virtual-audio.sh` already applies at
   login, desktop-button Start or by hand, so no package change; a plain `mscc start` in a
   terminal does NOT rebuild the sinks or read the file: `mscc.sh` only runs
   `mscc-volume-restore` if installed, and mscc-init 1.0.18 removed it), Forget deletes it. Monitors and operator devices are
   never set; a monitor not at 100% only gives a warning. Tested on the PC against a fake
   `pactl` only. What was wrong in Grok's (from its code): every slider move was saved and
   re-applied at each start and each GUI open / refresh; operator devices matched by name
   scoring; sink + monitor sliders on the same pipe; percent only.
   **Installer + Grok volume leftovers removed (2026-10-10, Ron: separate installer, "no remnant of
   the old Grok stuff", "don't break what works now"). Nothing installed on the Pi yet, not
   committed.**
   - `rpi/mscc-volume/`: `build-deb.sh` + `packaging/` -> `mscc-volume_1.0.0_all.deb` (own
     package, WSL build; the script strips CR itself). Installs `/usr/bin/mscc-volume` and
     `mscc-volume.desktop` (`Categories=X-MSCC;` = the MSCC menu folder, which the mscc package
     creates). `Conflicts / Replaces: mscc-volume-gui` so Grok's package goes if it is still on
     a Pi. In `installers/rpi/` + an optional line in `INSTALL.md`.
   - Deleted (24 tracked files, restorable from git): `rpi/mscc-volume-gui/` and, in
     `rpi/mscc-init-gui/`, `mscc_volume_gui/` + `mscc-volume-restore` (not shipped since 1.0.15).
     KEPT: `mscc_init_gui/volume.py`: MSCC Init uses it (`config.py` `set_operator_levels_100`),
     it is not the volume GUI. The old `mscc-init-gui_1.0.12..17` .debs in that folder were
     deleted too (Ron; all tracked, 1.0.14 had Grok's GUI inside); `mscc-init_1.0.18` kept.
     Left: the mscc-init postinst lines that delete `~/mscc/mscc-volume-gui`.
   - `mscc.sh` (both copies: `mscc-binaries/` = the packaged one, and `mscc-deb/packaging/.../
     binaries/`): `restore_pulse_volumes` and its call removed. `apply_alsa_levels_100` kept.
   - `mscc-virtual-audio.sh`: the restore block (Grok's restore tool lookup + a fallback that
     set ANY sink / source in the file) replaced by `apply_saved_levels`: only `sink` lines
     for VirtualA / VirtualB, percent + mute; all else ignored. Tested in WSL with a fake
     `pactl`: no file = 100% only; file with CRLF, mute and old operator lines = only the two
     sinks set; last line without newline OK.
   - **mscc 1.0.63 built 2026-10-10** (WSL, usual recipe): same 110 entries / modes as 1.0.62;
     only `mscc.sh`, `mscc-virtual-audio.sh` and the control version differ (binaries the
     same). In `rpi/mscc-deb/` and `installers/rpi/` (1.0.62 removed there).
   - **Installed on the Pi 2026-10-10 (Ron): mscc 1.0.63 + mscc-volume 1.0.0, rebooted. MSCC
     Volume is in the MSCC menu, both at 100%; configuration unchanged, client volume
     controls and WSJT-X work as before. Save / Forget across a reboot NOT tested yet (Ron:
     tomorrow).** `installers/rpi/install-mscc.sh` removed (Ron never used it; `INSTALL.md`
     now has the plain `apt install` line); the copy in `rpi/mscc-deb/` and the docs that
     mention it stay (Ron).
   - To test on the Pi: install both .debs, reboot; MSCC menu shows MSCC Volume; Save 80%,
     reboot (or desktop Start), window shows VirtualA 80%; Forget, reboot, 100%.
7. Next mscc .deb build picks up the "(package mscc-init)" hint text in mscc-deb postinst /
   build-deb.sh / install-mscc.sh (source only, committed 82b819b). No rebuild just for that.

Done: cmd-042 hi-cut (mscc 1.0.48), FREQ CAL fixes (1.0.49), QRP/QRO cal + amplifier.ini
removed (1.0.50, installed + works). mscc-init C CLI dropped from the mscc .deb in 1.0.47.

## Per-radio cal on the Pi - OLD DRAFT, superseded 2026-10-03 (see Open list 11; 0x29 now means CMD_SET_PARK_CAL_SETTINGS)

"Parked" (Stew's word) = saved copy of one radio model's cal files, kept in a folder per model
while another radio is in use. Per model, not per individual radio.
Windows as built (cmd-055/059, `ms-sdr-MKII/source/factory_seed.c`, WPF `CalPark.cs`): ms-sdr copies
the files at startup (same line: keep live; other line: stash live -> old line, parked -> live; new
line: factory -> parked -> live), then tells trans to reload. Save settings = the client copies files
on the Windows PC. No new opcodes. Does nothing for a remote Pi.
Claude's Pi draft (Stew has not seen it):
- Live `~/.local/mscc/`; parked `~/.local/mscc/cal/<line>/` (`iq.ini`, `power_cal.ini`,
  `amplifier_cal.ini`); `cal/LAST_LINE.txt`; factory `/usr/share/mscc/factory/{iq,power,freq}/<line>/`
  in the mscc .deb (repo `factory/` tree). Line names from FW major: 1 proficio-legacy, 2 geminus-mkii,
  3/4 proficio-mkii, 5 geminus-legacy, 6 ultimus-legacy, 7/8 ultimus-mkii.
- **trans does the copying** (new file, e.g. `cal_park.c`), so trans stays the only writer of
  `power_cal.ini`. Flush pending slider saves before any copy.
- `0x29 CMD_SET_RADIO_LINE` ms-sdr -> trans, data = FW major (sent next to `CMD_SET_PCB_VERSION`,
  main-controller.c:1417). trans compares with LAST_LINE, swaps if different, reloads its tables.
- `0x2A CMD_SET_CAL_SAVE` client -> ms-sdr -> trans: live -> `cal/<line>/` (Save settings; works
  remote; WPF change = Stew). 0x29-0x2F were the only free numbers (Pi + Windows headers, opcodes.txt,
  WPF source checked). Names/numbers not approved.
- ms-sdr never writes the cal files; re-reads power_cal/amplifier_cal after the swap for the client.
- Do NOT use trans `CMD_SET_IQ_DEFAULTS` (0x8D) to reload after a swap: on the Pi it deletes
  `iq.ini` and rebuilds defaults.
- Same as Windows: `freq_cal.ini` seed-if-missing only, `recv-iq.ini` not parked, no auto-park on
  exit, no mirror. Swap is detected only at ms-sdr start (restart needed).
Ron (2026-10-02): two different methods make no sense; Windows should do it the trans way, but he
does not want to fight that battle now. Open: trans vs ms-sdr copying; opcodes; park freq_cal /
recv-iq too?; restart-to-detect OK? Alternative = Pi copies the Windows method + only 0x2A.

## mscc-init package (2026-09-28, committed 82b819b; Stew brief cmd-050 = 443d7a4)

Ron missed the CLI after 1.0.47 and wanted it in Python, kept in the init-gui package.
- `rpi/mscc-init-gui/mscc_init_gui/cli.py`: port of C `mscc-init-linux/sources/main.c` (same 4
  steps/prompts), reuses config.py/devices.py (SWR keep fix; radio I/Q devices hidden like the
  GUI; warns if servers run). `/usr/bin/mscc-init`. **Works on the Pi (Ron).**
- GUI blank window (old bug, also in 1.0.15): "servers running" askyesno ran before the root
  was mapped and stayed hidden. Now `after(200, _startup_server_check)`, dialogs `parent=self`.
  **Works on the Pi (Ron, 1.0.17).**
- Package renamed (Ron: old name misleading): `mscc-init` 1.0.18, `mscc-init_1.0.18_all.deb`,
  Provides/Conflicts/Replaces mscc-init-gui (dpkg upgrade from 1.0.17 checked in WSL). Commands
  `mscc-init` / `mscc-init-gui`, folder `rpi/mscc-init-gui/`, `/usr/share/mscc-init-gui` unchanged.
  In installers/rpi (INSTALL.md + install-mscc.sh updated).
- Build: `build-deb.sh` in WSL Debian from an LF copy (strip CR with sed), repo files are CRLF.
- The C source `mscc-init-linux/` and `mscc-binaries/mscc-init` are kept but unused.

## Changes (2026-09-25)

sdrcore-trans:
1. `udp_thread.c` `CMD_SET_IQ_BAND`: unknown band ignored (was `G_iq_stack[200]` write).
2. `udp_thread.c` `CMD_SET_BAND_POWER_POWER`: `Get_Power_Mode_Index()` maps wire mode
   (0 AM,1 LSB,2 USB,3 CW,4 TUNE,5 FM) to table slot (USB,LSB,AM,CW,TUNE,FM).
3. `driver.c`: USB/LSB always use USB_POWER/LSB_POWER (Stew's 3ec263a had digital on
   TUNE_POWER). **Verified on air**: full power, follows SSB slider.
   Ron rule: TUNE power is TUNE only, fully separate. Windows/Ubuntu trans + WPF cmd-045
   item 8 break it -> brief `.mscc-coord/briefs/cmd-046.md` for Stew. Pi not changing.
4. `dsputils.c`: local DIGITAL_AUDIO (0) back on analog gain (6.324 stereo); remote
   modes 2/3 stay 2.5. Digital mic gain slider needs turning down.
5. `remote_mic.c`: smooth +/-300 ppm fill trim toward 100 ms (was 0.8 %/2.4 % pitch
   steps); prime/resync/re-prime. No file I/O in the audio callback; receiver thread
   logs EVENTs (hold_last reprime, resync, primed, overflow, udp_gap). **Remote audio
   verified on air** 2026-09-26: 30 m FT8 QSO, bad/under/overflow 0, no EVENTs,
   occ ~5020 (~105 ms), step 0.500020, peak ~14250 TX.
6. `main.c`: remote ring drained during TUNE (split streams); mic ring drift-safe (see 8).

sdrcore-recv:
7. `udp_thread.c`: `CMD_SET_IQ_BAND` bounds; phones/digital volume ATTN separate per
   audio mode; after CW TX reopen output for the current mode.
8. `main.c` (and trans mic ring): dual-stream ring primes to 64 ms, +/-300 ppm trim,
   resync, silence + re-prime on underrun (was ~21 ms dropout every few minutes).
9. `panadapter.c`: smoothing = true average of n frames in 32 bits, clamped 1..4
   (was n+1/n in uint16, overflow at 4). Pan levels shift; client dB CAL may need redoing.

Status: recv + ring fixes built on the Pi and working in Ron's first tests.

sdrcore-recv (2026-09-26):
13. `main.c` `manage_stream`: digital play stream (VirtualA) opens with >= 40 ms latency
    (`DIGI_PLAY_LATENCY`; was defaultLowOutputLatency). Cause: WSJT-X decode CPU bursts
    made the stream miss PipeWire cycles -> 128-sample silence gaps (clicks), `pw-top` ERR
    climbing only during decode (Ron confirmed). Phones stream unchanged. Log line
    "rate plan ... play_latency=N ms". First Pi test ran the OLD binary (not restarted;
    `mscc.sh start` skips running servers) - ignore it. 2026-09-26 after rebuild + restart:
    VirtualA play=48000 resample=1 dual=1 play_frames=1024 play_latency=40 ms (confirmed).
    Cold start (no client): phones 10 ms first, ms-sdr sends digital mode ~6 s later ->
    VirtualA 40 ms. With WSJT-X: ERR 0 -> 6 over several decodes (was climbing every
    decode). **Ron OK with this (tested OK 2026-09-26).** Renice not needed.
    Committed 658ae76.

ms-sdr (2026-09-26, cmd-044 port from Windows 93302b4):
14. `calibrate.c`: `Report_Calibration` always sends the per-run count (Cal_Reset removed);
    failed freq cal restores the previous mode (was `ModeChanged('A')`). Same hunks as
    Windows ms-sdr 3.174. Built on the Pi 2026-09-26 (see 15); done per Ron.
15. cmd-045a/b FREQ CAL STOP port (2026-09-26): `CMD_SET_CAL_ABORT 0x1F` (`usbavrcmd.h`,
    `main-controller.c` cal case list), `calibrate.c` = Windows 3.176 file (split auto/check
    run state, abort + 35 s drain, start refused while draining, GUI 0x1F 1=done 2=refused),
    `GetTickCount64` -> `Cal_Now_Ms()` (CLOCK_MONOTONIC). Pi CAL_RESET kept (`Create_PPM_ini`,
    not Windows' `Factory_reseed_live_file`). sdrcore-recv unchanged. Syntax-checked in WSL;
    Pi recv step wait ~30-33 s < 35 s drain. **Built on the Pi 2026-09-26 (with 14).
    Ron: treat as done; he will test later.** Not committed yet.

mscc-init-linux (2026-09-26):
10. `main.c` `init_mscc`: rewrites only its own keys, keeps other `mscc.ini` lines, and adds
    `SWR_METER=1`, `SWR_METER_PORT=6999`, `SWR_METER_TO_GUI=1` if missing (calibration had
    wiped them). Syntax-checked in WSL; not built/run on the Pi yet.
11. `mscc-init-gui/mscc_init_gui/config.py` `write_mscc_ini`: same keep + SWR defaults.
    Tested with Windows Python (existing and new file). Shared `_all.deb` with Ubuntu (Ron OK'd).
    File is `~/.local/mscc/mscc.ini` (not `~/mscc.ini`).
    **Verified 2026-09-26**: with `SWR_METER_TO_GUI=1` in `~/.local/mscc/mscc.ini`, SWR readings show in the client. 1.0.45 + init-gui 1.0.15 not yet installed/tested on the Pi.
12. `mscc-init-gui` 1.0.15: volume GUI dropped (Ron: didn't work out); postinst removes
    old `~/mscc/mscc-volume-gui`. Build with `build-deb.sh` in WSL from an LF copy (repo
    files are CRLF); the `.ps1` builder is stale. `mscc.sh` skips volume restore if absent.

## mscc 1.0.46 (2026-09-26)

Built in WSL from an LF copy of mscc-deb + mscc-binaries (Pi ELFs). Contents: ms-sdr (cal
STOP + cmd-044), sdrcore-recv (#13 40 ms digi), sdrcore-trans 09-25, mscc-init OLD
(09-16, no SWR keep fix; Ron: CLI not used, nothing runs it; source kept). postinst hint
now says run `mscc-init-gui`. Copied to installers/rpi (1.0.45 removed). Installed + tested OK (Ron).

## Done (mscc 1.0.48): cmd-042 (from pull f9efa00, 2026-09-25)

Windows `mscc-recv` 3.141 added 0xD1 CMD_SET_BW_HICUT index 5 = 1400 Hz, 6 = 1000 Hz
(DIG-U Hi, WPF cmd-041). Pi to match in `SDRcore-recv-linux/sources/udp_thread.c`
(hi-cut switch ~line 1547) + version bump -> `mscc_1.0.48_arm64.deb` (1.0.47 used 2026-09-27). Brief:
`.mscc-coord/briefs/cmd-042.md`. Order: after ubuntu-stew is done and Stew pushes.
Reviewed: 0-4 unchanged, low-cut max 500 < 1000, ms-sdr passes index through. Done, see Open list.

## To do: RF check of remote TX audio (Stew, spectrum analyzer)

FT8 QSOs don't prove a clean signal (FT8 tolerates dropouts/pitch steps). Check:
- Single tone (FT8 or steady whistle): one clean line; sidebands/spurs = ring stepping or dropouts.
- Two-tone: IMD (3rd/5th) for overdrive; digital mic gain slider still high (see 4).
- TX on/off edges: no key-up/key-down splatter.
- Full 13 s FT8 over: no frequency jumps or wobble.
- Same test local vs remote audio; a difference points at the remote path.
If bad: `grep "remote_mic EVENT" ~/sdrcore-trans.log` for the same time.

## Closed: RX low-edge hash = WSJT-X display, not in the audio (2026-09-26)

Seen 2026-09-26 in local digital audio (WSJT-X Wide Graph): ~300 Hz band of hash just
above the RX low-cut. Moves with low-cut (500 -> hash at 500-800), same width. Present
on dummy load and in LSB. Not seen in remote: the two back-to-back USB audio adapters
(analog) add a noise floor that hides it. Very weak; phones/on-air likely unaffected.
Not from our changes (`sdrcore.c` untouched).

Earlier guess (overlap-save wrap from FFT-bin zeroing in `fastconv`) is **disproved**.
Simulation 2026-09-26 (real `sdrcore.c` + `wsfirgen.c` + `jimfft` + `doAGC`, white
noise, USB 500-3000, 96 kHz, 2048/4096): passband flat +/-0.1 dB from 600 Hz up, no
bump at 500-800, no block-rate ripple. `wsfirBP` = LP(fc2) - LP(fc1) (center deltas
cancel), clean linear phase. AGC (per-sample gain on noise) lifts the stopband below
low-cut from -90 to about -45 dB: smear *below* the edge, not above it.
NR and auto-notch were OFF when seen (Ron). Not yet tested: digital output path (ring /
resample to VirtualA), WSJT-X Wide Graph "Flatten" (can draw artifacts at steep edges).
Harness was in the session scratchpad (not kept); rebuild from these notes if needed.
Recorded `parec -d VirtualA.monitor` (48 kHz, 28 s, dummy load, USB, low-cut 500) while
the hash showed on the Wide Graph (Flatten off): 540-900 Hz flat within +/-0.5 dB and
noise-like over time, same as the rest of the passband. Edge falls ~40 dB from 540 to
420 Hz. So the hash is how WSJT-X draws the steep edge; nothing to fix in sdrcore.
Seen in the same recording: weak steady tones at 1000, 2000, 3000, 1359 Hz (3-8 dB
above noise in a 1.5 Hz bin), likely birdies. Note: RX digi audio is VirtualA
(VirtualB = TX).

## QRP power cal moved to sdrcore-trans (2026-09-28; built + tested on the Pi by Ron: 2 bands calibrated, power response very good. Committed 5e06637; in mscc 1.0.50)

trans owns `~/power_cal.ini`: `power.c` Create_power_cal_file (startup, factory table
47,30,24,29,53,72,28,30,45,40,40,40 = old ms-sdr PCB 2/4/5/6), bounds/NULL-safe
Init_Proficio_calibration (defaults then RECORD=n overrides), Update_power_cal_file
(tmp + rename). `udp_thread.c` 0xA1 unknown band -> index -1; 0xA2 -> Set_QRP_calibration
(all modes, write file, Drive_Manager reload). Get_Power_Mode_Index removed.
ms-sdr `power_calibration.c`: read-only file load on 0xA1 -> GUI 0xB4; 0xA2 kept as copy +
forwarded (no file write, no INITIALIZE, no 100 ms sleep); 0xB4 from copy; defaults (0xAA)
ignored (client never sends it); Create/Update/Delete + defaults tables removed; main.c no
longer creates the file. No protocol change. Client already forces TUNE power 100 on the
Pwr Cal tab, so no TUNE override in trans. Client never sends 0xAB master reset.
Windows ms-sdr/trans still old way (same file format). Stew briefed: cmd-048 (95601bb; Ubuntu + Windows port, Windows factory-seed choice goes to Ron).
Client flow (Ron 2026-09-28, for reference only - no change wanted): CALIBRATE -> 0xA2 0,
slider moves -> 0xA2 each (saved each time), CALIBRATE again -> "Accept?" YES (nothing sent),
NO (nothing sent; value stays saved), CANCEL (0xA2 previous value).
Slider speed-up (same day): 0xA2 now RAM only + G_drive_recalc (Drive_Manager recomputes);
power_cal.ini saved after ~500 idle Drive_Manager loops (G_power_cal_save_countdown), or
right away before any G_power_file_needs_updated reload. No power.ini/amplifier rewrite,
no Init_Power_All per step; 1 log line per step (was ~70 + 7 file ops).

## Amp cal slider like QRP (2026-09-28; built + tested on the Pi by Ron with amplifier.ini removal: QRP + QRO cal smoother. Committed 1b03de5; in mscc 1.0.50)
trans 0x08 (CMD_SET_POTENTIA_CALIBRATION, -99..0): RAM stack + table (1 + v/100) + G_drive_recalc;
amplifier_cal.ini saved after ~500 idle loops (G_amp_cal_save_countdown) or before any reload.
Flush_pending_cal_saves() before INITIALIZE's Init_Power_All. Update_amplifier_calibration:
RECORD/BAND = index (missing file made every line RECORD=0), tmp + rename, 1 log line.
Found (not changed): amplifier.ini "user power" is unused (client always sends 0xFA=100);
ms-sdr Delete_amplifier_ini_file has remove(homedir) bug; deb amplifier_cal.ini has
calibrated values (-37..-50), not 0.
amplifier.ini removed (same day, Ron): ms-sdr amplifier.c no longer creates/reads/writes it
(Check_Amplifier_Version, Create/Update/Delete/Initialize gone, incl. remove(homedir) bug);
0xF9 replies 0xFB = 100 (constant, what the file held after band select; client shows it on
PowerOut) + 0x08 cal from amplifier_cal.ini (read-only, bounds-checked). 0xFA ignored in
ms-sdr and trans (no INITIALIZE / full reload). trans: Init_amplifier_user_values +
amplifier_table.user_power_value removed. Old ~/amplifier.ini left on disk, harmless.
mscc 1.0.50 built 2026-09-28 (WSL, LF copy): ms-sdr + sdrcore-trans from the Pi (QRP/QRO cal
changes, amplifier.ini removed); sdrcore-recv same as 1.0.49. In installers/rpi (1.0.49 removed).
**Installed on the Pi 2026-09-28, works (Ron).** Deb still ships init-files/amplifier.ini (unused; Ron: keep for now).

## 2026-10-12 (late) — Pi desktop switched to X11; audio levels after the reboots (local note)

- Pi desktop is now X11 + RealVNC Server (was Wayland/labwc + wayvnc), for RealVNC Viewer speed.
  cmdline.txt has `video=HDMI-A-1:1920x1080@60D`; `~/.config/autostart/vnc-1080.desktop` sets
  1920x1080 with xrandr. `wlr-randr` no longer applies. Avalonia client and WSJT-X are X11 programs.
- After the reboots WPF digital volume 100 pegged the WSJT-X meter. All Pi levels were 100% (pipes
  and streams): no `volume-levels.conf` existed, so VirtualA is 100% after a reboot. CAUSE NOT PROVEN: I assumed it had been lower before
  (unsaved), but Ron is sure it was 100% and digital volume 100 was right, and the 2026-10-10
  note says the window opened at 100%. Ruled out: pipe and stream levels, AGC (FAST as usual),
  digital volume attenuation 0x96 (WPF never sends it, recv default index 0 = 1.0). Ron set VirtualA to 32% in MSCC Volume and pressed Save
  (file now: VirtualA 32%, VirtualB 100%). VirtualB is right at 100%.
- "No ALC action" was TX ALC left off in user_controls.ini after Ron ran the Avalonia client; WPF
  read it back. Ron switched it on again. Not told to Stew.
- The `peek:` value in the trans log reads 0.000000 in digital mode even with full power; not a
  sign of missing audio.
- Still to check by Ron: after the next reboot VirtualA comes back at 32% (this is the Save test).
- Avalonia client felt slow on the Pi (Ron: not a concern now; unknown whether new with X11).
- Reboot check done (Ron, 2026-10-12 evening): VirtualA 32% / VirtualB 100% came back, WSJT-X meter
  normal with WPF digital volume 100, TX ALC still on. MSCC Volume Save across a reboot = PASSED.
  Forget across a reboot not tested. WPF restored digital volume 100 / mic gain 100 from
  user_controls.ini (correct).
- 2026-10-12 (Ron's decision): sdrcore-trans ALC can no longer be switched off.
  `SDRcore-trans-linux/sources/udp_thread.c` CMD_SET_ALC_MULTIPLIER (0x23) now sets G_Do_ALC = TRUE
  for any value (the off value was for testing, years ago). Trans only; ms-sdr still stores and
  reports the button value, so the client TX ALC button and `user_controls.ini` can say "off" while
  ALC is on. Checked: ms-sdr and WPF never send off except from the button; ALC acts on AM/LSB/USB
  only. EDITED, NOT BUILT, NOT TESTED, not committed. Not told to Stew. Pi only (Windows / Ubuntu
  trans unchanged).
- 2026-10-12: sdrcore-trans "ALC always on" BUILT AND TESTED by Ron (button has no effect, ALC
  always on). Note for Stew written, not committed: `.mscc-coord/NOTE-FOR-STEW-ALC-BUTTON-2026-10-12.md`
  (asks him to remove the ALC button; Avalonia not named).
- 2026-10-12: Digital Volume / Digital Mic Gain came back at 100 after `mscc restart`. Cause (old
  behaviour, not new): ms-sdr `User_Controls_Process` sets `update_flag = FALSE` for the four
  volume / mic gain level commands (phones and digital), so the value is only in memory until some
  other saved control is changed or Stop_all runs; `mscc restart` uses pkill and ms-sdr has no
  signal handler, so no final save. Fix (Ron OK'd): those four commands set
  `G_User_Controls_Save_Pending = USER_CONTROLS_SAVE_DELAY` (3) and Flusher_thread (1 s loop) counts
  it down and calls `User_Controls_Update_Init_File()` once. Files: ms-sdr-linux/source
  user_controls.c, extern.h, main.c. EDITED, NOT BUILT, NOT TESTED, not committed.
  Test: move Digital Mic Gain, touch nothing else, wait 5 s, `mscc restart` -> value comes back.
- 2026-10-12: ms-sdr delayed save of volume / mic gain levels BUILT AND TESTED by Ron ("the slider
  now function correctly"). Not committed. Uncommitted source now: trans udp_thread.c (ALC always
  on), ms-sdr user_controls.c / extern.h / main.c, plus the ALC button note for Stew.
- 2026-10-12: commits c0d26a1 (trans ALC always on + binary), 85b694c (ms-sdr slider save, source),
  cb476ef (note for Stew: remove ALC button), pushed. Then **mscc 1.0.64 built** (WSL, usual
  recipe via a temp git index so the uncommitted 1.0.63 MSCC Volume files are included; 110 files;
  binaries = the Pi's: ms-sdr 20:57 with slider save, sdrcore-trans 18:59 with ALC always on,
  sdrcore-recv 10 Oct). Copied to rpi/mscc-deb and installers/rpi (1.0.63 removed from
  installers/rpi; it stays in rpi/mscc-deb). NOT committed, NOT installed on the Pi from the .deb.
  V: is the Pi's whole home directory (V:\mscc = installed binaries), not only ini-files.
- 2026-10-12 (end): committed and pushed e1adebf (MSCC Volume work, item 28: rpi/mscc-volume,
  mscc-volume 1.0.0, scripts, old Grok volume GUI removed, mscc 1.0.63 .deb) and 0dff8fe
  (mscc 1.0.64: new ms-sdr binary, control, .deb in rpi/mscc-deb and installers/rpi, 1.0.62 gone
  from installers/rpi). Left uncommitted on purpose: rpi/CLAUDE.md, rpi/si5351-drive.md (birdie /
  private notes) and the PSoC Post Build fix + build output. Ron's list: MSCC Volume Forget across
  a reboot; install 1.0.64 from the package (optional). With Stew: + remove the TX ALC button
  (replaces "button state from ms-sdr").
- 2026-10-12 (Ron): dropped from the Parked list, do not list again: "why receive level was high
  at digital volume 100" and "Avalonia client slow on the Pi".

## 2026-10-13 — resume; Stew's cmd-074 / cmd-075 pulled

- Pulled d8e021a (cmd-074, WPF 10.13.0: TX ALC button removed, Windows trans 3.147 ALC always on,
  Windows ms-sdr 3.183 saves volume / mic gain) and 867eab9 (cmd-075, WPF 10.13.1: SMOOTHING
  control 0xDA default 3 sent at every Start, frame dropping removed, waterfall Falling by
  default, BASELINE + waterfall levels per band). Done from the With Stew list: remove ALC button,
  REFRESH becomes SMOOTHING, BASELINE per band.
- Stew's `.mscc-coord/NOTE-FOR-RON-SMOOTHING-WATERFALL-2026-10-13.md` waits for Ron's answer:
  waterfall now 15.6 rows/s (3x faster) and smears at high SMOOTHING. Option (a) client WF SPEED
  divisor with max-per-point rows (speed only); (b) recv sends unsmoothed data for the waterfall
  (all platforms, protocol change, Pi recv = Ron's work).
- Ron's list: answer that note; install WPF 10.13.1; MSCC Volume Forget across a reboot;
  mscc 1.0.64 from the package (optional); receive I/Q on 80/40/30 m (optional).
- 2026-10-13: Ron installed WPF 10.13.1. `smear.wmv` (SMOOTHING 3 then 10 at ~15 s, 20 m CW):
  no smear at 3, smear at 10 (row carry-over ~0.2 s vs ~1.5-2 s). Pi and Windows recv send the
  same rate: panadapter.c identical, 3 blocks = 15.6 frames/s (trace updates 15.6/s in the video).
  My first "Pi sends 9.5 rows/s" was WRONG: 9.4 is screen pixels/s. WPF waterfall keeps the last
  300 rows (MaxWaterfallHistory) and stretches them over the height with linear blending of two
  neighbour rows (always 19 s on screen; Ron's is ~180 px = 0.6 px/row).
  Ron's answer to Stew: option (a) WF SPEED in the client. Note committed and pushed:
  `.mscc-coord/NOTE-FOR-STEW-WATERFALL-ANSWER-2026-10-13.md`. With Stew list: WF SPEED added.
- 2026-10-13: Stew pushed three more (pulled by rebase): 314b286 NOTE-FOR-RON-IQ-PHASE-AUTOCAL,
  5d28cfb cmd-076 brief (RX IQ tab, "jump to image"), bc13b7e "UP 24 kHz discussion, keep as is".
  NOT READ YET with Ron.
- 2026-10-13: waterfall answer pushed as bb095fc (after rebase onto Stew's three note commits).
  **STEW'S OPEN QUESTIONS FOR RON (Ron 2026-10-13: "Nothing at the moment. But keep his list of
  questions to return to").** From `.mscc-coord/NOTE-FOR-RON-IQ-PHASE-AUTOCAL-2026-10-13.md`:
  1. I/Q phase correction (Stew proposes it himself: TX needs it, RX too; manual PHASE slider
     first, automatic later; recv `Q' = Q + k*I` in framesToComplex, trans on the output, per-band
     `IQ_PHASE=`, new opcode): will Ron do the Pi side first and Stew ports? Range and step
     (his example: +/-5 deg in 0.05 deg steps)?
  2. Confirm the Pi has no phase term today. Recv: confirmed amplitude only. Trans: NOT checked.
  3. Automatic power calibration from the SWR / power meter (user enters QRP and QRO targets):
     how did Solidus do it (loop, safety limits, time per band)? Can code be reused?
  4. UP 24 kHz: Stew keeps it as is. Does Ron want option 1 (display-only slide, client) or
     option 2 (recv NCO to -12 kHz by a new command, all platforms), or leave it?
  Also in that note, no answer needed: receive I/Q per-band
  defaults PARKED by Stew; RX IQ tab points (a)-(c) go ahead as cmd-076 (WPF only, not built yet).
  The phase correction is no longer only Ron's private note: Stew raised it on his own.
- 2026-10-13: Ron installed mscc 1.0.64 from the package on the Pi: "mscc running properly".
  Off his list.
- 2026-10-13 (Ron): Stew's question 4, UP 24 kHz: "my answer is leave. No changes." Neither of
  Stew's two options wanted. NOT yet told to Stew. Questions 1-3 still open.
  Ron: hold the UP 24 kHz answer and send it to Stew in ONE note together with his answers to
  questions 1-3 (phase: Pi first / range and step; Pi phase term today; Solidus power cal).

**Resume point 2026-10-13 (end of session).** Pushed: bb095fc (waterfall answer, option (a)).
Ron installed WPF 10.13.1 and mscc 1.0.64 (package proven). Held for Stew, send in ONE note with
the other answers: UP 24 kHz stays as it is. Stew's questions still open with Ron: phase
correction (Pi first? range/step), Pi phase term today (recv confirmed none, trans unchecked),
Solidus power cal. Ron's list also: MSCC Volume Forget across a reboot; receive I/Q on
80/40/30 m (optional). Uncommitted on purpose: this file, si5351-drive.md, PSoC Post Build fix +
build output. Separate topic (not sdrcore): IQBD detector, see memory file iqbd-detector.md.

**2026-10-13 (evening): Ron wants to look at implementing I/Q phase, TX first.** Reason: IQBD
detector + `rpi\tools\tx-iq-balance\iqbd-meter.py` (memory file iqbd-detector.md) show the TX
amplitude null on 10 m is shallow (image stays about 8 dB above the LO leakage line; on 20 m it
goes below it). Stew's question 2 answered for trans: **Pi trans has NO phase term** (checked:
`SDRcore-trans-linux/sources/main.c` 343-348 `outcplx.real * iMult` / `.imag * qMult`, `iq.c`
`IQ_calc`; `iq.ini` lines are `RECORD=,BAND=,IQ_OFFSET=` and `update_iq_ini_file` rewrites only
those three, so a new `IQ_PHASE=` field must be written there too or APPLY drops it). Proposal
given to Ron, NOTHING changed yet: stage 1 trans only (phase term on the output, `IQ_PHASE=` per
band, a live opcode driven by a small script on the Pi, no client / ms-sdr change); stage 2 opcode
through ms-sdr + client slider (Stew). Not told to Stew.
**Same evening: stage 1 MADE (Ron OK'd, asked for a slider GUI). NOT built on the Pi, NOT run,
NOT committed.** Trans, 6 files in `SDRcore-trans-linux/sources/`: `commands.h` (new opcodes
`CMD_SET_IQ_PHASE 0x2A`, `CMD_GET_IQ_PHASE 0x2B`, provisional numbers: free in trans and in
ms-sdr `usbavrcmd.h`, not agreed with Stew; `IQ_PHASE_STEP_DEG 0.05`, `IQ_PHASE_MAX_STEPS 400`
= +/-20 deg), `extern.h` + `main.c` (`iqPhaseSin` / `iqPhaseCos`; output loop
`Q' = (Q*cos + I*sin) * qMult`), `iq.h` + `iq.c` (`iq_phase` in `G_iq_stack`, `IQ_phase_calc`,
`IQ_PHASE=` read by name and written by `update_iq_ini_file`), `udp_thread.c` (`iq_active` =
record whose correction is in use, set at band change and at CMD_SET_IQ_BAND; 0x2A sets the
phase of that record and answers the sender; 0x2B answers only: opcode, record, phase int32,
amplitude int32; 0x57 COMMIT now also accepted outside a TX IQ session when a band is known).
WSL syntax check with the Makefile flags: clean. Version left at 3.142.
Slider GUI `rpi\tools\tx-iq-balance\iq-phase.py` (tkinter, runs on the PC, `--host` default
`proficio`, UDP 9200 straight to trans): follows the band, live on every step, SAVE sends 0x57.
Tested only against a stand-in server on the PC (10 checks pass).
Known gaps: `mscc-init-linux/sources/iq.c` and Windows / Ubuntu trans rewrite `iq.ini` without
`IQ_PHASE=` (they load it fine, drop it on their next save); sign of the phase value not known
(sweep both ways); an unsaved value is lost at a band change (file re-read), same as amplitude.
**Later same evening: stage 1 BUILT on the Pi by Ron and RUN on the rig. Proof of concept works;
Ron: finishing the phase work is probably not worth the effort.** 10 m: slider and SAVE work
(`iq.ini` now `RECORD=0,BAND=0,IQ_OFFSET=-74,IQ_PHASE=-5`), phase null only -0.25 deg, gain
about 4 dB. The shallow 10 m null was the AMPLITUDE value 5 steps off (-69, null is -74 when
stepped singly with a pause): image then 13-14 dB under the LO leakage line. My "10 m residual
is phase error" reading was wrong; Ron: phase was never a real issue with this hardware design.
Only 10 m tried. **Decision (Ron): leave the phase code IN trans; stage 2 (ms-sdr opcode +
client slider) is Stew's choice, Ron does not push it.** Still NOT committed, version still
3.142, opcode numbers still not agreed with Stew, nothing told to Stew (goes in Ron's one note).

**2026-10-13:** the three scripts moved to `rpi/tools/tx-iq-balance/` (Ron: they belong in the tree) with a README; uncommitted.
**2026-10-13 (Ron: commit and push all of the I/Q work):** trans phase code (6 files), `rpi/tools/tx-iq-balance/`, `.mscc-coord/NOTE-FOR-STEW-IQ-PHASE-ANSWERS-2026-10-13.md` and this file committed and pushed together; the note is now with Stew. Still open with Ron: his Solidus power cal answer. si5351-drive.md and PSoC build output still uncommitted on purpose.
**2026-10-13:** Stew dropped WF SPEED (cmd-077, tested as WPF 10.13.3, reverted; needs raw waterfall data + smooth scroll, `NOTE-FOR-RON-WATERFALL-RAW-2026-10-13.md`). Ron: leave the waterfall as it is in 10.13.2, no raw data from recv. Answer `.mscc-coord/NOTE-FOR-STEW-WATERFALL-AS-IS-2026-10-13.md`. With Stew list: WF SPEED removed.
**2026-10-14:** all bands 10-160 m set on Ron's rig with `iq-auto.py --phase`; Pi iq.ini copied to `rpi/tools/tx-iq-balance/iq-ron-proficio-mkii-2026-10-14.ini`. Note `.mscc-coord/NOTE-FOR-STEW-TX-IQ-FACTORY-VALUES-2026-10-14.md` offers the values for `factory/iq/proficio-mkii` as options (factory table already within 3 steps except 10 m: -80 vs -74). factory/ not changed.
**2026-10-14:** Stew's question 4 (Solidus automatic power cal) answered: Solidus was manual (power_calibration.c slider, amplifier_cal.ini for Potentia; meters never drive the power), no code to reuse, an SWR-meter auto power cal would be built from scratch. Note `.mscc-coord/NOTE-FOR-STEW-SOLIDUS-POWER-CAL-2026-10-14.md`. All of Stew's 10-13 questions now answered.

**2026-10-14: BUG CW + TUN locks the rig in TX (worked around in Pi ms-sdr; built and tested by Ron: CW TUN now drops out of TX, log shows "TUNE still on: TX off before mode C").**
Ron: in CW, TUN press then release leaves the rig transmitting until another mode (USB, LSB...) is picked; USB TUN is fine. Log (`V:\ini-files\ms-sdr.log`): on release the client sends CMD_SET_MAIN_MODE CW first, then TUNE off (USB: same order). Cause is in the radio firmware: `radio-psoc-firmware/Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/main.c` main loop runs `TX_Main()` (which acts on the host PTT, `TX_Request` from 0x50) only when `E_host_mode != 'C'` (lines 159-173); entering CW while keyed leaves the PA on and PTT off is never read; leaving CW runs the PA-off / `TX_Request = 0` block (161-169), which is why a mode change frees it.
Workaround (Ron chose ms-sdr for now): `rpi/ms-sdr-linux/source/main-controller.c` CMD_SET_MAIN_MODE: if `G_Tune` and new mode != T, `Tune_button(FALSE)` + CMD_SET_TX_ON 0 to trans and recv + 20 ms, then ModeChanged. WSL syntax check clean. Covers only TUNE -> CW on the Pi.
**FIRMWARE SHOULD STILL BE FIXED** (my preference, Ron agrees it should be done): on entering CW from another mode, do the same PA off / TX_Request = 0 as on leaving CW. That covers every path (PTT, CAT, digital programs) and every host. Needs a reflash of each rig. Windows / Ubuntu ms-sdr likely have the same TUNE -> CW problem (Stew's code): told in `.mscc-coord/NOTE-FOR-STEW-CW-TUNE-STUCK-TX-2026-10-14.md`.

**Ron's list (2026-10-14):** fix the radio firmware (PA off on entering CW; ms-sdr workaround covers TUN only); MSCC Volume forgets across a reboot; receive I/Q on 80/40/30 m (optional); install WPF 10.13.2 (R10-13-2, Stew's RX IQ tab fixes) and try the RX IQ tab. 630 / 2200 m: not on the MKII, never list.
**2026-10-14: firmware CW fix EDITED (Ron OK'd), NOT BUILT, NOT FLASHED, not committed.** `radio-psoc-firmware/Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/main.c`: on entering CW (top-of-loop CW block) now does the same as leaving CW: LED normal, PA off, AMP off, PCM3060 out on, `TX_Request = 0`. cw.c turns the PA on itself at key-down. Firmware version bumped to 3.233 (basic-plus.h). Test after reflash: CW TUN press/release drops TX; plain CW keying still works; ideally also test with the ms-sdr workaround removed (or from a Windows host).
**2026-10-14: firmware 3.233 BUILT AND FLASHED by Ron.** Test 1 PASSED: USB keyed by client PTT/MOX, switch to CW -> rig drops out of TX. Still to test: plain CW keying.
**2026-10-14: firmware 3.233 test 2 PASSED:** CW keying normal. (No-power scare was the CW power slider at 0 on that band, `user_power_value: 0` in the trans log; Ron set 50, power out fine. Not a bug.) Firmware CW fix DONE on Ron's rig; not committed yet.
Committed with note `.mscc-coord/NOTE-FOR-STEW-FIRMWARE-CW-FIX-2026-10-14.md`. **Ron's list (2026-10-14, after firmware fix):** MSCC Volume forgets across a reboot; receive I/Q on 80/40/30 m (optional); install WPF 10.13.2 and try the RX IQ tab.
**2026-10-14 (Ron):** install WPF 10.13.2 yes, but NOT testing the RX IQ tab / I/Q for now (I/Q calibration too time consuming). Receive I/Q items deferred.
**2026-10-14:** Ron installed WPF 10.13.2, running. Off his list. Remaining: MSCC Volume Forget across a reboot.
**2026-10-14: MSCC Volume Forget across reboot FAILED** (VirtualA back at 32%): WirePlumber ~/.local/state/wireplumber/stream-properties keeps VirtualA 0.032766 (=32% cubic) and restores it after the startup script's 100%. Fix (Ron OK'd): on_forget also sets VirtualA/B 100% + unmuted so WirePlumber stores 100%. mscc-volume 1.0.1 built (WSL), in installers/rpi (1.0.0 removed there) and copied to V:\ (Pi home). NOT installed, NOT tested, not committed.
**2026-10-14: mscc-volume 1.0.1 installed and TESTED by Ron:** Forget sets both 100%; after reboot both 100%, 'Nothing saved'. Forget across a reboot PASSED. Ron to set VirtualA 32% + Save again. Not committed yet. Ron's list now empty (I/Q deferred).
**2026-10-14:** Ron saved VirtualA 32% / VirtualB 100% again (volume-levels.conf checked). Committed mscc-volume 1.0.1.
**2026-10-14: client leaving (Ron OK'd).** WPF Stop in connect-only sends nothing, so ms-sdr held the session ~45 s (main.c gui_count >= 45) -> SESSION IN USE on a quick restart. Pi ms-sdr main-controller.c: CMD_CHECK_GUI_STATUS (0xFE) value 0 from the session owner -> Session_Release(). WSL syntax check clean. EDITED, NOT BUILT, NOT TESTED, not committed. Note for Stew (option): .mscc-coord/NOTE-FOR-STEW-CLIENT-LEAVING-2026-10-14.md (WPF sends 0xFE 0 at Stop; Windows/Ubuntu ms-sdr same lines). Can't be tested end to end until WPF sends it.
**2026-10-14:** main-controller.c copied to V:\ms-sdr-linux\source (Ron: from now on I copy changed Pi source to V: myself, never version.h). Ron to build.
**2026-10-14:** Pi ms-sdr with client-leaving BUILT and RUN by Ron: WPF connects normally (Session_Claim in log). Release on 0xFE 0 not testable until WPF sends it. Committed with the note for Stew.
**2026-10-14: mscc 1.0.65 built** (WSL, index-export recipe): = 1.0.64 except ms-sdr (13:06 today: CW TUN workaround + client leaving) and sdrcore-trans (13 Oct 22:15: TX I/Q phase); 110 files, same modes. In rpi/mscc-deb, installers/rpi (1.0.64 removed there) and V:\ (1.0.64 removed). Not installed from the .deb, not committed. mscc-firmware: Stew's package (linux-build/build-mscc-firmware-deb.sh, cmd-063/066), asked Ron.
**2026-10-14:** mscc-firmware left to Stew (Ron). Note: .mscc-coord/NOTE-FOR-STEW-FIRMWARE-PACKAGE-3.233-2026-10-14.md. Not committed yet.
**2026-10-14:** Ron installed mscc 1.0.65 from the package on the Pi: running normally.
**2026-10-14 (Ron):** PSoC flashing on the Pi is always BOOT jumper + Firmware Upload (bootloader-gui / bootloader CLI). MiniProg3 can't be used on Linux. Firmware's USB enter-bootloader request exists but nothing on the Pi sends it; Ron: jumper is the method.
**2026-10-14: MKII Rev 7 schematic + PCB review (Ron asked; files in Ron's Downloads: Schematic_Proficio-Mark-II-Rev-7_2026-10-14.pdf, Backend_2026-10-14.net, Gerber zip).** Schematic: divide-by-4 74ACT74 LO from one Si5351 output (good phase); TX baseband AC-coupled by C35-C38 so DSP cannot null carrier (hardware floor); ACT74 at ~112 MHz on 10 m is near its limit; title blocks say Rev 6. PCB (4 layers, in1 solid GND under the I/Q area): LO routes short and compact (QSD_I/Q differ a few mm = <0.2 deg at 30 MHz); RX/TX mixer-to-transformer pairs short and symmetric; in2 carries the 6 band-data lines U22 -> P1 DIN under T1/T2 (DC, shielded by in1 GND; minor). QSD lines pass next to X1/C147 (TCXO -> Si5351 XA): minor spur risk. Verdict: no layout fault limiting I/Q balance. Nothing sent to Stew.
**2026-10-14:** correction: once amplitude is set, the IMAGE is below the CARRIER (LO leakage), so the carrier hardware floor is the MKII's main TX limit. Note for Stew (options, Ron approved): .mscc-coord/NOTE-FOR-STEW-MKII-CARRIER-AND-LO-2026-10-14.md.
**2026-10-14: RX image meter (proof of concept, Ron OK'd "sniff first").** `rpi/tools/rx-iq-balance/rx-image-meter.py`: runs ON THE PI with sudo, raw AF_PACKET socket on lo, reads recv -> ms-sdr spectrum packets (127.0.0.1:8888, 0xD5, seq + 400 uint16), finds the strongest carrier within 24 kHz of the LO point (display point pixels/3; 72 kHz span = FFT bins 1024..4095), reads the mirror using recv's exact bin-to-point ranges, prints carrier / image / rejection dB (75 units per dB, as the client). Listen only; Ron moves the RX IQ slider. Tested on the PC with synthetic frames through a copy of recv's mapping (800/1600/3200 points, 30 cases pass) and the frame assembler; raw socket NOT tested (WSL has no Python). Copied to V:\rx-image-meter.py. Not run on the Pi, not committed. Next if it works: script steps 0x52 itself (needs recv iq_band set: RX IQ tab open), or a recv "send spectrum to me" opcode.
**2026-10-14: rx-image-meter.py PROVEN on the Pi (Ron, SMSG +6.2 kHz from LO):** ~40 dB rejection read, follows the RX IQ slider (far left ~22 dB, sharp null 51 dB right of 0, far right ~30 dB). Then (Ron OK'd) `rpi/tools/rx-iq-balance/rx-iq-auto.py`: on the Pi with sudo, imports rx-image-meter.py from its folder, sends 0x52 (5 bytes, opcode + int32) to recv 127.0.0.1:9000, coarse every 10 over +/-200, fine step 2 around best, single steps +/-2 with double average, saves with 0x57. Needs RX IQ START (recv iq_band). Tested on the PC against a fake recv (nulls +57 and -133 found exactly). Copied to V:\. Not run on the Pi, not committed. Client slider does not show the script's value.
**2026-10-14 (late): rx-iq-auto.py run on the rig.** First runs used the WRONG side (SMSG 6 kHz below dial = carrier ABOVE the LO point; its image never reaches the audio): saved 20 m 94, 30 m 79, 40 m 70, 80 m 64. I had not re-read the 2026-10-12 note above (the passband image comes from a carrier BELOW the LO point; 20 m nulls differ by side). Correct setup: START, tick UP 24 kHz (VFO 24 kHz above the SMSG, e.g. SMSG 14.0673 -> VFO 14.0913), carrier shows about -11.5 kHz. 20 m redone that way: null flat +48..+56, 51 dB; 51 saved via client APPLY (APPLY only sends 0x57 = save the value in use). File now: 20 m 51 (right), 30 m 79 / 40 m 70 / 80 m 64 (WRONG side, redo), others 58. SMSG: 80/40/30/20 only, fixed S9/low. Scripts not committed.
**2026-10-14 (late): all four SMSG bands redone on the correct side (UP 24 kHz, carrier about -11.5 kHz), saved by the script:** 20 m 51, 30 m 37, 40 m 27, 80 m 21 (recv-iq.ini 23:12). About 51-53 dB each. Uncalibrated (still 58): 10/12/15/17/60/160 m (+630/2200, not on MKII). Value rises with frequency; fill-in rule for the others still open (nearest band / trend / Stew per-model defaults, see 2026-10-12 note). Scripts not committed.
**2026-10-14:** rpi/tools/rx-iq-balance/rx-iq-fill.py written (Ron OK'd): least-squares line through the --cal bands, writes the others (not 630/2200), shows table, asks y, backup, restart mscc after. Tested on a copy: --cal 20,30,40,80 gives 160 14, 60 24, 17 61, 15 70, 12 81, 10 91. Copied to V:\. Not run on the Pi, not committed.
**2026-10-14:** note for Stew `.mscc-coord/NOTE-FOR-STEW-RX-IQ-TOOLS-2026-10-14.md` (scripts, Ron's values, side matters, trend with frequency, slider / APPLY ALL heads-up; options only). 40/80 m rejection left blank (output not seen).

**Resume point 2026-10-14 (end of session).** RX I/Q done on Ron's rig with the new scripts (d2fed18): measured 80/40/30/20 m = 21/27/37/51 (UP 24 kHz side), rest filled from the line (160 14, 60 24, 17 61, 15 70, 12 81, 10 91), mscc restarted, recv loaded them. Note for Stew pushed (4a0b37e, b1eaa6c: option 4 = AUTO button in the client). Ron's list: empty. Uncommitted on purpose: si5351-drive.md, PSoC build output; this resume line.

**2026-10-15: SD card wear from logging (Ron OK'd option 1).** No fsync anywhere in recv / trans / ms-sdr (fflush only). Measured recv `/proc/PID/io` write_bytes +688,128 in ~10 min (~100 MB/day) while its log grew only ~48 KB/h: each periodic line re-dirties a 4 KB page between ext4 5 s commits. Logs are in `~/.local/mscc/`; `~/mscc/logs/*.stdout` hold only ALSA startup noise (static). Fix: routine lines now logged FIRST TIME ONLY, failure lines kept: recv + trans `udp_thread.c` (KEEP_ALIVE from ms-sdr) and `flusher.c` (KA sent to ms-sdr); ms-sdr `main.c` (KA sent to client first only; "core KA status" line removed, "KEEP-ALIVE missing" kept), `main-controller.c` (KA from recv / trans), `swr_wifi_meter.c` (RX line first good packet only; fault edge still logged). WSL syntax check clean (only old %ld warnings). 7 files copied to V: (checked same as HEAD first). Versions: recv 3.144, trans 3.143 (extern.h, copied to V:), repo ms-sdr version.h set to the Pi's 3.170 (auto-bumped on each Pi build; not copied). Committed and pushed with Ron's OK. Also noted: ms-sdr rewrites ~/startup.ini up to once a second while tuning (option, not done: save after tuning stops). Ron: no note for Stew; Windows PCs rarely run from SD cards.
**2026-10-15: logging fix BUILT and RUN by Ron; measured:** recv write_bytes 49,152 at 10:12:48 and still 49,152 at 10:20:57 (was +688 KB per ~10 min). wchar still rises ~1 byte per write call: the internal pipe (fd 7/8) / audio wakeups, not the card. trans and ms-sdr not measured. Committed.
**2026-10-15:** trans and ms-sdr measured 10:23:37 -> 10:25:49 (only ~2 min): write_bytes flat (trans 49,152, ms-sdr 98,304). ms-sdr made no write calls at all in that time.
**2026-10-15 (Ron):** startup.ini rewrite while tuning: leave it alone for now.
**2026-10-15: mscc 1.0.66 built** (WSL, index-export recipe): = 1.0.65 except ms-sdr (Pi build 10:10), sdrcore-recv 3.144 and sdrcore-trans 3.143 (10:28): logging once-only fix. 110 files, same modes. In rpi/mscc-deb, installers/rpi (1.0.65 removed there) and V:\. Ron installed it: running normally. Committed.
**2026-10-15:** GitHub push emails stopped after b1eaa6c (14 Oct 23:26) for Ron and Stew (Stew's test push 81dfdef emailed neither); GitHub-side, not QTH.com. Ron: leave it (an Actions mail workflow was offered). Stew's empty `test sync.txt` removed at Ron's request: 759b244 (no With Stew list, Ron's choice).

**Resume point 2026-10-15 (end of session).** Pushed today: 42413f3 (Pi servers log routine keep-alive / SWR lines once only; recv 3.144, trans 3.143, ms-sdr 3.170), d2fd36d (mscc 1.0.66, installed by Ron, running normally), 759b244 (test file removed). SD card writes while idle now about zero (measured on all three). startup.ini rewrite while tuning: left alone (Ron). Ron's list: empty. Uncommitted on purpose: si5351-drive.md, PSoC build output, this resume entry.
**2026-10-15 (later):** GitHub retired repository push emails on 2026-10-15 (spam/abuse); not a fault. On Ron's request a push-email Actions workflow was added (8eda679, QTH.com SMTP 465), then Ron dropped it ("farce"): removed in f044450, no secrets added. No push emails now; don't offer the workflow again.

**Resume point 2026-10-15 (end of second session).** Nothing new on the Pi side. Ron's list: empty. Uncommitted on purpose: si5351-drive.md, PSoC build output, this file's resume entries.

**2026-10-18: recv / trans text messages to the client through ms-sdr (Ron OK'd). EDITED, WSL syntax check clean, copied to V:, NOT BUILT, NOT TESTED, not committed.**
Same packet ms-sdr sends to the client: `[0x0B CMD_SET_EXTENDED_COMMAND][0x30 CMD_SET_SERVER_ERROR][text\0]`, sent by the cores to ms-sdr on their keep-alive socket; text starts "SDRcore-recv: " / "SDRcore-trans: ".
- recv + trans `udp_thread.c`: `Core_Send_Message(text, now)` (logs it, each different message once only, max 8) and `Core_Message_Flush()`. ms-sdr starts AFTER the cores (start-mscc.sh), so messages wait until the first keep-alive from ms-sdr (flush there); now=1 sends at once (fatal exit: only reaches ms-sdr if it is already running, else ms-sdr's own "NO RESPONSE" shows). Opcodes added to both `commands.h`; declarations in `extern.h`. Versions: recv 3.145, trans 3.144.
- recv `main.c`: digital speaker not found (warning); I/Q device not found + Audio_Device_Error (fatal).
- trans `main.c`: I/Q device not found (fatal); operator mic not found; digital mic not found -> operator mic; operator mic failed to open; error label (fatal).
- ms-sdr `main-controller.c` `Extended_command`: new case CMD_SET_SERVER_ERROR -> log + `Gui_Add_Message(text)` (delivered when the client is attached, as now). ms-sdr version.h not touched (auto-bump on Pi build).
- Windows / Ubuntu (Stew): not told; their ms-sdr would need the same case only if their cores send messages.
**Resume point 2026-10-18:** core messages waiting for build + test: Ron's replacement rig arrives 2026-10-19. Test: rename digital-speaker.ini, restart mscc, connect client -> "SDRcore-recv: DIGITAL SPEAKER NOT FOUND..."; put the file back. Then commit (with Ron's OK).
**2026-10-19: core messages built (recv/trans 10-18, ms-sdr 10-19) and tested by Ron:** ms-sdr got "SDRcore-recv: DIGITAL SPEAKER NOT FOUND" and queued it, but the client showed NOTHING. Cause: WPF 10.13.2 `UdpRadioService.cs` 0x0B handler only knows the SWR subs 0x0B/0x0C/0x0D; sub 0x30 is dropped (so ms-sdr's own Gui_Add_Message warnings never showed either). My earlier "client already shows it" was wrong. Ron chose option 1: Stew's call to add a 0x30 handler (note `.mscc-coord/NOTE-FOR-STEW-SERVER-MESSAGES-2026-10-19.md`); Pi side: ms-sdr `Gui_send_message` thread now sends via `Gui_send_param_extended` (session peer + dll_s, text only, logs "Sent to client") instead of gui_s/si_gui with a PATH_MAX+2 padded packet. Edited, copied to V:, built (ms-sdr 3.174) and tested 21:41: log shows "Sent to client: SDRcore-recv: DIGITAL SPEAKER NOT FOUND" 5 ms after Session_Claim. Client shows nothing until Stew adds the handler. Committed with the note (Ron: commit and push).
**2026-10-19: mscc 1.0.67 built** (WSL, index-export recipe): = 1.0.66 except ms-sdr 3.174, sdrcore-recv 3.145, sdrcore-trans 3.144 (core warnings to the client; ms-sdr message sender on the dll_s path), copied from V:\mscc. 110 files, same modes. In rpi/mscc-deb, installers/rpi (1.0.66 removed there) and V:\. Ron installed it: running. Committed 460e7a0.
**2026-10-19: push emails back on (Ron: both him and Stew).** Workflow `.github/workflows/push-email.yml` restored from 8eda679 (unchanged). Repo secrets set: MAIL_SERVER mail.multus-sdr.com, MAIL_USERNAME w4mmp@multus-sdr.com, MAIL_TO w4mmp@multus-sdr.com,n8vet@n8vet.com (set with gh), MAIL_PASSWORD (Ron, web page, from Thunderbird saved passwords). Port 465 reachable.
**2026-10-20: Legacy Proficio 1.232 (radio firmware, in `radio-psoc-firmware/Proficio-Legacy - Copy`, Ron's copy; Stew's Proficio-Legacy untouched).** Legacy keys CW by gating DIN (PA line = T/R relay). Changes: DIN reopened after CW (SSB had no TX I/Q after any CW until power cycle); non-QSK key delay on DIN (Key_Delay_Timer/isr as MKII 3.234); key-click shaping with PCM3060 DAC soft mute (reg 0x44, 6 dB/ms at 96 kHz, unmute 14 ms before delayed key down, DIN closes 18 ms after mute); tx.c = MKII 3.234. Post-build path fixed in the copy. Built clean, NOT tested (no Legacy rig): Stew tests, note `STEW-LEGACY-NON-QSK-AMP.md` in the copy.
**2026-10-22: power cal stored in the radio, step 1 (Ron OK'd, option a = save after the slider stops). EDITED, WSL syntax check clean, copied to V:, NOT BUILT, NOT TESTED, not committed.** ms-sdr only: `power_calibration.c` 0xA2 (after forwarding to trans) also writes radio CMD_CALIBRATION 0xC8 type 3 (uint16) for that band; `Radio_cal_save_tick` (from `main.c` Flusher_thread, 1/s) sends type 4 save 5 s after the last step, waits while G_TX. Band map: ms-sdr power record 0 = 160 m ... 9 = 10 m, radio 0 = 10 m ... 9 = 160 m, so radio band = 9 - record (10/11 same). Write failure (old firmware) only logged, no Stop_all. trans / power_cal.ini unchanged; nothing reads the radio's value yet. Test: Pwr Cal on one band, move slider, log shows "Radio_cal_store_power" then "save started"; check with ~/cal-test.py.
**2026-10-22: step 1 BUILT and TESTED by Ron (160 m, 151 slider steps written as radio band 9, save started 1.3 s after TUNE off, status 0). Checked over ssh with cal-test.py: radio 160 m = 60 (= power_cal.ini), save OK, power mask 0x0200. Not committed (Ron: not yet).**
**2026-10-22: step 2 read back (Ron OK'd: radio wins). EDITED, syntax clean, copied to V:, NOT BUILT, NOT TESTED.** `Radio_cal_load_power` (power_calibration.c), called in main.c after Factory_seed_reload_servers (network ready): reads type 5 masks, for each power-valid band reads type 3; if different from power_cal.ini sends trans 0xA1 (meters) + 0xA2 (trans writes the file); updates G_power_stack. Logs one line per band + summary. Values > 255 skipped (8-bit slider). Not run if the network was not ready at startup (same as the factory reload).
**2026-10-22: step 2 BUILT and TESTED by Ron.** Radio 160 m set to 55 (cal-test.py) -> restart: log "160 m radio 55, file 60, radio value sent to trans", trans wrote power_cal.ini = 55. Radio set back to 60 -> restart: file back to 60. Ron's 160 m = 60 in radio and file. Not committed yet. Other 11 bands: only in power_cal.ini, not in the radio.
**2026-10-22: new radio rule (Ron).** Ron's rig is the replacement Stew sent (arrived 10-19); power_cal.ini holds the OLD rig's values except 160 m (the only band calibrated on the new rig). Rule: never copy file values into the radio; bands the radio lacks get the FACTORY value for the radio type (factory/power/<line>/power_cal.ini); only the slider writes the radio. Edited: `factory_seed.c/.h` `Factory_power_cal_values` (reads factory file by record), `Radio_cal_load_power` uses radio value if valid, else factory, else (no factory tree) file kept; differences sent to trans. Syntax clean, copied to V:, NOT BUILT. Expected on Ron's rig: 160 m radio 60 same; other 11 bands -> factory (e.g. 80 m 34, 10 m 46, 630/2200 0).
**2026-10-22: new radio rule BUILT and TESTED by Ron:** 160 m radio 60 same; 11 bands set to factory and sent to trans; power_cal.ini = factory file except 160 m 60. Not committed.
**2026-10-22: RADIO USB HANG during power cal (PSoC MKII 3.236, Ron's new rig).** Radio stops answering EP0 (libusb -7 / kernel usb_set_interface -110, stays enumerated) at a Pwr Cal band select, when trans re-opens the output-only I/Q stream (set_interface); needs power cycle. Hangs: 30 m 2.9 s after a save; 17 m 1.3 s after a save; save-loop test (cal-test.py save every 3.2 s) -> 17 m 1.1 s after a save. No hang: 6 band selects + ~50 TUNE toggles with no saves (160/40/30/20/15/30), band selects 3.3 s / 6 s after saves, 30 m twice. Radio values survive every hang (mask 0x03F0: 160 60, 80 40, 60 37, 40 38, 30 40, 20 44). Firmware read: Cal_Main / EEPROM_StartWrite+Query look correct; found DieTemp_1_Start unlocks the SPC while GetTemp runs (generated code, every ~35 s, unlikely). NEXT TEST: power cycle, start mscc, Pwr Cal select 17 m with no slider move (no save) -> hang = 17 m / hardware, not the save. Steps 1-3 (write, read back, factory rule) not committed.
**2026-10-22:** 17 m select with NO save: no hang (stream opened). Save is involved. Fix tried (Ron OK'd): PSoC MKII-PTT 3.237, calibration.c Cal_Main no EEPROM_UpdateTemperature() (CySetTemp; settings01.c PPM save has it commented out too). basic-plus.h minor 237. NOT BUILT. Test: save loop over ssh + ~10 band changes incl. 17 m.
**2026-10-22:** 3.237 did NOT fix it: radio hung at client connect 12:44:40 with no save for 3 min (radio answered at 12:42:11). All hangs at an audio set_interface (Pwr Cal band select or client connect). Ron: dummy out ms-sdr writes to find where the failure is: power_calibration.c RADIO_CAL_WRITES 0 (store_power logs 'Writes disabled', no USB, no save; startup read-back still runs). Copied to V:.
**2026-10-22:** Full power cal (10 bands, ~700 steps) with RADIO_CAL_WRITES 0: NO hang. Writes/saves implicated. With mscc stopped, file values written to radio with cal-test.py + 1 save: mask 0x03FF, 160 59, 80 39, 60 38, 40 36, 30 39, 20 43, 17 32, 15 34, 12 38, 10 45 (= power_cal.ini). Ron to power-cycle before starting mscc.
**2026-10-22: 3.238 the PPM way (Ron: 'something I know works'). EDITED, NOT BUILT.** Firmware: no save command (type 4 write stalls, read = status); a set marks the band dirty in the ISR; Cal_Main writes that band's ONE EEPROM row (rows 8-19, one per band: 'C', ver 2, band, valid bits, rx 4, tx 4, power 2, 0, sum) with one EEPROM_StartWrite, Query until done, CAL_QUIET_SET 10 passes after a set, waits while TX. Old 3.235-3.237 9-row record NOT read (all bands invalid after flashing). ms-sdr: RADIO_CAL_WRITES 1; slider steps only note the value; 5 s after the last step and not TX each changed band sent ONCE (Radio_cal_send_power); no save. V:\cal-test.py save() now just waits for idle. Copied to V:. Before starting mscc on 3.238: write file values into the radio with cal-test (else startup applies factory to every band).
**2026-10-22: 3.238 + ms-sdr (writes on) BUILT and TESTED by Ron: full power cal, all 10 bands, NO hang, no USB errors.** One value per band sent 5 s after the slider stopped; 12 m value went out 40 ms after the 10 m band select (write during the audio switch): no hang. Before: after power cycle, radio loaded all 10 rows at boot (0 changed). Radio = file: 160 56, 80 39, 60 38, 40 36, 30 39, 20 43, 17 33, 15 33, 12 39, 10 45. Not committed.
**2026-10-22: mscc 1.0.68 built** (WSL, index-export recipe): = 1.0.67 except ms-sdr 3.179 (Pi build 13:18: power cal in the radio, 3.238). 110 files, same modes. In rpi/mscc-deb, installers/rpi (1.0.67 removed there) and V:\. Installed by Ron: running.

**Resume point 2026-10-22 (end of session).** Power calibration stored in the radio: DONE on Ron's MKII. Radio firmware 3.238 (PSoC MKII-PTT): no save command; each set (0xC8 types 1-3) is written to EEPROM by the radio itself, one row per band (the PPM way; 3.236/3.237 save command hung the radio's USB at the next audio interface change). Pi ms-sdr 3.179: slider value sent once 5 s after the slider stops (out of TX); at start radio values win, bands the radio lacks get factory values, file never copied into the radio. Ron's 10 bands calibrated and stored in radio = power_cal.ini (160 56, 80 39, 60 38, 40 36, 30 39, 20 43, 17 33, 15 33, 12 39, 10 45). mscc 1.0.68 installed, running. Pushed: 6680c11 (firmware 3.238 + release binaries + ms-sdr), 9294f7d (note for Stew FIRMWARE-3.238-CALIBRATION), 712cf1d (mscc 1.0.68). Not done yet: RX / TX I/Q into the radio (ms-sdr / trans / recv side); Black Pill still has the save command (Stew's option in the note); Windows / Ubuntu ms-sdr (Stew). Left out of git on purpose: old dated release hexes (deleted on disk, still in git), bootloader/ folder, PSoC build output. Power-cycling the rig needs a Pi shutdown: ask only when needed.
**2026-10-22 (later): TO DO LATER (Ron: record only, no changes yet): remove the calibration clear command (0xC8 type 6, a no-data USB request like the old save, the likely hang cause).** Places: PSoC `usbvend01.c:385` handler, `basic-plus.h:149,160` (comment + CAL_TYPE_CLEAR), `calibration.c:2` history, version 238 -> 239. Black Pill `pio-async/src/usb_vendor.c:216`, `src/calibration.c:163` cal_clear(), `include/calibration.h:11,32,48`, `include/proficio_config.h:25` + version bump, re-sync 8 MHz copy; open question: also remove the pill's save command (type 4 write, the other no-data request)? `V:\cal-test.py` lines 31, 104, 194. Stew note FIRMWARE-3.238-CALIBRATION table line 22 (pushed: follow-up commit). ms-sdr and H743: nothing. Hang analysis (not proven): save = no-data control request (USBFS_InitNoDataControlTransfer) + 9 EEPROM rows; PPM / 3.238 = data-stage write + 1 row. No-data path most likely (a hang came 3 min after the last save). Proof test if wanted: 3.238 cal-test clear then band select (wipes radio values, needs Pi shutdown to recover).
**2026-10-22: clear (0xC8 type 6) REMOVED (Ron OK'd). EDITED, NOT COMMITTED.** PSoC MKII-PTT 3.239: usbvend01.c handler + l_cal_i + USBFS_pvt.h include (only used for the no-data call) removed; basic-plus.h CAL_TYPE_CLEAR removed, comment, minor 239; calibration.c history. NOT BUILT (Ron builds in PSoC Creator). Black Pill (psoc-replacement-stm32 repo, 25 MHz): usb_vendor.c, cal_clear() removed, calibration.h, build 9; pio build OK (build_info.h changed by the build). 8 MHz copy has no calibration code: nothing to sync. Black Pill save (type 4 write) left as is (Stew's call). V:\cal-test.py: clear command gone, test checks type 6 stalls. Stew note table line 22 updated (needs a commit).
**2026-10-22: CW key clicks, PSoC MKII-PTT 3.240 (Ron OK'd option 1). EDITED, NOT BUILT.** Stew's spectrum capture: wide skirts at every key up / key down (blue ~-47 dBc at +/-25 kHz), steady dah clean; scope: RF full in ~1 ms after the 23.5 ms non-QSK key delay. Cause: trans sends a steady carrier in CW (tune_modulate), the radio hard-switches the PA (CONTROL_RX) in the 1 ms key delay ISR; I/Q is DMA so the 8051 cannot shape samples (iqhump tables unused). Fix = Legacy 1.232 soft-mute method on the MKII: PCM3060 DAC soft mute (reg 0x44 = 0x03 / 0x00, 6 dB/ms, ~3 ms 10-90%) muted in non-QSK CW idle; ISR: PA on + unmute request DAC_RISE_LEAD_MS 14 before the delayed key down, mute request at the delayed key up, PA off DAC_RAMP_MS 18 later; main loop (Manage_Paddles_Port) does the I2C write; QSK unchanged (DAC unmuted); main.c leaving CW calls CW_Key_Delay_Stop (unmute). basic-plus.h minor 240 + prototypes. Open risk: codec I2C writes while audio streams never done before on the MKII. Test: CW normal + fast, listen for RX audio dropouts, trans/recv underruns, USB resets; Stew repeats the spectrum capture. Fallback 3.239.

**Resume point 2026-10-22 (end, second session).** Pushed: 443854a (PSoC MKII-PTT 3.239 calibration clear removed + 3.240 CW key click shaping, release binaries 20261022 = 3.240, note for Stew NOTE-FOR-STEW-FIRMWARE-3.240-CW-KEY-CLICKS, 3.238 note table fixed), 8be17b3 (psoc-replacement-stm32: Black Pill pio-async 3.9 clear removed, built, not flashed). Ron's MKII runs 3.240: power cal loaded from the radio (10 bands, 0 changed); non-QSK CW tested, RX audio normal, no errors / USB resets. WAITING ON STEW: spectrum capture of key up / key down on 3.240 (+ 1 ms/div scope edges). If 6 dB/ms soft mute is not enough: option 2 = step the PCM3060 DAC volume (0x41/0x42) from the main loop for a ~5 ms raised cosine. QSK path still unshaped (no key delay). Not done yet: RX / TX I/Q into the radio; Black Pill save command (type 4, Stew's call); Windows / Ubuntu ms-sdr (Stew). V:\cal-test.py updated for 3.239 (no clear). Uncommitted on purpose: rpi/CLAUDE.md notes, PSoC build output, old dated hexes (deleted on disk), bootloader/ folder.
**2026-10-22: 3.240 key click shaping CONFIRMED by Stew.** Spectrum (100 Hz RBW, 250 kHz span, persistence over keyed dahs): the wide key up / key down skirts are gone (before: about -47 dBc at +/-25 kHz; now only a narrow skirt within a few kHz, rest at the noise floor; the fixed spurs are the same as before). Scope 50 ms/div: paddle to RF about 20 ms (key delay), dah 203 ms (= 18 WPM dah, the keyer finishes the element after the paddle is released), soft edges. Option 2 (volume-step raised cosine) not needed.
**2026-10-22 (Ron): no notes to Stew about the Black Pill for now.** Ron is doing a partial Rev 2 build (no keyer parts: PIC U3/J3, LM386 side tone, U6/U7) and a lot of testing first.

**Resume point 2026-10-22 (end, third session).** All code committed and pushed (443854a firmware 3.239 / 3.240 + notes for Stew; 8be17b3 Black Pill 3.9). Ron's MKII runs 3.240; key click shaping confirmed by Stew (spectrum + scope). Rev 2 boards arrived: Ron doing a partial build to test the Black Pill (fit: R1, C9, C11, U4, C10, C1/C2/C6/C7/C8, pill headers, R11/R12, R14, R9/R10, BOOT; optional J4+U9 PTT, R13/R15/C3/THERM; leave off keyer: U3, J3, U5, R2-R8, RP1, L1, C12-C23, U6, U7, D1, D2, J5, J6, AUDIO_OUT; paddles not testable without the PIC). No notes to Stew about the Black Pill until Ron's testing is done. Open: (1) Rev 2 build + test; (2) RX / TX I/Q into the radio; (3) Black Pill save command (Stew); (4) Windows / Ubuntu ms-sdr (Stew); (5) QSK path unshaped (optional). Uncommitted on purpose: rpi/CLAUDE.md notes, PSoC build output, old dated hexes, bootloader/ folder.
**2026-10-24: ms-sdr PTT / key polling (Ron OK'd).** Black Pill "crash" = pill USB interrupt storm (fixed in pill 3.12, see the blackpill CLAUDE.md); before each one ms-sdr's polls returned junk (0xA500 / 0xA400 = the request bytes) and a 500 ms SET_PTT timeout called Stop_all. `main.c`: Get_Key_Status thread no longer started (it only logged CMD_GET_KEY_DOWN; result unused); Manage_MKII_PTT_Switch polls CMD_GET_PTT every 20 ms (was 1 ms: delayMicroseconds = Sleep in ms, not us), reads 1 byte, accepts only r == 1 (r < 0 used to pass `if (r)`), logs read failures once. Built on the Pi (ms-sdr compiled 14:39), running; PTT jack not testable until the Rev 2 PTT parts are fitted. Still: a real SET_PTT failure calls Stop_all; other `usbControlMsgIN` callers not checked.

**2026-10-25: ms-sdr firmware type (Ron OK'd).** `extern.h`: `G_firmware_type` (FIRMWARE_TYPE_UNKNOWN 0 / PSOC 1 / STM32 2), `FIRMWARE_PSOC_MINOR_MIN 200`. `main.c`: set right after srGetVersion (MINOR >= 200 = PSoC, else STM32 Black Pill, which restarted at 3.1 the same day and uses 1-199) and logged: `main . Firmware type: STM32 (minor 1)`. Nothing uses the variable yet (meant for pill-only requests such as CMD_AUDIO_STATS 0xC9, which the PSoC does not answer). Copied to V: (checked: only this edit differed), built on the Pi (ms-sdr core 3.181, 22:46), running, log line confirmed. **mscc 1.0.72 built** (WSL, same recipe as 1.0.71): = 1.0.71 except ms-sdr (from V:\mscc). 119 entries, same files / modes. In rpi/mscc-deb, installers/rpi (1.0.71 removed there) and V:\. Installed on the Pi over ssh: the postinst "Continue? [y/N]" prompt waits on /dev/tty and hangs without a terminal; finished with `sudo DEBIAN_FRONTEND=noninteractive dpkg --configure mscc`. Running.

**2026-10-29: AM receive audio poor, cause found (not edited yet; Ron moves the rig back to the Pi to do the work).** `sdrcore.c` initDSP builds the same real bandpass FIR for AM as for SSB: `wsfirBP(lastHRFiltLow .. lastHRFiltHigh)`, which passes +/-(low..high) and blocks 0 Hz, where the AM carrier is. The detector (`sdrcore.c` ~169, "Simple AM demod") is `mag = |I,Q|`, so with no carrier it full-wave rectifies the sidebands: harsh distortion. Proof on Windows recv (same code, Stew's): log `New Mode A lastHR=[500,4000] filt=[500,8000]` = carrier and +/-500 Hz removed. Also: no DC removal after the detector (carrier = big DC into AGC / NR / notch, thumps on fades). Proposed fix (Ron OK'd the plan, edit on the Pi tree tomorrow, ask before editing): (1) AM filter = lowpass 0..high cut (`wsfirLP`), (2) DC blocker after `mag` like the FM path. Later option: synchronous AM detector (selective fading). Then a note for Stew for the Windows recv (same two lines).
**2026-10-29: AM fix BUILT and TESTED (sdrcore-recv 3.146, Ron OK'd).** `sdrcore.c`: initDSP AM filter = `wsfirLP(0..lastHRFiltHigh)` (low cut ignored in AM; log still prints the client's `FIR=[low,high]`); fastconv AM = `mag` then DC block `AM_DC_BLOCK_A 0.999` (~15 Hz), reset when leaving AM (`am_demod_inited`). Built on the Pi over ssh (`make` in ~/SDRcore-recv-linux, installs ~/mscc/sdrcore-recv). Ron: 9.930 MHz AM "sounds much better". More listening tonight (oldies station).
**2026-10-29: noise blanker lost audio (Ron, not repeatable). Fix BUILT, running (sdrcore-recv 3.147, Ron OK'd), not proven.** Likely cause (not proven): `initBlanker` rebuilt nbstate in the UDP thread while the audio thread ran `doBlanker`; it set oindex = 0 before the new delaylen, so a shorter width (30 -> 10) could leave oindex past delaylen, `==` wrap never hit, reads (and iindex writes) ran off the 10000 delay line; a NaN from there locked the AGC (NaN gain passes the limits) = silent until mscc restart. Also found: resting gain gainramp[0] was 1 - ga (0.7 at width 10, -3 dB with NB on). Fix: `blanker.c` initBlanker only stores a request (+ log), `blankerPoll()` (called in `dsputils.c` complex_shift before doBlanker) applies it in the audio thread; wraps use `>=`; width clamped 2..9998; tripcounter reset if past the width; gainramp[0] = 1.0. `dsputils.c` doAGC: gain / maxval reset if not finite, NaN output samples -> 0. `blanker.h` prototype. Ron's first run: NB on/off, width 297 -> 240, threshold moves, no errors. Startup default is still NB enabled (presets) until the client sends its setting: unchanged.
**Resume point 2026-10-29:** rig on the Pi (pill). sdrcore-recv 3.147 running (AM + NB fixes), files copied to V:, NOT COMMITTED. Next: Ron tests AM tonight (oldies station) and the NB; then commit the code, note for Stew (Windows recv: same AM + NB fixes; the Windows log showed AM `lastHR=[500,4000]`), and an mscc .deb if Ron wants.
**Lesson 2026-10-29:** the recv Makefile's `make clean` deletes `$(BINDIR)/sdrcore-recv` (default ~/mscc): a test build in a copied tree needs `BINDIR=...` on `make clean` too. It deleted the installed binary once; rebuilt from ~/SDRcore-recv-linux at once. mscc was found stopped afterwards (ms-sdr too; recv log simply ends at 16:26), so not caused by that.
**2026-10-29 (evening): Stew's push 21a2721 pulled (rebase).** Windows trans 3.148 (5 ms TUNE start ramp), Windows recv 3.148 (Digital output after CW TX off), PSoC MKII PTT 3.242 (DAC soft mute before PA off, on Shack, source not pushed); his note `.mscc-coord/NOTE-FOR-RON-TX-EDGES-FW-3.242-SERVERS-3.148-SSB-SPLATTER-2026-10-29.md` has 5 questions for Ron. Pi recv already had the Digital fix (`Reopen_Current_Output`). Pi trans TUNE ramp was ported (3.145) then **backed out (Ron: not needed)**: the pill does both edges since firmware 3.6 (raised cosine on the TX I/Q in all modes, blackpill cd4d32c); backup branch `backup-trans-3.145-ramp`.
**TO TEST (Ron, later): Pi sdrcore-trans 3.145 = new ALC (Stew's question 1, SSB voice splatter). Built and running on the Pi, NOT COMMITTED.** `dsputils.c` `doALC` / `initALC`: level |I+jQ| instead of fabs(I), peak hold falling 40 dB in 50 ms (was 1 ms), gain moves toward target / peak with a 3 ms one-pole attack (was 0.4 dB cut per sample), release 3 s unchanged; target, gain limits 0.1..1, ALC meter (G_ALC_gain, G_alc_count), AM/LSB/USB only all unchanged. Test: (1) SSB voice sounds normal, ALC meter still moves, power about as before; (2) splatter: on an analyzer or another receiver, no broadband streaks early in an over (Stew's check: lower PWR until the ALC never acts = the old code's streaks gone too). If overshoot shows at sudden peaks: add a ~3 ms look-ahead. Fallback: 3.144 = repo HEAD `dsputils.c` / `extern.h`. Then commit, and answer Stew's question 1 (the Windows trans has the same code).
**2026-10-29 (late): Stew's 5 questions answered, note pushed d694832** (`.mscc-coord/NOTE-FOR-STEW-ANSWERS-TX-EDGES-2026-10-29.md`; pushed alone). Answers: (1) ALC change OK, Pi 3.145 running, not on-air tested; (2) no trans ramp for QSK amps, release mute in the firmware QSK path; (3) Legacy first, starting from `Proficio-Legacy - Copy` 1.235 (Build or Code to examine it first), then Ultimus MKII PTT, ATU lines, Geminus MKII; (4) RIT retune only after TX idle, done on the pill (3.7); (5) no analyzer here, port IQ_PHASE and check on Stew's. **Resume point:** Pi runs recv 3.147, trans 3.145 (new ALC, NOT committed, Ron to test, see TO TEST above), pill firmware 3.7. Open: Ron's ALC test, AM test.
**2026-10-30: frequency calibration fixed (Pi ms-sdr 3.182 pushed cb6f375; pill firmware 3.8-3.10, blackpill repo).** The pill never applied the PPM (3.9 now applies + keeps it in flash) and sent temperature / PPM replies byte swapped (3.10). ms-sdr: calibration start point G_int/G_dec now from the radio's PPM read at client connect (was 0 until a calibration ran in that session). WWV 15 MHz: -1.00 ppm, CALIBRATION_TEMPERATURE=36. Also: the 15 MHz spur comb was a bad USB cable (replaced). **Resume point:** Pi runs recv 3.147, trans 3.145 (new ALC, NOT committed, Ron to test), ms-sdr 3.182, pill 3.10. Open: ALC test, AM test.
**Resume point 2026-10-30 (midday):** Pi runs recv 3.147 (rebuilt 12:06 from ~/SDRcore-recv-linux), trans 3.145 (new ALC, NOT committed, Ron to test), ms-sdr 3.182, pill firmware 3.10. mscc-station pushed to 7d8b0da, local main = GitHub. Open: ALC test (see TO TEST), AM test.
**2026-10-30 (afternoon):** trans 3.145 (new ALC) committed + pushed a602465 after Ron checked the ALC meter (normal, log clean); power / voice / splatter checks are Stew's (analyzer). CHU is off the air: note for Stew pushed 6d81459 (`.mscc-coord/NOTE-FOR-STEW-CHU-REPLACEMENT-2026-10-30.md`): replace GEN CHU1 3.330 / CHU2 7.850 with RWM 4.996 / 14.996 MHz (RWM 9.996 already in the list; WWV 5/10/15/20 covers the U.S.). **Resume point:** Pi runs recv 3.147, trans 3.145, ms-sdr 3.182, pill 3.10; local main = GitHub. Open: Ron's AM test; Stew: ALC analyzer check + Windows ports (ALC, ms-sdr calibration start point), CHU -> RWM in the client.
**2026-10-30: Pi home cleaned up.** Test tools in `~/utilities` (on PATH via ~/.bashrc): `audio-stats.py`, `usb-snap.sh`, `check-usb`, `drop-caches`; pill firmware images in `~/utilities/firmware` (current `app-firmware-async.bin`, old `app-firmware-3.N.bin`); old build logs / snapshots in `~/utilities/old-logs`; how-to for each tool in `~/utilities/README.md`. fldigi 4.2.13.12 built from the tarball and installed in /usr/local (2026-10-29); its build folder removed. Resume point unchanged: recv 3.147, trans 3.145, ms-sdr 3.182, pill 3.10; open: Ron's AM test.
**2026-10-30 (evening): heat sink temperature (Pi ms-sdr 3.183, committed cdc928b, NOT pushed).** Ron fitted a 10k NTC (beta 3950 assumed) to the heat sink on the Rev 2 board's THERM input; pill firmware 3.11 reads it on PA4 and answers 0x06 (CMD_GET_POTENTIA_TEMPERATURE). ms-sdr `proficio-temperature.c` `Check_temperature`: on the STM32 radio only (`G_firmware_type`), reads 0x06 every 3 s, sends it to the client as 0x06 (WPF ignores 0x06 for now: Stew), log line every 30 s (`TEMP_LOG_MS`): `Check_temperature. Chip: 35 C, heat sink: 32 C`. Watch: `grep -a "heat sink" ~/.local/mscc/ms-sdr.log | tail`. Tested: lines every 30 s, no errors.
**Stew pushed (not pulled yet, 2026-10-30): 025439b trans/recv 3.149 + PSoC 3.242, note `.mscc-coord/NOTE-FOR-RON-SSB-TX-RESULTS-WINDOWS-3.149-2026-10-30.md`.** Windows only, analyzer-checked: (1) recv no longer `Pa_StopStream` at PTT, outputs silence (the stop made TX audio gaps on the shared USB device = splatter); (2) trans ALC ceiling 0.45 x drive (was 1.1), 2 ms attack, voice ALC meter = peak envelope / ceiling; (3) trans 8th-order 6 kHz low-pass on the mic before the AM/SSB modulator (`tx_audio_lowpass`, main.c): mic hiss at 21-24 kHz made the LSB image. PSoC 3.242: DAC soft mute both edges, mode-change drain; it also cleared Ron's PSoC post-build step (copy-release.bat) and changed TopDesign. Stew is still working (next: recv 3.150, ms-sdr 3.184 catch-up, PSoC 3.243, Legacy).
**Resume point 2026-10-30 (evening):** Pi runs recv 3.147, trans 3.145, ms-sdr 3.183, pill 3.11. **Ron: wait until Stew is finished**, then: pull (the local PSoC build files under radio-psoc-firmware differ from Stew's: stash them first, Ron OK'd the idea), port Stew's three server fixes to the Pi (recv no stop at PTT, trans ALC ceiling / meter, trans 6 kHz TX low-pass), build, test, and push everything together with cdc928b. `rpi/si5351-drive.md` is changed locally, not Stew's.
**2026-10-30 (late): Pi ms-sdr 3.184 (committed 0ba7c51, NOT pushed) + pill firmware 3.12:** pill sends -99 on 0x06 for an open thermistor, 999 for a short (was: frozen at the last value); ms-sdr logs `heat sink: OPEN (thermistor open or unplugged)` / `SHORTED (thermistor shorted)`. Normal line tested; open / short not yet (Ron: unplug the thermistor later). **Resume point:** Pi runs recv 3.147, trans 3.145, ms-sdr 3.184, pill 3.12. Still waiting for Stew to finish; then pull (stash the local PSoC build files first), port his three server fixes, push everything (cdc928b, 030aa04, 0ba7c51 and this note).
**2026-10-30 (night): Stew's 3.149 pushes pulled (rebase) and his three TX fixes ported to the Pi.** Pi trans 3.146: ALC ceiling 0.45 x drive (was 1.1), attack 2 ms, maxval starts at 0, voice ALC meter = peak level / ceiling (100 = ALC starting to act; digital keeps the gain meter), 8th-order 6 kHz low-pass on the mic before AM/LSB/USB (`tx_audio_lowpass`, LSB image). Pi recv 3.149: no `Pa_StopStream` / `Pa_StartStream` at PTT; `process_iq_to_stereo` outputs silence while transmitting (MONITOR off, not CW), remote phones get silence. Tested: 10 s PTT, pill stats since mark: in_drop 0 (was ~1000 per s of TX), in_gaps 0, out_under 0 (was +1 at each TX start / stop): the Pi had a small form of Stew's TX-gap problem. Log "RX muted, stream kept running" / "RX unmuted", no errors, G_ALC_multiplier 0.45. **SSB voice and the new ALC meter: Ron marked as working, to be tested on the air later.** Ron's PSoC build files from this PC are in `git stash` (stash@{0} untracked, stash@{1} changed), set aside for the pull. Stew now keeps working notes in `mscc-ui/windows-work-tree/CLAUDE.md` (read it after each pull); open items there for Ron: TX I/Q slot layout in the radio's calibration store (int16 offset + int16 phase, LE, same for the pill), why the 0xC8 clear was removed in PSoC 3.239, Pi power cal start order (his note: Pi puts factory values in the radio).
**Resume point 2026-10-30 (night):** Pi runs recv 3.149, trans 3.146, ms-sdr 3.184, pill 3.12. All pushed. Open: Ron's SSB voice / ALC meter test, thermistor open / short test, AM test; Stew's open items above.
**2026-10-31 (0420 UTC): AM receive test PASSED (Ron).** WRMI 5850 kHz talk in AM mode on recv 3.149 (includes the 3.146 / 3.147 AM fix): clean, "almost as good as SSB". Open now: Ron's SSB voice / ALC meter test, thermistor open / short test.
**2026-10-31 (0440 UTC): WPF client LSB filter bug, found and diagnosed by Ron. Note for Stew pushed 19d4417 (`.mscc-coord/NOTE-FOR-STEW-LSB-FILTER-BUTTONS-2026-10-31.md`, plus a "FROM RON, READ FIRST" line at the top of `mscc-ui/windows-work-tree/CLAUDE.md`).** WRMI 5850 in LSB sounded hollow / higher than USB; recv log: USB `FIR=[75,5500]`, LSB `FIR=[500,5500]`; LO CUT presses in LSB arrived as HI CUT index 0. Ron's rule: LO CUT / HI CUT send an index (LOCUT 0-4 = 500/300/200/100/75, HICUT 0-6 = 5500..1000), the same in every mode; recv's DSP (`setFilterOffsets`) places the sideband. recv is right. The client turns the index into signed RF edges (Hz) and back (`LowCutHzToIndex` / `HighCutHzToIndex`, UdpRadioService.cs); LSB edges are negative and swapped, so they fall to index 0. HI CUT in LSB and AM likely affected too. Fix is Stew's (send the indexes as they are). Pitch difference LSB / USB: probably the 500 Hz cut, recheck after Stew's fix. **Resume point:** Pi runs recv 3.149, trans 3.146, ms-sdr 3.184, pill 3.12; all pushed. Open: Ron's SSB voice / ALC meter test, thermistor open / short test; LSB / USB recheck after the client fix.
**2026-10-31:** Stew's LSB filter note updated (dfdac8a, pushed): FreeDV received in LSB (Ron: still used on 40 m and below) loses its low end to the stuck 500 Hz cut. Until Stew's fix, use USB / DIG-U for FreeDV if possible. Resume point unchanged.
**2026-10-31:** thermistor open / short test taken off the list (Ron). Open: Ron's SSB voice / ALC meter test; LSB / USB recheck after Stew's client fix.
**2026-10-31 (later): pulled Stew's 4e114e7 + 14a5e1e.** (1) WPF client 10.31.1 fixes Ron's LSB filter bug (buttons send the index in LSB / USB / DIG-U / AM; cuts also pushed once at connect, since ms-sdr restores its last indexes from user_controls.ini and the old client had left index 0 there). Stew's reply: `.mscc-coord/NOTE-FOR-RON-LSB-FILTER-FIXED-2026-10-31.md`. Avalonia client not looked at. Ron: recheck LSB / USB once 10.31.1 is on his PC. (2) Calibration travels with the radio (Windows ms-sdr 3.187 / 3.188, trans / recv 3.151, Geminus FW 2.152 / 2.153): TX / RX I/Q in the 0xC8 store (int16 gain + int16 phase LE, types 1 / 2), new opcode 0x2C CMD_SET_IQ_RECORD (ms-sdr -> trans / recv), and a fix for TX I/Q started from CW mode (mode never reached the radio during an I/Q session; Pi ms-sdr probably has it too). Note `.mscc-coord/NOTE-FOR-RON-CAL-IN-RADIO-GEMINUS-2026-10-31.md`, 3 questions. **Answered Q2 (pushed e322e19, `NOTE-FOR-STEW-CAL-CLEAR-2026-10-31.md`):** clear removed in 3.239 because it was a no-data control request like the save that hung USB; bring back as a control READ like save (handler clears valid bits, marks dirty, returns E_cal_status; Cal_Main writes rows). **Open for Ron:** Q1 slot layout for the pill (check pill calibration.c types 1 / 2 sizes), Q3 Geminus 1 ms tick (main loop vs hardware timer). Later on the Pi: port I/Q-in-radio (0x2C) and the CW-mode TX I/Q fix.
**2026-10-31: all 3 of Stew's cal-in-radio questions answered (pushed).** Q1 ae56fd9 (`NOTE-FOR-STEW-CAL-SLOT-LAYOUT-2026-10-31.md`): I/Q slots int16 gain + int16 phase LE match the pill's `cal_iq_t`, no pill change; 0x2C CMD_SET_IQ_RECORD kept for the Pi (free there). Q2 e322e19 (cal clear: bring back as a control READ). Q3 2bc533c (`NOTE-FOR-STEW-GEMINUS-1MS-TICK-2026-10-31.md`): Geminus main-loop STATUS_BEAT tick OK for now (runs slow, never fast: fine for TUN / PTT, risk for CW); later a 1 ms timer ISR like the Proficio's Key_Delay_Timer / KEY_DELAY_interrupt (`cw.c`: key sampling, 20-40 ms delayed replay to the PA, DAC mute requests, E_ms_tick). **Resume point:** Pi runs recv 3.149, trans 3.146, ms-sdr 3.184, pill 3.12. Open: Ron's SSB voice / ALC meter test; LSB / USB recheck with client 10.31.1; Pi ports of I/Q-in-radio (0x2C) and the CW-mode TX I/Q fix (Stew's ms-sdr 3.187 / 3.188).
**2026-10-31 (afternoon): Stew's cal-in-radio work ported to the Pi. BUILT and RUNNING (11:44 local), NOT COMMITTED (waits for Ron's test).** ms-sdr 3.185: forwards 0x2A (client PHASE slider) to trans and 0x2B (phase in use, from trans) to the client (before, Pi ms-sdr dropped 0x2A: the 10.30+ client's PHASE slider did nothing with Pi servers); tracks the band's offset (0x8B) and phase (0x2B); APPLY (0x57) queues TX / RX I/Q for the radio (0xC8 types 2 / 1, int16 offset + int16 phase LE, sent by Radio_cal_save_tick when out of TX); startup `Radio_cal_load_iq()` after the power load sends bands the radio holds to trans / recv as 0x2C CMD_SET_IQ_RECORD; `G_iq_cal_active` lets mode changes reach the radio during an I/Q session (TX I/Q from CW never transmitted; cleared by CMD_SET_MAIN_FREQ and power cal). trans 3.147: handles 0x2C; sends 0x2B + int32 phase to ms-sdr after CMD_SET_IQ_BAND and after each 0x2A / 0x2B (iq-auto.py still gets its 10-byte reply). recv 3.150: handles 0x2C. Startup log clean: `Radio_cal_load_iq . Finished. RX mask 0x0000, TX mask 0x0000, 0 band(s) from radio` (radio holds no I/Q yet). Ron installed client 10.31.1 (`installers/windows/mscc-net9-R10-31-1-install.exe`); its filter push at connect works (recv 75 / 4000 at connect). "SWR METER FAULT" at connect = Ron's Wi-Fi SWR meter (192.168.12.222) latched fault from an earlier ATU-100 tune, not ours. **Test (Ron, later):** (1) TX I/Q calibration started from CW transmits; (2) PHASE slider changes the phase (log CMD_SET_IQ_PHASE / CMD_GET_IQ_PHASE); (3) APPLY -> `Radio_cal_store_iq ... queued` then `Radio_cal_send_iq ... sent`; (4) restart servers -> `Radio_cal_load_iq ... from radio`. Then commit. Not done: Stew's power start-order change (Pi: bands the radio lacks get factory values; Windows: keep the file value).
**2026-10-31:** I/Q phase is TX only (trans `IQ_PHASE=` in iq.ini, Pi and Windows). recv has no phase correction anywhere (recv-iq.ini = offset only); the client's PHASE slider is on the TX I/Q tab only; the radio's RX slot phase is always 0. RX phase correction: Ron said no, not on the list.
**2026-10-31 (afternoon): I/Q calibration copied into the pill, and the I/Q tools on the Pi.** New `rpi/tools/iq-to-radio.py` (Pi `~/utilities/iq-scripts/iq-to-radio.py`, pyusb): reads `~/.local/mscc/iq.ini` (TX offset + phase) and `recv-iq.ini` (RX offset), sends each band with 0xC8 (type 2 TX / type 1 RX, int16 offset + int16 phase LE, as ms-sdr's APPLY), skips 0 / 0 bands, reads back and prints file vs radio; options `tx`, `rx`, `--dry-run`, `--show`. Needed because `iq-auto.py` / `iq-phase.py` talk straight to trans and save iq.ini only (not the radio). Ron ran it: 10 TX bands (10-160 m) + 12 RX bands all OK; pill healthy after the flash writes (0 abandoned, loop max 1.9 ms). Next server start should log `Radio_cal_load_iq ... 22 band(s) from radio` (same values as the files). RX 630 / 2200 m = 58 in recv-iq.ini, looks like a default, copied anyway. Pi `~/utilities/iq-scripts/`: iq-to-radio.py, cal-test.py, rx-iq-balance/ (rx-image-meter, rx-iq-auto, rx-iq-fill), tx-iq-balance/ (copy; runs on the Windows PC with the IQBD). `~/utilities/README.md` updated (iq-scripts section, iq-to-radio usage, firmware 3.12). The server code (ms-sdr 3.185, trans 3.147, recv 3.150) is still NOT committed: waits for Ron's I/Q test.
**2026-10-31 (afternoon): I/Q from the radio verified.** Server start: `Radio_cal_load_iq . Finished. RX mask 0x0FFF, TX mask 0x03FF, 22 band(s) from radio`; trans got 10 TX, recv 12 RX (0x2C). After NRST (pill reset) + server start: again 22 bands, so the pill keeps them in flash. In effect from start-up: right after the load ms-sdr's start-up sends the current band (CMD_SET_SDR_CORE_BAND), and trans / recv apply that band's values (recv log `IQ_calc. iq_offset: 70` on 15 m); no user band change needed. Still to test (Ron): TX I/Q calibration from CW, PHASE slider, APPLY. Server code still not committed.
**2026-10-31: utility scripts gathered in `utility-scripts/` (mscc-station root). Ron: from now on ALL new utility / test scripts go there.** `utility-scripts/Pi/` (runs on the Pi; installed to `~/utilities`, I/Q ones in `~/utilities/iq-scripts`): audio-stats.py, usb-snap.sh (Pi copy with the ~/utilities path fixes), check-usb, drop-caches, cal-test.py, iq-to-radio.py, rx-iq-balance/ (rx-image-meter, rx-iq-auto, rx-iq-fill). `utility-scripts/Windows/`: audio-stats.cmd (falls back to `..\Pi\audio-stats.py`) + audio-stats.py, copy-pi-ini.cmd / .ps1 / .vbs (stale "Windows ignores IQ_PHASE" comment fixed: trans 3.150+ reads it), tx-iq-balance/ (iqbd-meter, iq-phase, iq-auto, README, Ron's 10-14 ini), and Windows-fixed copies of cal-test.py, iq-to-radio.py (libusb0 / libusb-1.0 handling from audio-stats.py, files from %LOCALAPPDATA%\MSCC-NET9, `--dir`), rx-iq-balance/rx-iq-fill.py (Windows default file). Linux-only (Pi folder only): usb-snap.sh, check-usb, drop-caches, rx-image-meter.py, rx-iq-auto.py (loopback sniffing). A README.md in each folder, Pi-README style. `rpi/tools/` REMOVED (git rm staged, all 10 files had identical copies); older notes that say rpi/tools are history. Blackpill `troubleshooting/` kept (Ron). Pi ~/utilities/README.md: watch interval corrected to every half second. NOT committed: the utility-scripts folder + rpi/tools removal (staged), and the server code (ms-sdr 3.185, trans 3.147, recv 3.150) waiting for Ron's I/Q test.
**2026-10-31: utility-scripts pushed (77e3b8c).** Top-level `utility-scripts/README.md` says what the folder is for (one place for every utility / test script; not part of servers, client or firmware; any new script goes there, Pi\ or Windows\, with a README entry; build / packaging / flashing scripts stay with their projects). `utility-scripts/Pi/.gitattributes` keeps LF on every checkout. Ron: "Just makes more sense to have utilities / test scripts in one location." Before the push two local commits were rewritten (filter-branch, only rpi/CLAUDE.md wording changed) to keep a local-only topic out of the notes; the old version is in a local backup branch (never push it). **Resume point:** Pi runs recv 3.150, trans 3.147, ms-sdr 3.185 (NOT committed, waiting for Ron's I/Q test: TX I/Q from CW, PHASE slider, APPLY), pill 3.12 holding 22 I/Q bands. Open: SSB voice / ALC meter test; LSB / USB recheck with client 10.31.1.


## Moved from rpi/CLAUDE.md on 2026-11-14 (finished entries, verbatim, original order)

**HOLD LIFTED 2026-11-10 (Ron: Stew gave the go-ahead to implement all the changes). Was: ON HOLD (Ron, 2026-11-02): no new changes until Stew catches up on the important Pi work
(automatic NB, spectral NR, NR Low/Med/High, client controls).** When he pushes: pull, short
review of what he ported, say if the Pi needs a matching change. Don't start new work meanwhile.

**Stew's list, Ron doing it (2026-11-10): servers first, then client. Versions after all server changes.**
- Done (not on air yet, Windows rig needed): Windows recv automatic NB (blanker.c + struct), Windows recv spectral NR (denoise()).
- TX IQ RESET ALL (Ron 11-10): ms-sdr writes the FACTORY TX IQ values into the radio (no clear). A radio with factory values = calibrated (to factory), unlike a new rig with nothing stored. DONE in Pi ms-sdr (iq.c, factory_seed.c/.h; copied to V:, syntax-checked, NOT built) and Windows ms-sdr (built). Ron tests later (needs a full calibration after).
- Windows ms-sdr, built, not on air: heat sink 0x06 read + 30 s log (proficio-temperature.c); WiFi SWR meter port (swr_wifi_meter.c added to vcxproj, Windows clock / thread-name shims; 0x0F fault, 0x2E reset, tune flag, SWR_* ini keys; needs SWR_METER_TO_GUI=1 in mscc.ini, firewall UDP 6999, meter sending to the PC); Pill_stats (PILL_STATS=1). Windows build auto-bumped ms-sdr 3.189 -> 3.191.
- Server list done 2026-11-10. Versions (11-10): Windows recv 3.148 -> 3.152 (= Pi recv 3.152 features), Windows ms-sdr 3.196 and Pi ms-sdr 3.196 (built on the Pi into ~/mscc/ms-sdr, runs after the next mscc restart). Pi recv unchanged (3.152, rename only). Next: the client list.
- **Hard calibration resets (Ron 2026-11-10).** Soft = host files only (radio wins at next start); hard = files AND radio. New client opcode 0x2F CMD_SET_CAL_RESET_ALL (1 byte: 1 RX IQ = 0/0 all bands, recv + radio; 2 QRP power = factory table, trans saves + radio; 3 AMP = trans rewrites amplifier_cal.ini -99, host only). Pi ms-sdr Radio_cal_reset_all (power_calibration.c) + trans CMD_SET_AMPLIFIER_CALIBRATION_RESET switched on (udp_thread.c): on V:, syntax-checked, NOT built. Windows ms-sdr 3.197 + trans built. Client 11.10.7: RESET ALL on RX IQ / QRP CAL / AMP CAL (Yes/No, then OK-only pop-up). Existing hard resets: TX IQ RESET ALL (0x8D = CMD_SET_IQ_DEFAULTS, factory to radio since ms-sdr 3.196), FREQ CAL RESET. Reset configuration = client settings only (keeps iq/recv-iq/freq_cal/power_cal/amplifier_cal.ini + cal).
- **Client list DONE 2026-11-10 (client 11.10.13, not on air on a Windows rig):** NB Low/Med/High + pulse 50..510 + NB sent at connect; NR Low/Med/High; heat sink display; SWR RESET via 0x2E; 0x30 server-message pop-up; RESET ALL pop-ups; Reset configuration = settings only; hard resets (0x2F); PADDLE checkbox (0x9D); cal band-box fix; firmware-update pop-up (0x2D store=0: OK-only, once per run, lists minimum firmware); SWR fault now also turns TUN off (TESTED Ron 11-10 on the Pi: fault, pop-up, TUN off, RESET via ms-sdr). 0xD7-0xD9 removed from the client.
- **Servers 2026-11-10 (late):** Pi + Windows ms-sdr 3.200: server last-used band stack (Last_used.ini, vfoa/vfob) REMOVED (thread was commented out in both, client never asked for it, client keeps its own last-used); 0xD7-0xD9 reclaimed (usbavrcmd.h free list). Pi client keep-alive timeout 45 -> 15 s (= Windows). trans 3.157 both (Pi built). Pi runs ms-sdr 3.200, trans 3.157, recv 3.152, pill 3.15.
- **Pill 3.15 (2026-11-10, flashed, works):** heat sink below -40 C = OPEN (-99), above 150 C = SHORT (999); was clamped, an unplugged thermistor showed -40 C (pull-up is on the board, Ron).
- **Windows installer now builds from `mscc-ui/installer/windows/`** (payload\ + MSCC.aip + stage.ps1 + README), not C:\mscc-net9. Actual Installer: `actinst.exe /S MSCC.aip` -> `installers/windows/`. PSoC firmware = newest versioned build per radio (1.236 / 2.153 / 3.243 / 4.244 / 5.153 / 6.236 / 7.244 / 8.243), Black Pill = 25 MHz bins + flash tools (no 8 MHz). R11-10-13 built. Open: MSCC-Remote has no Release build (Oct-31 copy in payload); MSCC.aip paths are Ron's PC.
- **Pi package mscc 1.0.77** (2026-11-10): ms-sdr 3.200, trans 3.157, recv 3.152; built on the Pi from a /tmp copy (CR stripped from scripts), in `installers/rpi/`.
- **2026-11-10 (night): Pi and Windows servers made functionally identical (Ron).** Platform code (USB, sockets, paths, audio device discovery / ALSA-Pulse vs MME) stays different. NOT on the air yet.
  - ms-sdr: Windows 3.206, Pi 3.203. Windows got the Pi's 1 s Flusher loop (client timeout ~15 s, was ~45 s; core check ~10 s), CAT `MDn;` mode set (was commented out), mode saved to startup.ini on every change, signed RIT +-500 Hz, Radio_cal_startup_load (radio cal read at first connect if startup missed it), safe cal copies (.tmp + replace, .bak of parked, trans INITIALIZE before park), default cw.ini / mscc.ini instead of stopping, host name fallback 127.0.0.1, the Pi user_controls.c (ALC on/off saved, NR, pan guards), FARNSWORTH 1-4 -> 5. 3.201: a client leaving no longer stops the Windows servers. Both: TUNE never saved to startup.ini (stop while TUN on came back in TUNE); `last_used.c/.h` renamed `startup.c/.h` (only startup.ini code left); WATERFALL_STATUS `;` fixed in user_controls.ini.
  - recv 3.153 both. Windows got the Pi udp_thread.c (socket error = log + continue, digital volume per mode, I/Q band guard), float FREQ CAL / CW snap magnitudes, server pop-ups (I/Q device / digital speaker / audio start failed), 0xF4 keep-alive with source id, recv-iq.ini reload after create, Audio_Device_Error exits (was a NULL crash with no I/Q device), normal stop just exits. Both: 10 ms pause after a recvfrom error.
  - trans 3.158 both. Ron's choices: mic gate at key-down (35 / 200 ms + 10 ms fade) and 5 ms TUN ramp on both; ALC ceiling 1.0 digital / 0.89 voice on both (Pi digital was 0.89); line gain 2.5x for Digital, Remote-Digital AND Remote-Phones (only the local Phones mic gets 6.3x); voice ALC meter 0-10 dB. Windows got the Pi remote mic receiver (100 ms priming, resync >250 ms, +/-300 ppm trim, no pitch steps), recvfrom skip (was: trans stopped and handled a stale buffer), pop-ups, exit(1) on start-up failure, 0xF4 keep-alive, Flusher no busy spin, power-table log index fix. Pi got the phase reapply after CMD_SET_IQ_DEFAULTS. Both: a close error at a normal stop is only logged. TO CHECK ON AIR: Pi WSJT-X power / ALC (digital gain 6.3 -> 2.5, ceiling 0.89 -> 1.0); Windows Remote-Phones voice needs ~8 dB more mic gain.
  - Client: WPF 11.10.14 and Avalonia 1.6.93 / 1.6.94: SERVER button = Connect / Disconnect with a tooltip saying whether the servers start / stop; checkbox "Launch Servers" on both; Avalonia sends CW BW before CW pitch at connect (recv "CW Bandwidth NOT SET").
  - Installers: Windows R11-10-14, Pi mscc 1.0.78 + mscc-avalonia 1.6.94 in `installers/`. Pi runs ms-sdr 3.203, recv 3.153, trans 3.158, pill 3.15.
  - **Installers are GitHub Release downloads, not committed** (`.gitignore` installers/windows/*.exe, installers/rpi/*.deb; Ubuntu installers/linux unchanged). Release R11-10-14: https://github.com/w4mmp-ron/mscc-station/releases/tag/R11-10-14.
  - **Pi = 2 packages since mscc 1.0.79 (Ron):** `mscc` now also carries PortAudio (/usr/local lib + headers, loader file /etc/ld.so.conf.d/mscc.conf), MSCC Init, MSCC Volume, PSoC firmware (/usr/share/mscc/firmware/<Radio>/, same versioned files as the Windows payload) and Black Pill 25 MHz firmware (/usr/share/mscc/firmware/black-pill/). Conflicts/Replaces/Provides mscc-portaudio, mscc-init(-gui), mscc-volume(-gui), mscc-firmware: apt removes them. build-deb.sh takes the firmware from mscc-ui/installer/windows/payload (run stage.ps1 first) and PortAudio from rpi/mscc-portaudio/mscc-portaudio_19.8.3_arm64.deb. Installed on the Pi 2026-11-10 (old packages removed, servers running); mscc-ui 0.6.31 purged from the Pi. Optional: `mscc-avalonia`. INSTALL-FOR-PI .docx/.pdf not regenerated.

**2026-11-11 (finished, Ron): Release R11-11-2.** Windows `mscc-net9-R11-11-2-install.exe` (WPF 11.11.2,
ms-sdr 3.208); Pi `mscc_1.0.80_arm64.deb` (= 1.0.79 + ms-sdr 3.206, md5 = the running ~/mscc/ms-sdr) and
`mscc-avalonia_1.6.95_arm64.deb` (both also on V:\; both INSTALLED on the Pi 2026-11-11, servers
were stopped, /dev/tnt0-1 and the .ini files fine; Avalonia 1.6.95 not run yet). INSTALL docs +
Windows CHANGELOG updated, INSTALL-FOR-PI .docx / .pdf regenerated. Pi runs ms-sdr 3.206, recv 3.153,
trans 3.158, pill 3.15.
- **WSJT-X right-click Tune (CAT `AC111;`) DONE, tested on the Pi (Ron).** Capture (strace on
  ms-sdr's /dev/tnt0): each right-click sends `AC111;ID;`, nothing to end it, and WSJT-X sends
  NOTHING for a right-click while its tune runs (so the tune can't be stopped from WSJT-X).
  ms-sdr `comm-port.c`: AC111; = MSCC TUN carrier like the client TUN button (mode T, tune power if
  the client sent one, Tune_button = SWR meter tune flag before RF, TX on), client gets
  CMD_SET_TX_SET_BY_SERVER 1 (as CAT PTT; client locks PTT/TUN). Ends after 10-11 s
  (`Cat_tune_tick()` from the 1 s Flusher loop, main.c), at a second AC111; (other apps), or
  before a CAT `TX;`; then TX off, old mode back, TX released. Ended elsewhere (SWR / temperature
  fault, client mode change / disconnect): tick puts the mode back if still T and releases TX.
  An AC111; within 1 s of the start is ignored (seen 4 times ~150 ms after the start, cause not
  found, not seen under strace). Log: `CAT tune. START / STOP (reason) / ignored`. Files: comm-port.c,
  main.c, extern.h in both trees. Pi ms-sdr built (version.h on the Pi 3.206), Windows ms-sdr 3.208
  built, not on a Windows rig. Tune power shows 0 in the log until the client sends it (trans
  then uses its own; Ron: power looked normal).
  **Later 11-11 (Avalonia 1.6.95 on the Pi):** a second right-click 6.4 s into a tune DID arrive
  (`MD2;` + `AC111;`, 16:00:49 UTC) and stopped it; at 2-3 s earlier it sent nothing. Ron: WSJT-X
  right-click Tune itself looks buggy (sometimes sends, sometimes not). ms-sdr handles both. Note:
  an `MD2;` during a CAT tune changes the mode with the carrier still keyed (harmless in USB);
  option, not done: hold CAT mode sets until the tune ends.
  Avalonia 1.6.95 meter in watts: fine (Ron). SWR fault 3.40 on 15 m at 16:00:33 = Ron's
  left-click Tune by mistake (antenna not matched there), not a bug.
**2026-11-11 (finished, Ron): Release R11-11-4.** S/W **SPECTRUM SIZE** slider (20-80 %, default 40,
under Climbing): spectrum share of the panadapter height, waterfall gets the rest (was fixed: WPF 38 %,
Avalonia 40 %; the waterfall rows are resampled to its height, so nothing else changes). Global, not per
band. WPF: SpectrumColorSettings.SpectrumSplitPercent (renderer + peak marker), SpectrumWaterfallSettings
`SPECTRUM_SPLIT_PERCENT` in MSCC_Client.ini, PanadapterControlsWindow slider. Avalonia:
SpectrumDisplaySettings.SpectrumSplitPercent (LoadExtras), SpectrumRenderer, ClientSettingsStore
`SPECTRUM_SPLIT_PERCENT`, SpectrumWaterfallViewModel + window. WPF 11.11.4 TESTED by Ron (Windows, saved 49 %,
no log errors); Avalonia 1.6.96 INSTALLED on the Pi 2026-11-11 (over 1.6.95), Ron trying it (he reports later if it fails). Installers: Windows `mscc-net9-R11-11-4-install.exe`
(servers unchanged), Pi `mscc-avalonia_1.6.96_arm64.deb` (on V:\, installed); `mscc_1.0.80` unchanged. Commit e203740,
GitHub Release R11-11-4 (the .exe + both .deb). Actual Installer: run `actinst /S` from PowerShell only (Git Bash
turns `/S` into a path, the GUI opens and nothing is built).
Option, not done: drag the divider on the display.
- **RELEASE STATUS (Ron, 2026-11-11): Windows and Pi are READY FOR RELEASE at R11-11-4** (GitHub Release
  R11-11-4: mscc-net9-R11-11-4-install.exe, mscc 1.0.80, mscc-avalonia 1.6.96; Latest is now R11-11-5 with
  mscc 1.0.82).
  **Ron, 2026-11-11 end of day: his work is finished, "the ball is in Stew's court".** Everything works: WPF,
  Avalonia, Windows servers and Pi servers. Waiting on Stew: the Debian / Ubuntu implementation and the
  original-firmware check (zip sent; missing files). (The stale mscc-firmware line in Linux guide 2.7 was
  fixed by Ron's 2026-11-12 draft edit, 3c0a393; dropped from Stew's list, Ron.) The Debian (Ubuntu x86_64,
  `linux/`, `installers/linux`) implementation is WAITING ON STEW; don't release it until he's done.
  **Ron 2026-11-13: once Stew is done, the Debian amd64 .debs become part of every "finished"** (build, attach to the Release, Addendum).
  **Build warnings fixed 2026-11-13 (Ron asked), Pi + Windows, NOT built for real, not on air:** Pi test build in /tmp (BINDIR=/tmp, version.h untouched) went 43 -> 12 warnings. Fixed: %ld for 32-bit values -> %u / %d (log text only); proficio-temperature Initialize_band_data memsets cleared 10 bytes, not the arrays (uint32_t / int x 10); message_queue dequeue memset(sizeof pointer) -> buffer[0] = 0, strncpy bound PATH_MAX - 1; startup.c startup_record = {0} (freq / mode were garbage if startup.ini lacked a key); recv CMD_SET_BW_LOCUT default case keeps previous_low_cut; Version.day memcpy + terminator; User_Controls_Process casts; Pi slen socklen_t; trans #ifndef RON. Left (false alarms): strncpy size-1 copies, factory_seed snprintf PATH_MAX, unused TX_Hold_Timer. Pi files copied to V:. **Found, NOT fixed (asked Ron):** message_queue.c `char M_freq_queue[PATH_MAX][MAX_COMMAND_QUEUE]` has its sizes swapped (50-byte rows; long messages run into the next slot, dequeue memset PATH_MAX wipes the following ~80 slots) - both trees.
- **2026-11-14 (finished, Ron): Release R11-14-3** = Windows `mscc-net9-R11-14-3-install.exe` (WPF 11.14.3, recv 3.155; ms-sdr 3.213 / trans 3.159 unchanged) + Pi `mscc_1.0.95_arm64.deb` (recv 3.156; vs 1.0.94 only control + sdrcore-recv, 129 files, modes same, no CR) + `mscc-avalonia_1.7.02_arm64.deb` (Avalonia not tested: Ron). **RX I/Q band clear:** recv kept the RX I/Q calibration band (iq_band) after the session ended, so later 0x52 / 0x57 still went to that band. Now recv (Pi / Linux 3.156, Windows 3.155, udp_thread.c) sets iq_band = NO_IQ_BAND on a band change (CMD_SET_SDR_CORE_BAND, log `IQ band N cleared`), and WPF + Avalonia `LeaveRxIqSession` (leave tab, START off, disconnect, close) send 0x58 band 0 (recv: `Unknown band 0. Ignored` = cleared; ms-sdr forwards it on the RX path). No longer local (Ron: normal commit). TESTED by Ron on the Pi with WPF 11.14.3: leave tab -> band 0 cleared twice, START again sets 15 m, band change loads 20 m offset. Also: RX IQ START order fix (0x55 RX before 0x58) shipped in R11-11-2. Lessons: on the Pi `make clean` needs the same BINDIR (plain clean deleted ~/mscc/sdrcore-recv); the Windows sln build bumps ms-sdr version.h even with no code change (put back 3.213); run actinst /S from PowerShell, not Git Bash.
- **RX IQ START order fixed (client):** WPF `StartRxIq` and Avalonia `StartRxIqAsync` now send
  0x55 RX before 0x58 band (ms-sdr iq.c routes 0x58 by the last 0x55; band first after a TX I/Q
  session took the TX path: retune to the TX cal freq + trans TUNE). WPF 11.11.1, Avalonia
  1.6.95 (csproj + DEBIAN/control), both built, not tested on the rig. Test: TX I/Q cal, then
  RX IQ START: rig stays on the receive freq, trans not in TUNE.
- **2026-11-14 (finished, Ron): Release R11-14-2** = Windows `mscc-net9-R11-14-2-install.exe` (WPF 11.14.1, ms-sdr 3.213; recv / trans unchanged) + Pi `mscc_1.0.94_arm64.deb` (ms-sdr 3.210; vs 1.0.93 only control + ms-sdr) + `mscc-avalonia_1.7.01_arm64.deb` (vs 1.7.00 only app dlls / deps / README / control; same 3 CR files; INSTALLED on the Pi with mscc 1.0.94, TESTED by Ron: carrier on centre in Avalonia too; Windows R11-14-2 installed; version is in packaging/mscc-avalonia/DEBIAN/control AND the csproj). Contents below.
- **2026-11-14 (later): spectrum centre fixes.** (1) AM: ms-sdr `AM_OFFSET_RX` -11860 -> -12000 (Pi, Windows, Debian `extern.h`); Pi ms-sdr 3.210 running (old kept as ~/mscc/ms-sdr.3.209), TESTED by Ron: AM carrier on centre. Windows ms-sdr not rebuilt yet. (2) Client pixel -> bin mapping (WPF `SpectrumDisplayControl.xaml.cs`, Avalonia `SpectrumRenderer.cs` + `SpectrumDisplayControl.axaml.cs`): new `FracToBin` (bin k centred on k/N, 0 Hz = bin N/2), pixel x = [x/W,(x+1)/W); was (N-1)/(W-1) + truncation: carrier 1 px left at 1x (hidden under the centre line, right skirt under the filter edge) and 1 px (~26 Hz) right at 4x (measured from Ron's PNGs, 687 px plot). Peak marker bin/N, marker X and WPF ticks floor(frac*W). WPF 11.14.1 installed in C:\mscc-net9, TESTED by Ron: carrier on centre at 1x and 4x. Avalonia compiles, not built as .deb / not tried. Left alone: recv fftshift in dsputils.c drops the 0 Hz bin (LO between bins N/2-1 and N/2), under 1 px.
- **2026-11-14 (finished, Ron): Release R11-14-1** = Windows `mscc-net9-R11-14-1-install.exe` (ms-sdr 3.212; WPF 11.13.6, recv 3.154, trans 3.159 unchanged) + Pi `mscc_1.0.93_arm64.deb` (ms-sdr 3.209; vs 1.0.92 only control + ms-sdr differ, 129 files, modes same, no CR) + `mscc-avalonia_1.7.00_arm64.deb` (unchanged). ms-sdr `comm-port.c` (Pi, Windows, Debian trees): CAT `TX0;` and `TX1;` key like `TX;` (same transmit audio source; log "parse_record. TX Received (TX1;)"). Found while fixing fldigi PTT on the Pi, but NOT the fldigi cause: with Hamlib TS-2000, fldigi sends plain `TX;` even with "Audio on Auxiliary Port" ticked (log 2026-11-14 18:10 UTC); fldigi only needed a restart to apply its PTT settings. fldigi now keys by RTS on /dev/tnt1 (GPIO/Serial PTT tab, Hamlib keeps frequency); after changing fldigi PTT settings restart fldigi. Pi build in /tmp/msbuild (servers kept running). mscc 1.0.93 INSTALLED on the Pi 13:10 local; fldigi Hamlib PTT keys (sends `TX;`); `TX1;` TESTED 18:14 UTC (python on /dev/tnt1 with RTS/DTR held off, client connected): transmit 1 -> "TX Received (TX1;)", `RX;` -> transmit 0. `TX0;` same code, not sent. Windows R11-14-1 INSTALLED 2026-11-14 (ms-sdr 3.212 md5 = payload); Windows TX0; / TX1; not tried yet (Ron tests later).
- **2026-11-13 (finished, Ron): Release R11-13-5** = Windows `mscc-net9-R11-13-5-install.exe` (WPF 11.13.6; servers unchanged) + Pi `mscc-avalonia_1.7.00_arm64.deb` (not tried on the Pi yet; Ron tests later) + `mscc_1.0.92_arm64.deb` (unchanged). Addendum, Windows CHANGELOG + INSTALL.md, installers/rpi/INSTALL.md updated.
- **2026-11-13 (late): Clear RIT resets only the slider (Ron)**, RIT checkbox stays as it is; log "Clear RIT: offset reset to 0". WPF 11.13.5 (`MainWindow.xaml.cs` ClearRit_Click) TESTED by Ron (installed into C:\mscc-net9 by hand; built with CopyToMsccNet9 switched off via `-p:CustomAfterMicrosoftCommonTargets=<empty target file>`). Avalonia 1.6.99 (`MainViewModel.cs` ClearRit; 1.6.98 -> old). **Then RIT made sticky (Ron):** WPF 11.13.6 saves RIT_ON / RIT_OFFSET in MSCC_Client.ini (same keys as Avalonia; SpectrumWaterfallSettings + MainViewModel: loaded into both VFOs at start, saved on change, sent once per session at the first frequency report, log "RIT pushed at connect"); Avalonia 1.7.00 no longer turns RIT off / offset 0 on disconnect (that was saved, so RIT never came back), Clear RIT tooltip "Offset -> 0 (RIT stays on)". WPF 11.13.6 installed into C:\mscc-net9 and TESTED by Ron (RIT stays on after a restart); Avalonia 1.7.00 built, NOT installed: `mscc-avalonia_1.7.00_arm64.deb` in installers/rpi + V: (1.6.99 -> old, never installed). Not committed; goes out at "finished".
- **2026-11-13 (finished, Ron): Release R11-13-4** = Windows `mscc-net9-R11-13-4-install.exe` (WPF 11.13.4, ms-sdr 3.211, recv 3.154 / trans 3.159 unchanged) + Pi `mscc_1.0.92_arm64.deb` (ms-sdr 3.208, recv 3.155, trans 3.160) + `mscc-avalonia_1.6.98_arm64.deb`. Comm / CAT parity, details below (the "BUILT, NOT installed" heading is history; all installed and tested as noted). The push at "finished" was refused: Stew had pushed f09daed (12:38: linux/ = copy of the Pi trees, Pi recv 3.155 / trans 3.160 Ultimus sound-card fix, postinst Pi-only AudioInjector guard, status report + MSCC Init "ultimus", control 1.0.91, Avalonia build script -Arch amd64 + launcher in git). Rebased (no conflicts); comm-port.c + version.h copied into linux/ms-sdr-linux (amd64 test build OK); Stew's 6 recv/trans files copied to V: (V: was = old git), built on the Pi into /tmp -> rpi/mscc-binaries -> mscc 1.0.92 (my unreleased 1.0.91 -> old). Avalonia 1.6.98 rebuilt with Stew's new script on Windows (works: same 234 files, .desktop now LF). 1.0.92 + 1.6.98 INSTALLED by Ron 13:54, servers started 13:55, logs clean (heat sink OPEN = thermistor removed, expected).
- **2026-11-13 later: comm / CAT parity (Ron: "bring everything up to par"), BUILT (compile only), NOT installed, NOT on air:**
  ms-sdr Pi + Windows `comm-port.c`: PIN=2 (DTR -> DCD) now keys (tested `fDCD`, never set; now `fRLSD`, logs once per key); Pi now sends the client "CAT PORT x NOT AVAILABLE - CAT DISABLED" like Windows; Windows open-failure log shows the real Win32 error (was fixed ERROR_FILE_NOT_FOUND). Windows ms-sdr built 3.211 (Release/windows-wpf). Pi real build on the Pi with BINDIR=/tmp/msbuild (servers kept running, ~/mscc untouched): ms-sdr 3.208 (V: version.h bumped), same known warnings only -> rpi/mscc-binaries/ms-sdr -> `mscc_1.0.91_arm64.deb` (vs 1.0.90: only control + ms-sdr differ, 129 files, modes same, no CR) in installers/rpi + V:, NOT installed; 1.0.90 -> installers/rpi/old. `mscc-avalonia_1.6.97_arm64.deb` (only the app dlls / deps / README / control differ; same 4 CR text files as 1.6.96) in installers/rpi + V:, NOT installed; 1.6.96 -> old. Note for Stew: `.mscc-coord/NOTE-FOR-STEW-AVALONIA-MIGRATION-FROZEN-2026-11-13.md`. Windows INSTALLED into C:\mscc-net9 2026-11-13 13:16 (MSCC closed): WPF 11.13.1 (dotnet build, CopyToMsccNet9; md5 = bin) + ms-sdr-MKII.exe 3.211 (old 3.209 kept in the session scratchpad); recv / trans unchanged. No Windows installer built yet. Pi mscc 1.0.91 + mscc-avalonia 1.6.97 INSTALLED by Ron 13:18, logs clean; WPF client connected, Remote Phones / Digital switching + saved choices seen working in the log. WPF 11.13.2 (13:3x, installed): saving COM port / baud / PTT line in Settings reopens Remote CAT at once (CAT reads comm-port.ini only when it opens; Avalonia has no COM settings screen and reads it at each CAT open). WPF 11.13.3 (installed): old bug fixed, every WPF start wrote PIN=0 to comm-port.ini (CommPortSettingsPanel.RefreshPortList cleared _suppressSave inside OnLoaded, so the baud SelectedIndex saved before PinCheck was loaded); also hit local ms-sdr, which reads the same file. TESTED 13:33-13:35 (WPF on Windows, Pi host, Remote Digital): CAT PTT, RTS PTT (CTS on/off -> Pi transmit 1/0 same second), right-click Tune (AC111; -> TUNE, transmit 1, stop after 10 s, back to USB). Remote window CAT line now shows the PTT line ("PTT: CAT or RTS" / "CAT or DTR" / "CAT (PTT line off)"; WPF hint no longer says PTT=CAT): WPF 11.13.4 installed, Avalonia 1.6.98 .deb (installers/rpi + V:, not installed; 1.6.97 -> old).
  Clients, WPF (11.13.0 compile) + Avalonia 1.6.97 (built, no .deb yet): shared `KenwoodTs2000.cs` now identical in both MSCC.Core copies: AC111; CAT tune in Remote CAT = press TUN, same rules as ms-sdr (10 s, second AC111; stops, 1 s debounce, CAT TX; wins, TUN off elsewhere = timer stops; IF; reports TX while tuning; CAT close ends it). WPF only: RTS/DTR PTT in Remote CAT (PIN=1 CTS / 2 DCD, checked in the read loop every ~10 ms, read wait 10 ms when a line is set; non-overlapped handle), Remote devices saved by name per path (REMOTE_PLAY_NAME / REMOTE_MIC_NAME / REMOTE_DIGI_PLAY_NAME / REMOTE_DIGI_MIC_NAME; Digital falls back to Settings Digital Speaker/Mic, then VAC), Phones/Digital choice saved (REMOTE_DIGITAL). Not changed: WPF Settings PIN is a checkbox (0/1) and its Save turns PIN=2 into 1.
  The WPF compile check half-ran CopyToMsccNet9 while MSCC was open: deleted MSCC.Wpf.deps.json + runtimeconfig.json from C:\mscc-net9; restored from installer payload (md5 match with the installed exe/dll).
- **Pi ms-sdr version.h in git (Ron 2026-11-13):** git had 3.196 (V: is the Pi build copy and auto-bumps; version.h is never copied TO V:). Copied V: -> repo once (3.207); goes out at the next "finished". Ron: do this at EVERY "finished" (V: -> repo only).
- **2026-11-13 (finished, Ron): Release R11-13-3** = Windows `mscc-net9-R11-13-3-install.exe` (= R11-13-2 + Black Pill flasher hint: BOOT jumper + NRST, was "hold KEY (PA0) + NRST") + Pi mscc 1.0.90 / mscc-avalonia 1.6.96 (unchanged). New guide `docs/manuals/MSCC-Updating-Firmware.md` + .pdf (from Claude Doc https://claude.ai/artifact/WRn3wDtpFP83iLjteCZyKy; PDF via `make-addendum-pdf.py <file>.md`).
- **2026-11-13 (finished, Ron): Release R11-13-2** = Windows `mscc-net9-R11-13-2-install.exe` (ms-sdr 3.209, recv 3.154, trans 3.159) + Pi `mscc_1.0.90_arm64.deb` (ms-sdr 3.207, recv 3.154, trans 3.159; installed, logs clean) + mscc-avalonia 1.6.96 (unchanged). Build-warning fixes + message queue fix. TESTED on the Pi 2026-11-13: low cut 75/100/200/300/500 all applied; message queue: two start-up warnings (digital speaker + digital mic, ~75 chars each, queued in the same ms) both delivered whole and in order at connect (test: both digital .ini set to missing names, restored after, md5 checked). Details below.
  **R11-13-2 BUILT 2026-11-13 (Ron: fix queue, real build, bump release; NOT released, NOT on air):** message_queue.c M_freq_queue -> [MAX_COMMAND_QUEUE][PATH_MAX] both trees. Pi real build in ~/ (V:): ms-sdr 3.207 (auto), recv 3.154, trans 3.159 (extern.h, both trees; Makefiles do not track headers: make clean needed after a header change); binaries in Pi ~/mscc (servers were stopped) = rpi/mscc-binaries -> Pi `mscc_1.0.89_arm64.deb` (installers/rpi + V:, not installed). Windows MSBuild MFC: ms-sdr 3.209, recv 3.154, trans 3.159; stage.ps1 + actinst -> `mscc-net9-R11-13-2-install.exe` (MSCC.aip AppVersion 11.13.2; my first edit doubled the BOM -> actinst exit 1, fixed). Windows CHANGELOG + INSTALL.md, installers/rpi/INSTALL.md updated; Addendum at "finished".
  **Rebuilt same day:** trans alc.c start-up line + Windows recv main.c line print VERSION_MAJOR.MINOR (were "trans 3.158" / "recv 3.151" text); versions kept. Pi trans built into /tmp (servers running) -> `mscc_1.0.90_arm64.deb` (= 1.0.89 + trans only; 1.0.89 was installed by Ron and ran clean: no log errors). Windows recv + trans rebuilt, R11-13-2 .exe rebuilt (ms-sdr 3.209 unchanged).
  **Test build 2026-11-13:** Pi server source builds on amd64 unchanged (WSL Debian, `make PORTAUDIO=distro`; warnings 36 / 4 / 3, no errors; not run, deleted). Told Stew: `.mscc-coord/NOTE-FOR-STEW-PI-SERVERS-BUILD-ON-AMD64-2026-11-13.md` (goes out at the next "finished").
  **Added for Stew 2026-11-12 (Ron's OK), an option for him:** Visual Studio build bookkeeping files in
  `mscc-ui/windows-work-tree` (66 tracked: `.tlog`, `.recipe`, `.iobj`, `.ipdb`, `.lastbuildstate` for
  ms-sdr / recv / trans) change at every Windows build. If he likes, git could ignore them (.gitignore +
  `git rm --cached`). Until then they are left uncommitted (Ron's choice).
  **For Stew 2026-11-12 (Ron asked for a clear note): test the keyer update on a PSoC radio.** PSoC firmware
  with CMD_KEYER_BOOT 0x9F (Proficio MKII PTT 3.244 / ATU 4.245, Geminus MKII 2.154, Ultimus MKII PTT 8.244 /
  ATU 7.245) builds clean but has never run on a radio (Ron has none with the daughter board). Steps:
  `radio-psoc-firmware/STEW-KEYER-UPDATE-TEST.md`.
- **2026-11-12 (finished, Ron): Release R11-12-4** = Windows `mscc-net9-R11-12-4-install.exe` + Pi `mscc_1.0.85_arm64.deb`:
  Pi window "Black Pill Firmware Update" (`firmware-programmers/rpi/stm32-flash/stm32-app-flash-gui.py`, link
  `stm32-app-flash.gui`, menu `mscc-blackpill-flash.desktop`; runs `stm32-app-flash.sh`; lists only
  `<ver>-app.bin`, refuses boot-app / bootload-PA8; refuses while the servers run). No sudo: udev line for
  `0483:df11` (flash mode) in `99-proficio.rules`. Tested by Ron on the Pi (3.16 over 3.16, servers check),
  files put there by hand; the .deb copies are byte-identical. Windows `stm32-app-flash.exe`: same servers
  check, file filter and refusal (built, not run yet). `keyer-flash.py` Windows servers check fixed (it matched
  `recv.exe` / `trans.exe` exactly; real names `mscc-recv.exe` / `Mscc-trans.exe`), keyer .exe rebuilt.
  1.0.85 = 1.0.84 + those files (compared file by file). Client, servers, firmware unchanged. Installed on the Pi
  2026-11-12 (Ron asked; MSCC menu files moved away first to test the install: all came back).
  **DONE 2026-11-13, mscc 1.0.86 BUILT and INSTALLED on the Pi by Ron 2026-11-13 (new log line seen, tty0tty rebuilt, config untouched; not released; = 1.0.85 + this line only, compared file by file; installers/rpi + V:; WSL Debian build from a /tmp copy, file modes 755, 1.0.85 had 777):** postinst udev log line now also names Black Pill flash mode 0483:df11 (was "(Proficio app 16c0:05dc + HID LOADER 04b4:b71d)");
- **2026-11-13 (finished, Ron): Release R11-13-1** = Pi `mscc_1.0.88_arm64.deb` (postinst udev log line names Black Pill flash mode; MSCC Start / Stop / Status window); Windows R11-12-9 .exe and mscc-avalonia 1.6.96 unchanged. Addendum (.md + .pdf), INSTALL-FOR-PI (.md/.docx/.pdf) and installers/rpi/INSTALL.md updated. Details below.
  **mscc 1.0.87 BUILT 2026-11-13 (not installed, not released; = 1.0.86 + this only, compared file by file):** MSCC Start / Stop / Status menu gave no message: the X11 desktop (openbox + lxpanel-pi) has no notification service, notify-send failed silently. mscc-desktop-ctl.sh notify() now falls back to a small Tk window (OK button, closes after 5 s; errors red and stay). Tested on the Pi from /tmp (Ron saw it).
  **mscc 1.0.88 BUILT + INSTALLED 2026-11-13, works (Ron) (not released; = 1.0.87 + this only):** 1.0.87 INSTALLED by Ron (Start window seen). Window now reads the mscc start / stop lines: "Servers already running (...)", "Started: x. Already running: y.", "Servers were not running.", "Stopped: x. Were not running: y."; "may have exited" = orange, "could not stop" = red, both stay until OK (Status warning too). mscc.sh unchanged. Wording tested with sample output; already-running window seen by Ron on the Pi. INSTALL-FOR-PI.md: menu-window line (was libnotify), package names now mscc_<version> / mscc_*_arm64.deb + "versions in the Addendum" (was 1.0.84 / 1.6.96); .docx + .pdf regenerated 2026-11-13.
  **2026-11-12 (Ron): Windows uninstaller ON for the next R build:** `MSCC.aip` `Uninstall=1`, `ShowAddRemove=1`
  (was 0 / 0: R11-12-x had no Uninstall.exe and no Apps & features entry). Test at the next build: entry appears,
  Uninstall removes C:\mscc-net9 files + shortcuts; check what it does with user settings in %LOCALAPPDATA%\MSCC-NET9.
  **Built R11-12-5 (uninstaller only, payload = R11-12-4) and TESTED by Ron 2026-11-12:** MSCC-NET9 in Apps & features,
  install / uninstall / install OK; %LOCALAPPDATA%\MSCC-NET9 settings kept. Apps & features version shows the client's
  file version (11.11.4+b10624a...), not 11.12.5. Shipped as Release R11-12-5 (Ron: "finished"; Pi stays mscc 1.0.85).
  **Release R11-12-6 (2026-11-12, finished):** Windows Black Pill flasher says "No Black Pill in flash mode found..." on
  List / Burn and hides the dfu-util banner (Ron tested both). Payload = R11-12-5 + that exe/dll. Pi stays 1.0.85.
  **Next build (Ron 2026-11-12): `MSCC.aip` UpgradeMode=0 -> 2** ("If previous version is installed": Uninstall -> Update;
  0 asked "uninstall MSCC-NET9?" on R11-12-6). 2 = Update by the help's option order (Uninstall, Uninstall Silently, Update, ...),
  WRONG: UpgradeMode is the "Update Mode" switch (update-only program); R11-12-8 refused a fresh install after uninstall ("MSCC-NET9 is not installed... update program cannot continue"). Fixed for R11-12-9: UpgradeMode=0, IfInstalled=2 (= "If previous version is installed: Update"; option order Uninstall, Uninstall Silently, Update, ...). R11-12-9 TESTED by Ron 2026-11-12: fresh install (settings page shows), install again (no question), uninstall (files gone, settings kept): all pass. Note: Update mode leaves firmware files a new release no longer ships (older uninstalls left 12 old versioned firmware files; Ron deleted C:\mscc-net9 by hand). Shipped as Release R11-12-9 (Ron: "finished"); R11-12-7/8 were test builds (not published, moved to installers/windows/old).
  Also next build (Ron): ReadMe page in the setup wizard (`DialogReadme=1`, `Readme=` -> `mscc-ui/installer/windows/settings-note.rtf` (RTF since R11-12-8: title bold 16 pt, text 12 pt),
  Ron's wording: "Your settings are kept" + install/update/uninstall does not touch them + stored in %LOCALAPPDATA%/MSCC-NET9).
  Same day, Ron's PC only: dead registry entries from old installs deleted (Proficio, MSCC 020.03.04.1, MSCC-PW
  023.02.17.01, HKLM\SOFTWARE\WOW6432Node\Multus SDR LLC); .reg backups in C:\Users\Ron\Documents\registry-backups. Dead MSCC Start-menu entries (old MSCC, MSCC-MKII, -GEMINUS, -Proficio, -PW, Multus SDR LLC) moved to C:\Users\Ron\Documents\startmenu-backup; only MSCC-NET9 left (Ron: clean).
- **2026-11-12 (finished, Ron): Release R11-12-3** = Windows `mscc-net9-R11-12-3-install.exe` + Pi `mscc_1.0.84_arm64.deb`:
  the five PSoC keyer-update builds above (untested) + keyer-flash message listing them. R11-12-2 (same day,
  published before "finished" by mistake; Ron: leave it) = keyer GUI .exe without flashing console windows.
  1.0.84 = 1.0.83 + those PSoC files + keyer-flash.py / -gui.py (compared file by file). Not installed on the Pi.
- **2026-11-12 (finished, Ron): Release R11-12-1 = keyer update over USB.** Daughter-board PIC keyer gets an
  I2C bootloader (0x5C; `keyer-buffer/bootloader/`); pill 3.16 passes USB opcode 0x9F (CMD_KEYER_BOOT,
  marked taken in ms-sdr `usbavrcmd.h`; ms-sdr never sends it) to the PIC; `keyer-flash` / `keyer-flash.gui`
  (`firmware-programmers/keyer-flash/`, Windows .exe in `firmware-programmers/windows/keyer-flash/`) refuse
  while the servers run. Tested on the rig: Pi command line and Windows GUI, 179 rows, settings kept.
  Installers: Windows `mscc-net9-R11-12-1-install.exe` (`Keyer Firmware\` + Start-menu "Keyer Firmware
  Update", Black Pill 3.16; client / servers unchanged), Pi `mscc_1.0.83_arm64.deb` (= 1.0.82 + keyer tools,
  menu entry, `/usr/share/mscc/firmware/keyer/`, Black Pill 3.16; compared file by file with 1.0.82).
  1.0.83 not yet installed on the Pi (the tools were put there by hand first). PSoC radios: same pass-through
  PARKED (Ron: "No step 3 for now"). Pi tools never go in ~/utils (Ron's own folder).
- **Docs: firmware upgrade note (2026-11-11, Ron).** docs/manuals drafts: Windows 2.6 + Linux 2.7 got a bold
  "Firmware update required" paragraph (cal store in the radio, upgrade for the transceiver you have, old firmware =
  "RADIO FIRMWARE UPDATE REQUIRED" + cal only on the PC); firmware tables updated to the shipped versions (3.243,
  4.244, 5.153, 7.244, 8.243; were 3.242 / 4.243 / 5.120 / 7.243 / 8.242) + a Black Pill 3.15 row; Windows file
  example now versioned + Black Pill folder. Remote guide 1.3 and Configurations rule 5 got short versions; the
  announcement's GETTING IT got an IMPORTANT paragraph. Release-0 PDFs, DRAFT boxes and the Linux file column
  (old mscc-firmware 1.0.2 names; Ubuntu waits on Stew) left alone. Backups in the session scratchpad only.
  **Later the same day (Ron):** Windows + Linux guides got the notice on its OWN PAGE (page 2, between the title
  page and About this guide): "IMPORTANT / The radio's firmware must be upgraded" (24 / 20 pt bold red), body
  16 pt bold, clickable link to "3. Updating the radio firmware" (bookmark _UpdatingFirmware) + 2.6 (Windows) /
  2.7 (Linux). Contents page numbers are one page off until Word updates the TOC field: settings.xml has
  updateFields, so Word asks on open; answer Yes and save. Not checked in Word yet (only LibreOffice text check).
  Windows chapter 3 got "You don't need Zadig for this": the PSoC bootloader is a USB HID device (04B4:B71D,
  Windows driver built in); Zadig/WinUSB is only for normal use (16C0:05DC); don't run Zadig while the LED
  blinks LOADER. Still stale in Linux 2.7: firmware "from the mscc-firmware package" (Pi now = mscc).
- **PSoC firmware backup / fallback (2026-11-11).** The USB bootloader has NO read command (only erase / program /
  row checksum), so PSoC firmware can't be dumped like the Black Pill; MiniProg3 + PSoC Programmer can read the chip
  (our builds: protection off). Per-radio data that matters = EEPROM (cal, PPM, settings); an upload never touches
  it. Idea, not done: an EEPROM read vendor command + MSCC Back up / Restore (needs new firmware on every radio).
  Pi programmer (firmware-programmers/rpi/psoc-usb-bootload, CLI + Tk GUI, in /usr/local/bin on proficio) reviewed:
  works; no check that the file fits the radio (all radios = same PSoC; row checksums could identify the installed
  firmware and warn), stale "install mscc-firmware" / YYYYMMDD wording, unused Security Key field. Not changed.
  **Stock firmware fallback = Stew (Ron):** the pre-change firmware for every radio isn't in our files; note
  `.mscc-coord/NOTE-FOR-STEW-STOCK-FIRMWARE-FALLBACK-2026-11-11.md` (now: radio-psoc-firmware/original/<Radio>/).
  **Found 2026-11-11** in old, unused folders of `C:\mscc-net9\PSoC Firmware` and COPIED unchanged to
  `radio-psoc-firmware/original/` (Ron's name; README there with source, date, MD5): `Proficio/Firmware-122-01`
  (2026-08-02, never in git, older than the repo, looks like Proficio 1.22, model unknown),
  `Proficio-MKII-ATU/Proficio-MKII-ATU` (from "Profico MKII", 2026-07-22, never in git), `Geminus-MKII/Geminus`
  (from "Geminus MKII", 2026-07-22 = the build in Stew's original Geminus MKII tree, commit 000b1fc). NOT confirmed;
  Stew asked in the note. **`radio-psoc-firmware/original/` is in .gitignore** (public repo; may not be ours to
  publish) — local only on Ron's PC until Stew confirms. Stew can't see them in git, so Ron SENDS them to him:
  `radio-psoc-firmware/original-firmware-for-Stew-2026-11-11.zip` (185 KB, the folder + README; also
  git-ignored); the Stew note says so. Zip SENT to Stew 2026-11-11.
  **Policy (Ron, 2026-11-11):** the original firmware is open source (public repo is fine), but it NEVER goes in
  the installers; it may go in git later for safe keeping (not decided). A user firmware problem: analyze and fix
  the new firmware first; the original is the last resort. Stew note + original/README updated to say so (the
  README copy in the zip Stew has is the older wording). Still missing: one Proficio (Legacy or MKII PTT), Geminus
  Legacy, all three Ultimus.
  **C:\mscc-net9 fresh install (Ron, 2026-11-11):** the installer only adds files, so PSoC Firmware had piled up old
  dated / unversioned copies, .gitkeep files and old folders. Ron deleted C:\mscc-net9 and reinstalled R11-11-5
  (settings live in %LOCALAPPDATA%\MSCC-NET9, untouched). Checked: PSoC Firmware = only the 8 current versions,
  Black Pill = the 3.15 versioned files. Idea, not done: installer removes old firmware files; folder names
  mixed case (Proficio-legacy / Geminus-legacy vs Ultimus-Legacy), cosmetic.
- **2026-11-11 (finished, Ron): Release R11-11-5** = Black Pill versioned names + docs firmware note.
  `mscc-net9-R11-11-5-install.exe` (AppVersion 11.11.5; WPF client unchanged, title bar 11.11.4; servers
  unchanged) + `mscc_1.0.81_arm64.deb` (same servers, Black Pill files renamed) + mscc-avalonia 1.6.96 (unchanged).
  psoc-replacement-stm32 committed + pushed separately. 1.0.81 install over ssh HUNG (2nd time): postinst
  "Stop the MSCC servers? Continue [y/N]" waits on the pty apt opens; finished with
  `DEBIAN_FRONTEND=noninteractive dpkg --configure mscc`.
- **2026-11-11 (finished, Ron): mscc 1.0.82 = the install no longer hangs.** postinst `confirm_stop_servers` asks only
  when ms-sdr / sdrcore-recv / sdrcore-trans is running (else "MSCC servers not running: nothing to stop"), and the
  question times out after 2 min (= N, install aborted). Same servers. INSTALLED on the Pi 2026-11-11 with plain
  `sudo apt install -y` over ssh (finished at once; the 2-min timeout not tested, servers weren't running). Replaces
  1.0.81 in GitHub Release R11-11-5 (Windows installer unchanged). Over ssh still use
  `sudo -n env DEBIAN_FRONTEND=noninteractive apt install -y ...`.
- **Black Pill firmware now versioned like the PSoC (2026-11-11, Ron; released in R11-11-5).** Names:
  `BlackPill-25MHz-<major>.<minor>-app.bin` + `...-boot-app.bin` (were app-firmware-async.bin /
  boot-app-firmware-async.bin); `bootload-PA8.bin` keeps its name. Version = FIRMWARE_VERSION_MAJOR/MINOR in
  include/proficio_config.h. psoc-replacement-stm32 (SEPARATE repo, w4mmp-ron/psoc-replacement-stm32, needs its own
  commit): `extra_tinyusb.py` writes the versioned files into release/; a rebuild with different bytes of the same
  version is saved as `...-<ver>-b2-...` + a "bump the minor" warning (like PSoC copy-release.py; the build time is
  in the image, so every rebuild differs: bump the minor for every build you flash or ship). release/ 3.15 files
  renamed (built 11-10 15:44 = the 3.15 change), release/README, LED-CODES.md, platformio.ini comment updated.
  mscc-station: stage.ps1 ships the newest versioned set (never -bN) and clears old .bin from the payload (ran OK,
  payload = 3.15 files); rpi/mscc-deb/build-deb.sh takes exactly one versioned set (tested, path with a space);
  stm32-bootloader-flash.ps1 `-Image combined` finds the newest -boot-app.bin (tested); Pi stm32-flash scripts,
  READMEs, installers/rpi/INSTALL.md, INSTALL-FOR-PI (.md/.docx/.pdf) updated. Not yet run through a real `pio run`
  (rebuilding 3.15 now would give -b2; first real test = next version bump). Pi keeps the old names in
  /usr/share/mscc/firmware/black-pill until the next mscc .deb (bump to 1.0.81); Windows installer needs a new
  R-build too. Left alone (not mine): 8 MHz build_info.h, .vscode/settings.json in that repo.
- **Firmware check (2026-11-11, Ron): MKII-style CW + QRP / I/Q cal store, from the source.** MKII-style CW =
  key-delay buffer (non-QSK amp delay), key-click shaping, paddles. Cal store = 0xC8 CMD_CALIBRATION with
  CAL_TYPE_POWER / RX_IQ / TX_IQ.
  - PSoC, all YES on both: Proficio MKII PTT 3.243 / Ultimus 8.243 (the MKII code); MKII ATU 4.244 / Ultimus 7.244
    (cw.c identical to PTT); Proficio Legacy 1.236 / Ultimus 6.236 (keys by gating DIN = TX I/Q, not the PA line);
    Geminus MKII 2.153 and Geminus Legacy 5.153 (relays latched for the semi-break-in session, no QSK). Shaping on
    PSoC = PCM3060 DAC soft mute. = the client's minimum-firmware list.
  - Black Pill: F411 25 MHz 3.15 (shipped) YES / YES, shaping = raised-cosine ramp on the TX I/Q (also TUN / SSB /
    digital), cal store also holds PPM. F411 8 MHz 3.14 (not shipped) YES / YES. STM32H743 3.232: paddles + QSK
    only (no key delay, no shaping) and NO cal store (client would ask for a firmware update); not shipped.
  - Empty leftovers in radio-psoc-firmware: `Proficio-Legacy`, `Proficio-MKII-PTT`, `Proficio-MKII-ATU`;
    `Proficio-Legacy - Copy` (1.236) is an old copy, the current Legacy is `Proficio-Ultimus-Legacy`.
- **proficio power (2026-11-11, Ron's custom buck board in the MII):** the Pi 5 said the supply can't give
  5 A. Cause: a Pi 5 asks the supply over USB-C Power Delivery for 5 A; a plain buck can't answer, so the Pi assumes
  3 A and limits USB to 600 mA. Supply is fine (EXT5V 5.07-5.09 V, get_throttled=0x0). Fix: EEPROM
  `PSU_MAX_CURRENT=5000` (rpi-eeprom-config --apply, rebooted 17:28): usb_max_current_enable=1 (USB 1.6 A), no
  warning in the log. Old EEPROM: BOOT_UART=1, BOOT_ORDER=0xf461, NET_INSTALL_AT_POWER_ON=1. The MSCC servers
  do not start by themselves after a reboot (MSCC Start). Undo: remove the line, reboot.
- **proficio backup card (2026-11-11, same as squireoaks):** USB-adapter card `sda` (29.2 GB). The old
  copy had the SD's disk ID 21382c14 (boot mix-up risk); Ron re-ran SD Card Copier with New Partition
  UUIDs -> copy ID e6308e0a (cmdline.txt + fstab match). rpi-clone 2.0.27 (geerlingguy fork,
  /usr/local/sbin, not on ron's PATH; Ron installed it). First `sudo /usr/local/sbin/rpi-clone -u sda`
  32 s, exit 0. Root cron: `0 3 * * * /usr/local/sbin/rpi-clone -u sda > /var/log/rpi-clone.log 2>&1`.
  Check after 03:00: log ends "Done with clone to /dev/sda". squireoaks: reach it only through
  proficio (`ssh proficio 'ssh ron@squireoaks ...'`); its clone ran OK 11-11 03:00 (49 s).
- **Client power meter in watts (Ron, TESTED on WPF 11.11.2 at ~120 W, "the fix is fine").** Was
  x1 / x10 / x100 (10 / 100 / 1000 W full scale): 110 W jumped to x100 = needle at 1.1 of 10. Now
  full scale 10 / 20 / 50 / 100 / 200 / 500 / 1000 W, tick labels in watts (200 W = 0 40 80 120
  160 200), centre legend "W", up at 95 % of full scale, down below 30 % of the next lower one
  (as before). `AnalogSMeterControl.xaml.cs` (WPF) and `.axaml.cs` (Avalonia 1.6.95, built, not
  tried). New WPF + Windows ms-sdr 3.208 copied to `C:\mscc-net9` (old ms-sdr in
  `C:\mscc-net9-old\ms-sdr-MKII-3.206-backup.exe`).
- **WiFi SWR meter vs client (checked, fine):** tcpdump of UDP 6999 on the Pi during a WSJT-X PTT
  tune: packets every 0.1-0.2 s, `fwd` 120.8 easing to ~117.5 W, `peak` 122.2, `avg` 111-114,
  swr 1.32. ms-sdr passes `fwd` to the client (client ~120 W = right). ms-sdr logs only the FIRST
  meter packet after start (SD wear) plus fault edges; option (not done): one "TX end: peak fwd /
  ref / swr" line per transmit. `tune` = 0 on CAT PTT (flag only on CAT AC111; or client TUN).

- **DONE 2026-11-11 (see above). Was: WSJT-X "right-click for rig tune" does nothing.** WSJT-X on
  the Pi (rig TS-2000) asks hamlib for the rig's tuner, which should send Kenwood `AC111;` (not yet
  seen on the serial line). ms-sdr `comm-port.c` only handles AI ID IF MD PS RM FW KS FA FB RX TX,
  so AC is ignored. Fix: handle `AC` in comm-port.c and start MSCC's own TUN carrier
  (`tune_modulate`); first capture what WSJT-X really sends, and decide how TUN ends (AC has no
  stop: timeout, or WSJT-X Tune released). Left-click Tune (CAT PTT + tone) works and stays as is.
  **What WSJT-X 3.0.2 can send (Pi uses hamlib 4.6.2; from hamlib source `rigs/kenwood/ts2000.c`
  + `kenwood.c` tag 4.6.2, not a capture; actual set depends on WSJT-X settings):**

  | WSJT-X action | hamlib sends | ms-sdr handles? |
  |---|---|---|
  | Connect | `ID;` (wants `ID019;`), `AI;`, `AI0;` if AI on, `IF;` | yes (`AI0;` no reply, fine) |
  | Read freq | `FA;` (`FB;` for VFO B / split TX) | yes |
  | Set freq | `FA;`, then `FA00014074000;`, then `ID;` | yes |
  | Read mode | `MD;` | yes |
  | Set mode | `MD;`, then `MDn;` (1 LSB 2 USB 3 CW 4 FM 5 AM 6 FSK 7 CW-R) | yes for 1-5 |
  | Read PTT / VFO / split | `IF;` (cached 0.5 s) | yes |
  | PTT on | `VX0;TX;` | yes (finds `TX;`, `VX0;` ignored) |
  | PTT off | `RX;` | yes |
  | Set VFO | `SA;`, then `FR0;` / `FR1;` | **no** |
  | Split (Split = Rig only) | `IF;`, then `FR0;FT1;` / `FR0;FT0;` | **no** |
  | Power status | `PS;` | yes |
  | Right-click Tune | `AC111;` (RIG_OP_TUNE) | yes since 2026-11-11 (10-11 s TUN) |

  Every set command except `RX` is followed by `ID;` (hamlib's check). Gaps: SA, FR, FT.
- **Pi runs:** mscc-avalonia 1.6.89 (Ron's Avalonia copy, below), recv 3.152 (automatic NB + spectral NR), trans 3.151 (2026-11-05: operator mic
  fix, see below), ms-sdr 3.195 (Pill_stats log, below), pill 3.14 (2026-11-10, 0x9D passed through),
  PIC keyer = keyer-buffer (buffered Farnsworth), package mscc 1.0.75 installed (Ron 2026-11-05).
  **mscc 1.0.76 built 2026-11-09** (`installers/rpi/mscc_1.0.76_arm64.deb`, also V:\ = Pi home)
  = 1.0.75 + ms-sdr 3.195 (Pill_stats; md5 = the running ~/mscc/ms-sdr). 125 files, same modes, no
  CR in text files. Not installed from the .deb yet. Build: scratchpad build-mscc-deb.sh (WSL Debian,
  stages under /tmp/mscc-src, strips CR from text only, runs build-deb.sh; session-only).
  INSTALL-FOR-PI.md now names 1.0.76 (the .docx / .pdf copies not regenerated).
  Pushed 2026-11-05: keyer + 0x9D work (fb90f87), mscc 1.0.75 (f78d141), pill 3.13 (a2e351d in
  psoc-replacement-stm32). Open from today: SSB on a dummy load to confirm the mic fix;
  Stew's PADDLE checkbox (note pushed). (PIC reset kink fixed 11-10, pill 3.14.)
- **2026-11-05 mscc 1.0.74 / 1.0.75 (`installers/rpi/mscc_1.0.75_arm64.deb`):**
  - **/dev/tnt* missing after a kernel update:** the Pi booted kernel 6.18.60 (installed 11-03);
    tty0tty was only built for the older kernels. 1.0.74 adds kernel hooks
    (`/etc/kernel/postinst.d/mscc-tty0tty`, `/etc/kernel/header_postinst.d/mscc-tty0tty` ->
    `/usr/share/mscc/bin/mscc-tty0tty-kernel <kver>`, log `/var/log/mscc-tty0tty.log`, never
    fails the kernel install) and postinst builds it for every installed kernel with headers.
    Checked on the Pi: hooks run by run-parts, build with headers, skip without; tnt0-7 back.
    Next kernel update: the log should say "built and installed" for the new version.
  - **Operator mic bug (sdrcore-trans, found 11-05):** `score_operator_input()` tied the headset
    speaker monitor (`alsa_output.usb-C-Media...analog-stereo.monitor`) with the real mic
    (`alsa_input...mono-fallback`), both 6200, monitor first -> trans used the monitor: phone TX
    would send RX audio. Fix: never a `.monitor` for the operator mic (input-devices.c). Built on
    the Pi (trans 3.151), log now shows the mono-fallback mic. **Not yet checked on the air**
    (SSB on a dummy load). Windows trans (Stew's) not checked.
  - **`mscc status` false FAIL "Operator speaker: no PortAudio device matches"** in Phones mode:
    the report wanted the exact ALSA name; with recv playing through Pulse the raw hw:3,0 shows
    0 outputs. 1.0.75 report scores operator devices like recv / trans (folded name, Pulse
    preferred, no .monitor mic), so it shows the device the servers open. Passed on the Pi.
  - Package build: WSL wrapper stages under /tmp and strips CR from text only (scratchpad
    `build-mscc-deb.sh`, session-only). 51 world-writable data files in the package (same in
    1.0.73), left as is.
- **Keyer: buffered Farnsworth on the paddles — DONE and in the rig (2026-11-05).** Details,
  design and build / flash steps: `keyer-buffer/RESUME.md` (top section). Ron: "all the work",
  "the speed sounds spot on". You key at your own speed and hear yourself; letters go into a
  buffer and go on the air with Farnsworth gaps (letters at SET_WPM, gaps at the 0x76 text WPM);
  20 letters waiting -> STP on the sidetone, paddles off, drain, RES. Switch = new opcode
  **0x9D SET_PADDLE_FARNSWORTH** (0/1): Pi `cw.ini` `CW_Paddle_Farnsworth=` (ms-sdr 3.192),
  pill 3.13 passes it to the PIC, client never sees it (no client change). Ron's cw.ini now:
  CW_Speed 18, CW_Mem_Text_WPM 15, CW_Paddle_Farnsworth 1 (backup cw.ini.bak-20261105).
  How we got there: bit 7 of 0x76 was dropped (the WPF client clamps the 0x76 value to 60 and
  sends it back at connect); the first try, stretching LETTERSPACE in the old keyer
  (`worktrees/keyer`, d40a7ba), worked but felt "wonky" (holds your next letter back). The new
  keyer (`worktrees/keyer-buffer`) rebuilt the PIC timing on a 1 ms TMR2 tick, state machines,
  real-ms element times (the old loop timing was off; 18 now = true 18 wpm).
  **PIC reset kink FIXED 2026-11-10 (pill 3.14):** the pill now passes every 0x9D on (was: only
  changes), so after a PIC reset one mscc restart puts Farnsworth back. Tested (Ron).
  **2026-11-10 keyer streamed** (keyer-buffer 79831ce, in the rig): a held paddle starts at once
  (was ~2 s wait, 16-element chunks). Pi cw.ini = `~/.local/mscc/cw.ini`.
  Not done: Windows ms-sdr (Stew's) and PSoC have no 0x9D; ms-sdr only logs a rejected 0x9D.
  Idea, not started: keyboard CW (client text into the player).
- **AM receive spectrum is 140 Hz low (Ron 2026-11-03). EDITED 2026-11-14 (Ron OK'd): `AM_OFFSET_RX` -12000 in Pi, Windows AND Debian ms-sdr `extern.h` (all three recv trees now have the AM lowpass, so the "not linux/" caveat below is old); Pi file copied to V:. Not built / tested yet; Makefile doesn't track headers, so main.c must rebuild.** On WWV
  20 MHz in AM the carrier sits ~6 px left of the spectrum center line at 4x zoom (measured
  157 +-15 Hz from a PNG; USB, LSB and CW are on center). Cause: ms-sdr sets the AM receive LO
  with `AM_OFFSET_RX (-11860)`; every other mode, and AM transmit, uses -12000, and sdrcore-recv
  always mixes down by exactly 12000 Hz. Likely reason Ron set 11860 years ago (he doesn't
  remember; this is read from the code): the old AM filter was the SSB bandpass (Lo..Hi), which
  blocks 0 Hz, so the carrier had to sit 140 Hz off. The sdrcore-recv 3.147 AM fix (c9850ca)
  made AM a lowpass (0..Hi), so 11860 is no longer needed. Fix: one line,
  `AM_OFFSET_RX (-11860)` -> `(-12000)`, in `rpi/ms-sdr-linux/source/extern.h:58` (Pi) and
  `mscc-ui/windows-work-tree/ms-sdr-MKII/source/extern.h:47` (Windows ms-sdr; tell Stew). Don't
  change it in the old trees (`linux/`, `Solidus/servers/`): their recv still uses the bandpass
  for AM and would lose the carrier. Leave the PSoC `AM_OFFSET 11860` (`si5351.c`) alone unless
  the PSoC turns out to set the LO in AM. Test: WWV in AM, carrier on the center line, audio
  sounds the same.
- **Pi ms-sdr 3.194 (2026-11-06): radio calibration read retried at client connect, BUILT on the Pi
  18:10 (~/mscc/ms-sdr), committed and pushed. Checked in the log 2026-11-08: PASSED** (one read at
  startup 04:59: power 10 bands same, RX IQ 12 + TX IQ 10 from radio, no errors; the two later
  client connects sent status only, no second read). New `Radio_cal_startup_load(who)` in
  power_calibration.c (mutex + once-per-run flag, needs G_network_initialized): reload replaced
  cal files, Radio_cal_load_power, Radio_cal_load_iq. Called from main.c at startup (replaces
  the three calls) and in main-controller.c at client connect before Apply_Appliance_Startup.
  Log line: `Radio_cal_startup_load . startup|client connect: reading the radio's calibration`.
- **Pi ms-sdr 3.195 (2026-11-09, Ron: the one exception to the hold): pill counters in the
  ms-sdr log. BUILT on the Pi 13:03, WORKS (Ron's mscc.ini has `PILL_STATS=1;`).** mscc.ini
  `PILL_STATS=1` (default off) -> the temperature thread (every 3 s, STM32 radio only) reads radio
  command 0xC9 (`RADIO_CMD_AUDIO_STATS`, read only, value 0; client 0xC9 = auto snap, no
  conflict) and logs `Pill_stats. 10.0 min normal running, pill up .. h: in .. empty .. gaps ..
  (max .. ms) in_drop .. out_under .. out_drop .. ep0_cut ..` (`<<<` on trouble, PILL RESTARTED
  line). `Audio_stats_busy(why)` restarts the 10 min: SetHWLO_queue when the frequency changes
  (TX on / off re-sends the same one; RIT / split keying does count), ModeChanged (incl. TUNE),
  Session_Claim, Stop_all; new window only after 30 s quiet. Files: proficio-temperature.c,
  extern.h, main.c, main-controller.c (Parse_mscc_record), version.h 195. First lines 13:14 and
  13:24 clean. Ron (2026-11-09): gaps etc. during connect / band change / tune don't matter, only
  normal running. Cron log still on. Docs: utility-scripts/Pi/README.md (audio-stats section).
- **Ron 2026-11-08 22:20: the Black Pill (pill 3.13) is PRODUCTION READY.** Cron log, pill up
  22.5 h: 17:50-22:10 local clean (idle receive, WSJT on); 22:10-22:20 FT8 on 30 m (~10
  transmits) + 2 tunes: gaps 0, in_drop 0, out_under 4 = the 4 trans reopens (T/U mode commands),
  host-side as above. No pill fault counted all day (ep0_cut, abandoned, slow, out_drop, resets,
  hang record all 0). Left: the trans dual-stream no-reopen fix (after the hold). Cron log stays on.
- **Setup 3 (Ron's station), Configurations draft item: ANSWERED 2026-11-03, draft updated (DRAFT box + Appendix A item 2 removed).**
  Read on the Pi: ms-sdr CAT /dev/tnt0, WSJT-X /dev/tnt1 (pair), Rig Kenwood TS-2000, PTT RTS on tnt1;
  MSCC plays VirtualA -> WSJT-X in VirtualA.monitor; WSJT-X out VirtualB -> MSCC VirtualB.monitor; Mono.
  Ron: nothing set in the Windows client for COM ports or audio; mscc-init on the Pi sets them, and the
  Phones devices too. Unknown: VirtualA_TX / VirtualB_TX exist on the Pi, unused by these settings.
- **2026-11-01: Linux guide rewritten: `docs/manuals/MSCC-Operators-Guide-Linux-Release-1-DRAFT.docx`**
  (39 pages, Word, Pi + Ubuntu as one, Pi kit = standard, MSCC UI = Windows client). Built from
  scratchpad build_linux.py (session-only; edit the .docx from now on). Release 0 WSJT-X audio was
  backwards: correct = Input VirtualA.monitor, Output VirtualB (also fixed in Configurations setup 4).
  11 draft items (Appendix C). In the note for Stew (Linux guide section).
- **Remote audio summary for Stew (2026-11-05, pushed 6c8db8a): `mscc-remote-audio/REMOTE-AUDIO-WHAT-WE-KNOW.md`.**
  Plain English (Ron: the first version "was written for a machine"). Why: Stew was being led to
  redo remote audio; Ron: Pi host -> WPF Windows client works fine, same server code on every host.
  Stew mixes up which way round (host = has the radio + servers, client = where you sit); the file
  asks him to name both. Ron's device model (corrected me): VirtualA / VirtualB belong to the
  SERVERS (local Digital, WSJT-X on the same computer), not the remote client. Remote client:
  Remote Phones = real speaker + mic; Remote Digital = client <-> virtual sound device <-> WSJT-X
  (Stew: VB-Audio; Ron: two USB adapters back to back). Linux client: PipeWire null sinks
  `MSCC_Remote_RX` (client plays, WSJT-X input = MSCC_Remote_RX.monitor) and `MSCC_Remote_TX`
  (WSJT-X output, client input = MSCC_Remote_TX.monitor), made at login by Ron's mscc-avalonia
  since 1.6.84 (2026-11-06, see the Avalonia item below). The first proposal (`mscc_remote_rx`
  "MSCC Remote RX") crashed PortAudio: name and description must be the same. Open (Stew's
  mscc-ui, fixed in Ron's copy 1.6.80): Avalonia doesn't save the Remote device choices or
  Phones (reverts to one shared device; maybe the spectrum noise, a guess). Stew asked to add his VB-Audio setup to the file. Still to correct (proposed, not
  approved): the 11-01 user-documents note and the Remote guide say "Pi / Linux remote works
  differently"; the remote-audio part is the same on every host.
- **Ron's Avalonia client copy (2026-11-05/06, pushed 24d8738 = 1.6.79): `worktrees/Avalonia`, Pi
  package `mscc-avalonia` 1.6.89 installed on the Pi (2026-11-07).** Copy of Stew's
  `mscc-ui/Avalonia-Migration` 0.6.75 with its own `src/MSCC.Core` copy. Version 1.x (Ron) vs
  Stew's 0.6.x. **Since 2026-11-13 (Ron):** WPF and this Avalonia are kept at par both ways (Ron
  works in WPF too; see the shared `CLAUDE.md`); Stew's `mscc-ui/Avalonia-Migration` is no longer
  kept up to date and will be deleted. Building WPF copies into `C:\mscc-net9` (CopyToMsccNet9).
  Installs beside Stew's mscc-ui 0.6.31: /opt/mscc-avalonia, /usr/bin/mscc-avalonia, menu
  "MSCC Avalonia". Build `Avalonia/build-mscc-avalonia-deb.ps1` (bump Version in the csproj and
  `packaging/mscc-avalonia/DEBIAN/control`), deb to installers/rpi + Pi ~; publish/ and *.deb not
  committed. Own settings `~/.config/MSCC-Avalonia/mscc-avalonia.ini` (seeded once from the shared
  file); reads only a copy of `~/.local/mscc/*.ini` the launcher makes at each start
  (`~/.config/MSCC-Avalonia/host/`); never writes mscc.ini (on the Pi it is ~/.local/mscc/mscc.ini).
  Still shared with Stew's client: favorites and last-used files (Ron thinking about it).
  Ported from WPF 11.1.5: ALC button removed, 2 TONE, TX IQ PHASE, cuts at connect, QRP save step +
  0x2D lamps, SAVE / APPLY ALL labels, FREQ CAL note + 4x, SMOOTHING + Falling waterfall, SWR meter
  (FWD face, fault RESET, settings), TIME DISPLAY + TIME MARKER, PEAK MARKER, dB LABELS, AUTO SNAP,
  BASELINE / WF GAIN / WF ZERO per band. Also: window size / position remembered, own resize grips
  (the Pi's Openbox runs ~/.config/openbox/rpd-rc.xml = theme only, no side handles).
  Not ported: Reset configuration, identity request, RF power report. **Not tested on the air yet**
  (2 TONE, PHASE, QRP save, SWR meter, smoothing).
  **2026-11-06, Remote Digital on the air (1.6.80 to 1.6.86): Windows host (servers on Ron's PC),
  Avalonia + WSJT-X on the Pi, two FT8 QSOs.** Stew to check the RF (Ron can't see it).
  1.6.80 Remote devices saved by name ("Name|HostApi"), Phones and Digital separately
  (REMOTE_PLAY_NAME, REMOTE_MIC_NAME, REMOTE_DIGI_PLAY_NAME, REMOTE_DIGI_MIC_NAME); before, Digital
  always went back to VirtualA / VirtualB.monitor and Phones kept a PortAudio index that shifts.
  1.6.81 Remote CAT watches the line in comm-port.ini PIN= (1 = CTS = WSJT-X RTS, 2 = DCD = DTR)
  every 10 ms and keys like CAT TX; (ms-sdr Pin_Check_Thread does the same locally). WSJT-X PTT =
  RTS or CAT both work. Linux raises RTS when a port opens: watch for a short key at WSJT-X start.
  1.6.82 Digital default mic = MSCC_Remote_TX.monitor (PortAudio lists the internal name, not
  "Monitor of ..."); Remote window 520x720 with the log always visible; closing the main window
  closes the Remote window and exits (ShutdownMode OnMainWindowClose).
  1.6.83 / 1.6.84 installer: user service `mscc-avalonia-remote-audio.service` (systemctl --global
  enable) runs /usr/share/mscc-avalonia/bin/mscc-avalonia-remote-audio.sh (pactl null sinks,
  name = description, 48 kHz, leaves existing ones alone). 1.6.83 also put ALSA names in
  /etc/alsa/conf.d; the Pi has no pipewire-alsa, so they were broken and a same-named ALSA
  MSCC_Remote_TX hid the real one from WSJT-X. Removed in 1.6.84 (postinst deletes the file).
  1.6.85 Remote Phones / Digital choice saved (REMOTE_DIGITAL). 1.6.86 title bar version from the
  build (was a hard-coded 0.6.75). 1.6.87 (2026-11-07, works, Ron): keep-alive lost pop-up as WPF
  ("MSCC - Server Not Responding", Yes = wait again, No = close MSCC, X = Yes); before, only status text. 1.6.88 (2026-11-07, Ron: "Much better"): spectrum draws every frame
  (SpectrumRefreshDivisor 3 -> 1); at 3200 bins ~12 frames/s arrive (99 pkts/s, 8 per frame), it drew ~4/s. 1.6.89: spectrum banner
  ("Center ... MHz | bins | zoom | click to tune", from Stew's Avalonia, not in WPF) removed: covered the pass band.
  Learned: Avalonia's Remote Digital needs PTT from WSJT-X (CAT or RTS); there is no VOX. WSJT-X on
  the Pi (Qt5, PulseAudio plugin): Input MSCC_Remote_RX.monitor, Output MSCC_Remote_TX, Rig
  TS-2000, Serial /dev/ttyUSB11 (= tnt1; Avalonia holds tnt0). The pair comes back by itself
  after a Pi reboot (Ron checked). Windows ms-sdr stops all servers ~45 s after the
  client leaves (Flusher_thread "Client FAILED" falls through to Stop_all; the Pi sets
  servers_failed = 0). Start the Windows servers again before each connect. Note for Stew (list item 11):
  `.mscc-coord/NOTE-FOR-STEW-WINDOWS-MSSDR-CLIENT-GONE-2026-11-06.md`. Pi ms-sdr PIN=2
  (DCD) never keys: it checks fDCD, which is never set (only fRLSD is read).
- **2026-11-01: Pi ms-sdr 3.187: WiFi SWR meter fault to the client, BUILT on the Pi 15:35 (~/mscc/ms-sdr),
  NOT committed, running only after mscc stop/start.** Meter is UDP unicast: at a Pi station it talks
  to ms-sdr only, which relayed SWR/FWD/REF but never the fault (client got Fault=false; the 0x30
  "SWR METER FAULT" text is dropped by the client). New extended sub 0x0F CMD_SET_SWR_FAULT:
  [fault][online][meter IPv4 4 bytes], sent at once on a fault change, with each update (<= 2/s),
  and offline; not gated by SWR_METER_TO_GUI. Files: swr_wifi_meter.c, usbavrcmd.h, version.h (187,
  synced back from the Pi). Client side = Stew: `.mscc-coord/NOTE-FOR-STEW-SWR-FAULT-TO-CLIENT-2026-11-01.md`.
  Windows ms-sdr has no meter code (client listens directly there).
- **2026-11-01 17:00: Pi ms-sdr 3.188 + trans 3.150 BUILT and RUNNING on the Pi (Ron restarted, old client 10.31.x, no calibration until 11.1.0;
  committed): port of Stew's Windows round 1 (ms-sdr 3.189 / trans 3.154 / client 11.1.0, 49d8c90).**
  0xA2 slider = live only; 0xA2 | 0x8000 (POWER_CAL_SAVE_FLAG) = save: ms-sdr writes the radio, trans
  writes power_cal.ini (trans reads the flag from buf[2] bit 7; value stays buf[1]). Startup copy from
  the radio to trans sent WITH the flag. New 0x2D CMD_SET_CAL_STATUS ms-sdr -> client: int32 kind (1 RX
  IQ, 2 TX IQ, 3 power) | flags<<8 (bit0 store) | mask<<16; sent at Session_Claim, end of
  Radio_cal_load_iq, after each radio write (masks reread). Includes the 3.187 SWR fault (0x0F).
  **Needs client 11.1.0** (old client never sends the save flag = QRP cal not saved). New trans with an
  old ms-sdr never saves power_cal.ini (Stew's trans comment says otherwise; code saves on flag only).
  Files: ms-sdr power_calibration.c, main-controller.c, usbavrcmd.h, extern.h, swr_wifi_meter.c,
  version.h (188); trans udp_thread.c, extern.h (150).
- **2026-11-01 19:41: Pi trans 3.151 (2 TONE, port of Stew's Windows 3.155 on 0x88) + ms-sdr 3.189 (0x2E
  CMD_SWR_METER_RESET: client RESET -> HTTP reset to the meter in a detached thread) BUILT on the Pi, NOT
  running until mscc stop/start, NOT committed.** Stew pulled 11-01: client 11.1.5 (0x0F handled, installer
  R11-1-5), Windows trans 3.157 (2 TONE + voice ALC 0.89; 0.45 was a bad StarTech USB card). Note:
  `.mscc-coord/NOTE-FOR-STEW-2TONE-PI-AND-0x2E-2026-11-01.md`. Running since 19:46. **Fault test PASSED 19:51
  (Ron, client 11.1.5):** meter fault latched -> client showed fault + RESET, Ron pressed the RESET
  button ON THE METER ITSELF (corrected 2026-11-03; this line used to say the client's RESET) -> meter
  cleared, ms-sdr relayed it. The client's own RESET was first tested 2026-11-03 (client HTTP straight
  to the meter, after fixing a typo in MSCC_Client.ini SWR_HF_IP=192.156.12.222 -> 192.168.12.222): worked.
  Not tested yet: client RESET from a remote client (Tailscale) - the client sends the reset straight
  to the meter's LAN IP, and doesn't use ms-sdr's 0x2E reset yet. **2 TONE PASSED 20:12 (Ron, 20 m USB, full power; button is on the RX/TX tab;
  panadapter stops in TX so tones/IMD not seen).** Note: `NOTE-FOR-STEW-2TONE-PI-TESTED-2026-11-01.md`.
  Open: 0x2E RESET path (needs Stew's client send).
**2026-10-31 (16:39-16:43 local): LSB filter fix CONFIRMED on Ron's rig (client 10.31.1, Pi recv 3.150).** recv log: in LSB, USB and AM every LO CUT press arrived (75 / 100 / 200 / 300 / 500) and every HI CUT press (5500 / 2400 / 2700 / 3000 / 4000), each cut moving alone. Ron: LSB and USB on an AM station now sound the same. LSB / USB recheck CLOSED. Open: Ron's I/Q test (TX I/Q from CW, PHASE slider, APPLY; server code ms-sdr 3.185 / trans 3.147 / recv 3.150 not committed until then); SSB voice / ALC meter test.
**2026-10-31:** Ron will do the TX I/Q test later (needs setting up). Test: (1) CW mode, start TX I/Q calibration on a band: it must transmit; (2) move PHASE: phase changes; (3) APPLY; (4) Claude checks ms-sdr.log for `Radio_cal_store_iq ... queued` then `Radio_cal_send_iq ... sent`. Then commit ms-sdr 3.185 / trans 3.147 / recv 3.150.
**2026-10-31 (12:56 local): Pi ms-sdr 3.186 = power start order as Windows 3.185 (built, running, NOT committed, goes with the I/Q-in-radio port after Ron's TX I/Q test).** `Radio_cal_load_power`: bands the radio does not hold now KEEP the power_cal.ini value (before: factory value, which overwrote a calibrated file on the first start with a radio holding nothing). Radio values still win where held. `Factory_power_cal_values()` no longer called (left in factory_seed.c, as Stew did). Log on Ron's rig: 160-10 m `radio N, file N, same`, 630 / 2200 m `not in radio, file 0 kept`, 0 changed; I/Q 22 bands from radio; no errors. Stew's open item #3 (power start order) DONE on the Pi. Pi now: recv 3.150, trans 3.147, ms-sdr 3.186, pill 3.12.
**2026-10-31 (13:00-13:10 local): recv NR (noise reduction) found defective; Ron: replace it with a spectral-subtraction NR (option B), PARKED for now.** Ron: "the audio becomes really distorted" with NR on. Code: `denoise()` in `SDRcore-recv-linux/sources/dsputils.c` = LMS adaptive line enhancer at 96 kHz after the AGC (48 taps, 32-sample delay, mu 0.002, wet 0.35-1.0 by level, step normalised by ONE sample's power averaged over ~12 samples, no leakage, no output limit). Measured: Ron listens on Remote Phones (UDP to the PC, port 9100, 48 kHz mono s16 after a 16-byte "MSA1" header; local C-Media output is muted then, so recording its monitor gives silence). Captured with `sudo tcpdump -i any -w X.pcap "udp dst port 9100"` and decoded (scratchpad pcap2wav.py). Fair A/B on one rag chew, NR toggled during a 21 s capture (recv log times are UTC): with NR ON (level 50) a ~3.8 kHz whistle at the top of the 4 kHz filter (strongest component every second), >3 kHz energy from about -15 dB to -4 dB of total, peaks 0.33-0.37 vs 0.20 off (0.366 repeated = clipping), speech band 300-1000 Hz down up to 10 dB. Cause: the ALE boosts anything correlated over the delay; noise through recv's sharp audio filter is strongly correlated at the filter edge -> whistle; per-sample normalisation over-corrects at quiet moments -> overshoot / spikes. A Python copy of the algorithm on an upsampled NR-off capture did NOT reproduce it (the earlier NR-on / NR-off captures were at different moments; only the same-moment A/B shows it). Options: A = repair (NLMS over all taps, leakage, output never above input), B = replace with frequency-domain spectral subtraction (as most SDRs). **Ron chose B, parked.** When done: same change in the local copy (sync rule), Stew's Windows recv likely has the same NR, retest with the same A/B capture.
**2026-10-31 (13:15 local): recv NB (noise blanker) checked: WORKS, makes no ticks of its own.** Same Remote Phones A/B capture (21 s, NB toggled; switch times from recv log `CMD_GET_SET_NB_ENABLE`), ticks = samples far outside their 10 ms neighbourhood after a first difference (scratchpad ticks.py): NB off 3.8 ticks/s (biggest 18-415x local, usually 30-50x), NB on 1.1/s (12-22x), two seconds with none; ticks back at once each time NB went off (real band impulse noise, irregular spacing 14 ms to hundreds of ms); no clicks at the switch moments; threshold 0.00999 throughout (3.147 thread fix holds). Ron finds NB hard to adjust. Why (blanker.c): threshold is an ABSOLUTE amount above the running envelope (envelope + threshold), so the same setting means different things on quiet / noisy bands; the envelope attack / release is 0.05 ms, so it nearly follows the pulse itself; no feedback on what it does; width is in raw 96 kHz samples (30 = 0.3 ms). **PARKED (Ron): automatic NB.** recv only: running level + spread over ~50-100 ms, blank only short samples far outside the spread (~5-6x), self-protection (less sensitive if blanking more than ~100/s); NB = on/off for the user (threshold knob could become Low / Medium / High: Stew). Includes "relative threshold" and "slower envelope"; pulse blanking (gain ramp) unchanged. Optional "blanking meter" (recv sends blanks per second, client shows it) needs Stew. When done: same change in the other recv copy, retest with the tick capture.
**2026-10-31 (evening): SSB / WSJT output power low (about 40 W of 100), fixed in trans 3.149 (BUILT on the Pi over ssh, NOT committed).** Cause: the 3.146 ALC ceiling 0.45 x drive (from Stew's Windows 3.149) held peaks to 0.45 / 0.8 (TUNE level) = about 32% of TUNE power. `ALC_CEILING` moved to `extern.h` (udp_thread.c CMD_SET_ALC_MULTIPLIER uses it, was a hard 0.45). 3.148 = 0.80: WSJT tone 89 W vs TUNE 109 W (ATU-100); 3.149 = 0.89: 95 W (PA near compression). Ron then recalibrated power (QRP and QRO): output power now right. Open (Stew, Ron 2026-10-31: no SSB voice test by Ron): SSB voice + splatter check on his analyzer with the 0.89 ceiling (ask, worded as a question).
**2026-10-31 (22:30): TX I/Q test PASSED (Ron).** CW, PHASE slider 11 -> 1 (TX), APPLY: ms-sdr.log (~/.local/mscc/ms-sdr.log) `Radio_cal_store_iq . TX band 4: offset -26, phase 1 (queued)`, 10 s later `Radio_cal_send_iq ... sent (radio stores it)`, client read back phase 1, no errors. Server code committed (ms-sdr 3.186, trans 3.149, recv 3.150), not pushed. Ron's open list is now empty; SSB voice + splatter check is Stew's.
**2026-10-31 (late): one ALC ceiling for voice and digital (Ron).** Stew's Windows trans 3.152 (e7e4056) uses 1.0 x drive for digital, 0.45 for voice. Ron: WSJT and voice are both just audio feeding the rig; the ALC only stops overdrive, so the ceiling = full calibrated power and mic gain sets the drive. Pi stays on 3.149 (0.89 for both; a 3.150 port of Stew's split was made and undone, never built). Note for Stew: `.mscc-coord/NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md` (question + options, asks for a 0.89 voice check on his Signal Hound).
**2026-10-31 (late night): NR replaced (option B), recv 3.151, BUILT on the Pi over ssh 23:16 (~/mscc/sdrcore-recv, no warnings), NOT COMMITTED. ssh from git-bash can't resolve `proficio`; PowerShell `ssh -i $HOME\.ssh\proficio_ed25519 ron@192.168.12.199` works.** `denoise()` in dsputils.c = spectral subtraction: 2048-pt FFT at 96 kHz (21 ms, 47 Hz bins), 50 % overlap, sqrt-Hann, per-bin noise = smoothed power min-tracking (rises 3 dB/s), Wiener gain with decision-directed SNR (anti musical noise), gain 0..1 only (never louder). Level 1..100: floor -6..-20 dB, oversubtraction 1..2. Passes audio unchanged for the first ~0.1 s after ON. Adds 21 ms delay while ON. `nr_delay()` removed. Offline test (scratchpad nrtest/t.c, white noise + 1 kHz tone 0.3 s on/off): noise-only -5.8 / -12.1 / -17.3 dB at level 1 / 50 / 100, tone kept (corr 0.98-1.00), output peak never above input. On-air A/B 23:25 local (40 m rag chew, level 50, NR toggled 3x in a 39 s Remote Phones capture, scratchpad nrtest/nrab.py): background between words -40.8 -> -53.6 dB (about -12.7 dB, as the offline test), talk level -29.6 -> -30.7 dB (about 1 dB less), talk-to-gap 11 -> 23 dB, >3 kHz share -13.6 -> -19 to -23 dB, peaks 0.07-0.10 on vs 0.11-0.20 off (no overshoot / clipping), no whistle (top tone 316-469 Hz = voice). Ron: "just a touch of warble, not bad ... much better than it was". 3.151 stays on the Pi. PARKED (Ron): warble fix = smooth the gain over neighbouring bins (+/-2 bins, ~+/-100 Hz), floor unchanged; alt: higher floor. Committed and pushed with note for Stew `.mscc-coord/NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md` (Windows recv has the identical old ALE). Windows recv (mscc-ui/windows-work-tree/SDRcore-recv) has the same old ALE -> note for Stew after the on-air test.
**Resume point 2026-10-31 (night), the list:** Pi runs recv 3.150, trans 3.149 (ALC ceiling 0.89, voice and digital), ms-sdr 3.186, pill 3.12 (22 I/Q bands stored). All server code committed and pushed (1e3ecb4, note 836c33f); local `rpi/si5351-drive.md` edits not needed (Ron 2026-10-31: pill runs fine), left out. Ron's open list: empty (TX I/Q test passed, power fixed). Parked: (3) NR: DONE as recv 3.151 (spectral subtraction, on the Pi); warble tweak (gain smoothing across bins) parked; (4) automatic NB (blanking meter optional); (5) sync AM (off the list); (5a) Windows Operator's Guide rebuild: the 60-page PDF reads out of order and mixes terms; no editable source in the repo. Proposed order: what MSCC is + glossary, before you start, install and first start, operating, calibration as one chapter (APPLY stores I/Q in the radio), settings, digital modes, appendices. Open questions: Windows guide first? write it for the client as it is, with a note to Stew about confusing names? Stew: (2) SSB voice + ALC meter on the air, splatter check with a 0.89 ceiling; (2a) one ALC ceiling question (NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md); (6) blanking meter / NB Low-Med-High display if wanted; (7) heat sink temperature 0x06 in the client; (8) Pi-side ports noted in his CLAUDE.md, new scripts into utility-scripts.

**2026-11-01: Windows Operator's Guide reviewed in full (high effort): `rpi/GUIDE-REVIEW-WINDOWS-2026-11-01.md`** (read it, not this line). Main points: cal now lives in the radio (QRP / TX IQ / RX IQ), so the Save settings text is wrong; TX IQ RESET ALL and Reset configuration don't stick (radio values win at next start: Stew code issue); TX IQ PHASE missing; RX IQ RESET ALL is now APPLY ALL; POTENTIA / QSK described wrongly (Ron's 9/29 text never applied); firmware table old; pictures have no number callouts; source (WeasyPrint HTML/MD) not in repo, ask Stew. Not committed.
**2026-11-01: new Windows guide written (Ron chose Word): `docs/manuals/MSCC-Operators-Guide-Windows-Release-1-DRAFT.docx`**, 43 pages, for client 10.31.2, reorganized as in the review (sections F), pictures reused from the Release 0 PDF, control lists keyed by the printed label (no number callouts needed). Gray DRAFT boxes + Appendix C list what to confirm (firmware table, cal survives FW update, ALC target, Windows NR, reset-to-factory, AMP CAL target) and pictures to retake. Built with python-docx from a light-markup text (scratchpad, session-only); from now on edit the .docx itself. TOC prefilled from a LibreOffice render; Word asks to update fields on open (Yes). Not committed.

**2026-11-01: automatic NB, recv 3.152, BUILT on the Pi (last build 08:53, copied to V:), committed and pushed with notes for Stew.** Files: `SDRcore-recv-linux/sources/blanker.c` (rewritten), `sdrcore.h` (blanker_state), `extern.h` (3.152). No client / ms-sdr change: NB button = on/off; threshold slider = sensitivity K 4 (slider max) .. 8 (slider min), default 6 (Ron's slider sat at min -> K 8); width as before. Pulse = |I/Q| > K x running mean (2 ms, pulses left out); a "pulse" longer than the width (max 2 ms) is a signal: un-marked before the gain falls (delay = width/2 + max pulse, 0.47 ms at width 30). Gain: 0 within +/- width/6 of any pulse sample, raised-cosine back to 1 at width/2 (no restart jump). Self-protection: >400 pulses/s raises K (up to 4x), <200/s lowers it. 50 ms learn after each (re)start. Log every 10 s while on: `Blanker . N pulses/s, K ...`. Offline test (scratchpad nbtest/t.c, WSL): noise only 0 false at K 6 / 8 (1.3/s at K 4); 30x pulses -27 dB at K 6; 40 dB SSB with instant start 0 false at K 6 (first version locked up: 60 % blanked; fixed). Next: build on the Pi, Ron listens, tick capture A/B (scratchpad ticks.py from 10-31 is gone; redo). On-air 08:30 (20 m USB, K 5.95, width 40, NB toggled): ticks >18x only with NB off (28-58x), NB on 0.8/s max 18x vs off 1.2-1.5/s; blanker 1-2 pulses/s; no holes. Slider test 08:34: every step restarted the blanker (ring zeroed = up to 4 ms silence at width 400): fixed 08:38, slider steps keep level and audio (K only = no change; width = ring resized, oldest dropped or oldest stretch repeated); log `Blanker . setting: K, width`. Offline 69 steps, 0 holes. Width now read as MICROSECONDS (client slider 10..510 is labelled uS; recv used samples = 10.4x longer, up to 5.3 ms); 200 us default; built 08:42. Threshold range unchanged (K 4..8, Ron OK). Note for Stew: Windows recv also reads width as samples. On-air 08:47 (200 us, K 6): big clicks (>25x) off 0.41/s, on 0.05/s (~9 of 10 removed); one 54x passed with NB on, cause unknown (too long >0.2 ms / busy band, mean 9x higher than 08:18 / not noise). Built 08:53: log counters per interval + summary when NB turned off: `N pulses/s (biggest Nx), too long N/s (biggest), near N/s (0.67K..K, biggest)`. Decide fix from data (maxrun fixed 1 ms if too long; linear-prediction detector if busy band). Ron to restart MSCC. On-air 08:55 (60 s NB on, 200 us, K 6): no audio tick >26x, blanked 1-2/s (biggest 50-78x), too long 0 in every interval (cause 1 ruled out, maxrun change not needed), near 0.3-3/s at 5.4-6.0x (option: threshold to ~5x). Notes for Stew: `.mscc-coord/NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md` (port blanker.c + sdrcore.h; client questions: Pulse min 50 us, threshold Low/Med/High = 1/500/750, 1001..1009 oddity, send NB state on connect) and `.mscc-coord/NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md` (NR slider -> Off/Low/Med/High = 0/15/50/85, client only). NR warble fix: Ron 2026-11-01 leave it (NR works well; lower level = less warble). Then a note for Stew (Windows recv, maybe Low/Med/High labels).

**2026-11-01: Remote guide rewritten (Word, same style as the Windows draft), WINDOWS ONLY (Ron: Windows client + Windows servers; Pi/Linux remote differs and needs its own guide, not written yet): `docs/manuals/MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx`**, 20 pages, client 10.31.2, server names as Windows .exe; session lock and ports also checked in Windows ms-sdr-MKII / SDRcore-recv / SDRcore-trans; pictures from the Release 0 PDF. Corrections from code: Phones / Digital chosen inside the Remote Audio window (main-window Phones / Digital turn Remote off); Remote off at each start; one client at a time (ms-sdr session lock, freed by Stop or ~45 s); Remote Digital CAT = client SETTINGS COM Port; closing the window keeps audio. 7 DRAFT items in Appendix C (Windows-host steps with the session lock, ipconfig picture, fixed host address, what a refused client shows, retake chapter 3 pictures, mic-level wording vs Windows ALC 0.45, WSJT-X audio picture). Built with python-docx from the Windows draft (scratchpad, session-only); edit the .docx itself from now on. Not committed.

**2026-11-01: `docs/manuals/MSCC-Configurations-DRAFT.docx` (13 pages, Word, same style): client-server explained (ch 1: client, the 3 servers, ship bridge / engine room picture, always client-server even on one PC per Ron), then the five setups in plain English** (Ron): 1 Windows local, 2 Windows client PC1 + Windows servers PC2, 3 Windows client + Pi/Linux servers, 4 Pi/Linux local, 5 Pi/Linux client + Pi/Linux servers. Each: diagram, who for, what runs where, client settings, audio, digital programs, which guide. Key rule (Ron): Windows host = client owns the COM port (SETTINGS → COM Port; setup 2 WSJT-X on the client with Remote Digital); Pi/Linux host = host owns everything (mscc-init sets COM ports /dev/tnt + audio, WSJT-X runs on the Pi, reached by VNC; Windows client only operates + Remote Phones). 4 DRAFT items (setup 2 host keeping servers alive; setup 3 mscc-init port/device names + what the client does for Pi digital audio; setups 4/5 MSCC UI steps, wait for the Avalonia review). Next (Ron): the Avalonia client, then a Pi/Linux remote guide. Not committed.
**2026-11-06: Configurations draft, setup 6 added (Ron: "looks fine"):** Pi / Linux client + Windows host (section 3.6, diagram, DRAFT box: tested with MSCC Avalonia 1.6.86, MSCC UI 0.6.x lacks the devices / saved choices / RTS, Windows servers stop ~45 s after the client leaves). "five" -> "six" throughout, 2.1 table row, 2.2 sentence, chapter 4 column 6, Appendix A item 2. Edited with python-docx (no Word on this PC); LibreOffice users: Tools > Update > Update All for the contents page numbers.
