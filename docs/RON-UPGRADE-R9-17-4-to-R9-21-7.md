# Upgrade — R9-17-4 → R9-21-7: config / INI notes for Ron

**Station assumed:** Windows 11 **WPF client** → **Pi servers** (radio on the Pi).  
**Installers:** `mscc-net9-R9--21-7-install.exe` + Pi `mscc_1.0.44_arm64.deb`  
**UI change list:** [`RON-2026-09-21-UI-CHANGES.md`](RON-2026-09-21-UI-CHANGES.md)

This sheet is about **what to delete (or not)** so radio cal and client last-used behave correctly. Ordinary install steps are short at the end.

---

## Where the files live (important)

| Kind | Machine | Path |
|------|---------|------|
| **Client** UI / last-used / Remote Digital mic | **Windows PC** | `%LocalAppData%\MSCC-NET9\` |
| **Radio cal** (IQ, QRP, freq PPM) | **Host that runs ms-sdr** | Same idea, different OS |

For your usual remote setup the **host is the Pi**, so live radio cal is:

```text
~/.local/mscc/iq.ini
~/.local/mscc/power_cal.ini
~/.local/mscc/freq_cal.ini
(+ other ms-sdr inis in that folder)
```

On a PC that runs **Windows servers locally**, radio cal is:

```text
%LocalAppData%\MSCC-NET9\iq.ini
%LocalAppData%\MSCC-NET9\power_cal.ini
%LocalAppData%\MSCC-NET9\freq_cal.ini
%LocalAppData%\MSCC-NET9\cal\<product-line>\   ← per-radio IQ/QRP cache (new in this release)
C:\mscc-net9\factory\…                         ← ship tables next to ms-sdr (new)
```

**WPF client settings (always on the PC you operate from):**

```text
%LocalAppData%\MSCC-NET9\MSCC_Client.ini
%LocalAppData%\MSCC-NET9\MSCC_LastUsed.ini
%LocalAppData%\MSCC-NET9\MSCC_LastUsed_VFOB.ini
```

`REMOTE_DIGI_MIC_VOL` lives in `MSCC_Client.ini`.

---

## Default answer: do **not** wipe INIs

For R9-17-4 → R9-21-7 you **normally delete nothing**.

| Keep | Why |
|------|-----|
| Pi `~/.local/mscc/*.ini` | Your tuned IQ / QRP / PPM on the radio host. Installer does **not** replace these. |
| Windows `%LocalAppData%\MSCC-NET9\MSCC_Client.ini` | Spectrum, window, Remote Audio prefs, digi mic slider default. |
| Windows `MSCC_LastUsed*.ini` | Per-band last freq/mode. New client **stops writing junk while Stopped**; one Start with DIG-U selected cleans bad rows better than a wipe. |
| Windows live `iq.ini` / `power_cal.ini` / `freq_cal.ini` (if you ever run local servers) | First start of the new ms-sdr **bootstraps** existing live IQ/QRP into `cal\<line>\` — it does **not** overwrite a tuned radio with factory. |

---

## When factory / ship cal actually pulls in

### A) Your normal case — **Pi is the host**

New **factory IQ / QRP / PPM tables** in this Windows release live under `C:\mscc-net9\factory\` and are applied by **Windows ms-sdr**.  
**Pi ms-sdr does not yet run that factory seed path.**

So for Ron’s Win→Pi station:

- Installing R9-21-7 on Windows **does not** by itself rewrite Pi `~/.local/mscc` cal.
- Deleting Pi INIs **will not** magically install the new factory trees (they are not in the Pi `.deb` the same way).
- **Leave Pi cal alone** unless Stew gives you a deliberate Pi cal refresh.

Pi `mscc_1.0.44` still matters for **remote audio** (phones headroom + mic stream reset), not for factory IQ tables.

### B) Windows is the host (local Launch Servers)

Then the new rules apply:

1. Installer drops `C:\mscc-net9\factory\…` next to `ms-sdr-MKII.exe`.
2. On start, after FW major is known:
   - If `cal\<line>\iq.ini` (or power) **exists** → that cache wins.
   - Else if live `iq.ini` / `power_cal.ini` **exists** (upgrade from R9-17-4) → **keep live**, copy into `cal\<line>\` (bootstrap).
   - Else → copy from `factory\…` into live **and** `cal\<line>\`.
   - `freq_cal.ini`: factory copy **only if live file is missing**. Existing PPM is kept.

**To force ship factory IQ/QRP/PPM on a Windows host** (destructive — loses user fine-tune):

1. Stop MSCC / servers.
2. Either:
   - **Settings → Reset configuration** (wipes AppData config except `logs\`, deletes `cal\`, leaves radio cal missing so next start seeds factory), **or**
   - Manually delete:
     - `%LocalAppData%\MSCC-NET9\iq.ini`
     - `%LocalAppData%\MSCC-NET9\power_cal.ini`
     - `%LocalAppData%\MSCC-NET9\freq_cal.ini` *(only if you also want factory PPM)*
     - entire `%LocalAppData%\MSCC-NET9\cal\` folder
3. Start servers with the radio connected so FW major is known.

**Non-destructive factory pull (preferred when you only want one concern):**

| Want | Do this (radio connected) |
|------|---------------------------|
| Factory TX IQ | **TX IQ → Reset All** |
| Factory QRP / power_cal | **QRP / power Reset** (if present) |
| Factory freq PPM | **FREQ CAL → Reset** |
| Full client + cal reseed | **Settings → Reset configuration** |

Those Reset buttons overwrite live **and** `cal\<line>\` from `C:\mscc-net9\factory\` for the **connected** FW major.

---

## Client last-used / DIG-U (Windows AppData)

**Do not delete `MSCC_LastUsed.ini` just for this upgrade.**

Old builds could write a “poison” last-used (e.g. 40m / USB) while the radio was **Stopped**. New client only saves last-used while **running** with a real frequency.

**One-time operator fix (no delete):**

1. Install R9-21-7, Connect, **Start**.
2. Pick the band you care about → set **DIG-U** (or your preferred mode).
3. Leave it running a moment (or Stop cleanly after). That rewrites the band’s last-used row.

Only wipe last-used if bands still behave crazy after that:

```text
%LocalAppData%\MSCC-NET9\MSCC_LastUsed.ini
%LocalAppData%\MSCC-NET9\MSCC_LastUsed_VFOB.ini
```

(Close WPF first.) Next Start rebuilds defaults / digi LAST HF·LF (14.074 / 474.2 when empty).

---

## Remote Digital mic slider

No INI delete. After upgrade, open Remote Digital — slider defaults to **100%** and saves as `REMOTE_DIGI_MIC_VOL` in `MSCC_Client.ini`. Turn down only if the VAC chain is hot.

---

## Short checklist for Ron

1. **Close** WPF / digi apps; on Pi `mscc stop`.
2. Install Windows **`mscc-net9-R9--21-7-install.exe`** (note the double dash).
3. On Pi: `./install-mscc.sh --reinstall ./mscc_1.0.44_arm64.deb` then `mscc start`.
4. **Do not** delete Pi `~/.local/mscc` or Windows AppData cal “to pick up factory” — that does not apply to Win→Pi the way it does for local Windows servers.
5. Connect from WPF → Start → set **DIG-U** once on your digi band if mode looks wrong.
6. Smoke Remote Digital TUNE; if mush after a SESSION IN USE fight, recycle Pi servers once (software fix is in 1.0.44, recycle still helps).

### Only if Stew asks you to load **new factory IQ** on a **Windows** host

Use **TX IQ Reset All** / **FREQ CAL Reset** / **Settings → Reset configuration** as above — not a blind delete on the Pi.

---

## Quick reference — delete map

| Action | Win→Pi (your station) | Windows local servers |
|--------|------------------------|------------------------|
| Delete Pi `~/.local/mscc` cal | **No** (unless Stew says) | n/a |
| Delete Win `MSCC_Client.ini` | **No** | **No** |
| Delete Win `MSCC_LastUsed*.ini` | **Only if** DIG-U still broken after one Start+set | Same |
| Delete Win `iq.ini` + `cal\` | **No effect** on Pi radio cal | Only to force factory seed |
| Settings → Reset configuration | Resets **client PC** AppData; does **not** reseed Pi host cal | Full client + factory reseed on next server start |
