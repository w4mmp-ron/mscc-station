# Manual paste-ready updates from Ron (QUESTIONS-FOR-RON.md, answered 2026-09-28/29)

Apply when regenerating the Operator's Guide PDFs. Source PDFs today live under
`docs/manuals/`; there is no checked-in ODT/DOCX — paste these into your draft
source, clear the yellow "Question for Ron" boxes, then re-export the PDFs.

Canonical answers: `.mscc-coord/QUESTIONS-FOR-RON.md`.

---

## Linux / Pi — §2.2 Raspberry Pi install (was yellow on ~p.7)

**Remove** the yellow question about `usermod` on the Pi.

**Replace** the Pi "Log out…" paragraph with:

> Log out and log back in (or restart the Pi) after the **first** install. The
> `mscc` servers package postinst already adds the installing user (the account
> that ran `sudo apt install`) to `dialout`, `plugdev`, and `audio`. You do **not**
> need to run `sudo usermod -aG dialout,audio,plugdev "$USER"` yourself for that
> account.
>
> Any **other** login that will run MSCC still needs the groups:
>
> ```bash
> sudo usermod -aG dialout,audio,plugdev "$USER"
> ```
>
> then that user must log out and back in (or reboot).

Keep the Ubuntu block as it is (`usermod` + log out before first use).

---

## Linux / Pi — §2.6 Upgrading / "settings kept" (was yellow on ~p.16)

**Keep** the statement that settings and calibration are kept on upgrade — Ron
confirmed it is correct.

**Remove** the yellow question about overwrite.

**Optional clarifying note** (one short paragraph after the keep sentence):

> On install or upgrade the servers package seeds `~/.local/mscc/` only if that
> folder is missing or empty; an existing folder is left untouched (uninstall
> leaves it too). The servers create a calibration file only when it is missing;
> they never overwrite one. The only client actions that replace them are FREQ CAL
> RESET (`freq_cal.ini`) and I/Q defaults (`iq.ini`). If a single file is deleted,
> the next server start recreates it with factory values; if the whole folder is
> emptied, the next install/upgrade reseeds the package set.

---

## Linux / Pi — CW tab PHONES and POTENTIA / QSK (~p.30)

**Remove** the yellow "Question for Ron" / "Same as Windows" open line.

**Paste:**

> **POTENTIA / QSK.** For Potentia 50/100 amplifiers (full break-in). **On:** the
> radio keys the AMP line and the PA together at key-down. **Off (default):** AMP
> line first, then PA, so a relay-switched amp has time to change over. Leave off
> unless a Potentia is attached.
>
> **PHONES.** Not used. The client may still send opcode `0x70`, but ms-sdr (Pi and
> Windows) ignores it. (UI removal of this checkbox is tracked separately; until
> then you can ignore it.)

---

## Windows — CW tab PHONES and POTENTIA / QSK (~p.17)

**Remove** both yellow "Question for Ron" lines.

**Paste** the same two paragraphs as the Linux CW tab above (POTENTIA / QSK, then
PHONES not used).

---

## After PDF refresh

- Update `docs/manuals/README.md` revision note.
- Drop this file or mark it Applied once both Linux and Windows PDFs are refreshed.
