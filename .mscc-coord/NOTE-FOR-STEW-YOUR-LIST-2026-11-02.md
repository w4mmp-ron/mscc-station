# For Stew: what we think is on your list

From Ron, 2026-11-02. A check that our lists match. Tell me if anything is missing, done already,
or not yours.

## Most important

1. **Automatic NB: port it to Windows recv**, then answer the client questions (Pulse min 50 us,
   threshold Low/Med/High = 1/500/750, the 1001..1009 slider oddity, send the NB state on connect).
   `NOTE-FOR-STEW-RECV-NB-AUTOMATIC-2026-11-01.md`
2. **NR control as Off / Low / Med / High** = 0 / 15 / 50 / 85. Client only.
   `NOTE-FOR-STEW-NR-LOW-MED-HIGH-2026-11-01.md`
3. **Spectral NR: port it to Windows recv.** The Windows recv still has the old NR that distorts.
   `NOTE-FOR-STEW-RECV-NR-SPECTRAL-2026-10-31.md`

## Next

4. **SSB voice on the air with the ALC meter**, and a splatter check on your analyzer at the 0.89
   ceiling. Also the question of one ALC ceiling for voice and digital.
   `NOTE-FOR-STEW-ALC-ONE-CEILING-2026-10-31.md`
5. **TX IQ RESET ALL and Reset configuration don't stick.** The radio values win at the next start.
6. **Heat sink temperature (0x06) in the client.** Also the Pi-side ports noted in your CLAUDE.md,
   and the new scripts into `utility-scripts`.
7. **Client side of 0x2E** (SWR meter RESET through ms-sdr). You already have this one.

## Guides

8. **Windows Operator's Guide Release 1 draft:** the Appendix C answers (firmware table, does cal
   survive a firmware update, ALC target, Windows NR, reset to factory, AMP CAL target), plus
   pictures and the guide source.
9. **Review the Windows parts** of `MSCC-Configurations-DRAFT.docx` (setups 1 and 2) and
   `MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx` (7 draft items). The key question:
   how does a Windows host keep the servers running with the one-client lock?
   `NOTE-FOR-STEW-USER-DOCUMENTS-2026-11-01.md`

## Parked on your side

10. **PSoC 12 kHz comb (live-slot fade).** Parked until you know the USB vs DAC slot timing. It
    can't be heard on the air, so there's no rush.
