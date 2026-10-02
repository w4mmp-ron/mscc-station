# Linux calibration parity with Windows and remote Save settings

**Status:** Phase 1 **cmd-060 done**. Phase 2 **cmd-061 done**. Phase 3 activated as **cmd-063** (optional `mscc-firmware` + Load File default). Phase 4 = WPF/Windows opcode still later.
**Request:** Stew, 2026-10-01

## Goal

Make Linux (Raspberry Pi and Ubuntu) work like Windows for multi-radio calibration, then make remote **Save settings** target the host rather than the local Windows client.

The desired end state is one clear story for local Windows, local Linux, and remote Windows-to-Linux operation. Windows cmd-055/cmd-059 are done; Linux must catch up before any further Windows park work is started.

## Current state and constraints

- Windows cmd-055/cmd-059 are shipped: live calibration is under `%LocalAppData%\\MSCC-NET9\\`, parked calibration is under `cal\\<line>\\` with `LAST_LINE.txt`, and WPF **Save settings** copies live files to the Windows park.
- Windows uses detection-driven same/swap/new behavior. There are no continuous live-to-parked mirrors and no exit auto-park in v1.
- Pi and Ubuntu currently have the `init-files` seed only. They do not yet have the packaged per-line `factory/` tree, Linux `cal/<line>/` park, or `LAST_LINE` state.
- In remote Windows-to-Pi/Linux use, calibration edits belong to the host's live files, but the current WPF Save settings path parks the Windows client's files. That is the behavior to correct.
- `sdrcore-trans` remains the owner of `power_cal.ini` creation, loading, and saving. Linux park/restore must not create a second owner or continuously overwrite that file.

## Locked decisions (Stew 2026-10-02)

1. **One host path for cal location.** Local is mini-remote (client↔servers). **Save settings** is one command: “park live for this radio line.” The host (servers) performs the copy using that OS's paths. The client does not choose AppData vs `~/.local/mscc/`, and the cal-location button has no local/remote special case. This is the same for Linux↔Windows, Linux↔Pi, Pi↔Windows, and Pi↔Linux: the client does not care where calibration files live.
2. **Detection park/restore stays host-side** on connect/load.
3. **Factory packaging:** ship per-line `factory/` inside the `mscc` servers deb (Pi + Ubuntu), for example `/usr/share/mscc/factory/...`, updateable with package upgrades. Live and parked data under `~/.local/mscc/` are user-owned; the package never overwrites them. Copy-paste is for one-off smoke tests only, not shipping.
4. **PSoC firmware:** use a separate optional package (for example `mscc-firmware` or flash-tools), not every `mscc` install. Keil builds stay Shack-only; the package ships `.cyacd`/`.hex` artifacts only.
5. Until host park plus the host Save-settings command exist, remote must not pretend to park via client-local `CalPark`.

## Ordered phases

### 1. Ship per-line factory data on Linux

Ship the repository `factory/` tree inside the `mscc` servers Debian package for both Pi and Ubuntu. The package should install the per-line factory templates for the calibration families (`iq`, `freq`, and `power`, keyed by the supported line names) in a stable, package-owned location such as:

```text
/usr/share/mscc/factory/...
```

Package upgrades may update that factory tree, but must never overwrite user-owned live or parked data under `~/.local/mscc/`. The exact installed path may instead be next to the binaries if that is the existing Linux packaging convention, but it must be stable and shared by the server/UI implementation. Copy-paste is only for one-off smoke tests, not the shipping mechanism.

Clarify the relationship between the flat `init-files` and `factory/`: `init-files` may remain for non-calibration seed/default files, while per-line calibration fallback comes from `factory/`. If both contain calibration templates during the transition, document which one wins and avoid silently using a flat, model-independent calibration as a multi-radio fallback.

**Phase gate:** a fresh Pi and Ubuntu package contain the same named per-line factory inputs; an upgrade refreshes package factory data without overwriting `~/.local/mscc/` live or parked user data.

### 2. Add cmd-055-equivalent park/restore on Linux

Use the Linux user's live directory and a per-line park, with the exact filenames and permissions settled in the implementation brief:

```text
~/.local/mscc/                 # live files used while the servers run
~/.local/mscc/cal/<line>/      # parked set for that radio line
~/.local/mscc/LAST_LINE        # or the established .txt spelling, consistently
```

The implementation must be detection-driven:

- **Same line:** leave the current live calibration in place.
- **Swap:** identify the prior line from the recorded state/model, preserve its live set in that line's park, then restore the detected line's parked set into live.
- **Brand-new line:** start from that line's packaged factory set, place the initial set in its park, and restore it to live.

The parked/live manifest must cover the Linux equivalent of the Windows calibration set (including IQ and frequency calibration, and power calibration where applicable). `power_cal.ini` must continue to be created, loaded, and saved by `sdrcore-trans`; the park/restore orchestration must preserve that ownership and must not have a competing calibration writer.

Match the Windows v1 operating rules:

- no continuous live-to-parked mirrors during normal calibration;
- no automatic park on process exit in v1;
- a deliberate **Save settings** action copies the current live set to the selected/detected line's park;
- `LAST_LINE` and detection are used on load/switch, not a manual radio-switch tool as the primary path.

Avalonia (local Linux and, where applicable, the remote client) needs a **Save settings** action or equivalent that invokes the host-side behavior and copies live to parked on the host. It must not silently save a remote host's calibration into local client files.

**Phase gate:** repeated same-line starts preserve live calibration; a radio swap restores the correct line; a new line uses its packaged factory fallback; a deliberate save survives restart; and a restart does not undo a QRP/power calibration because of a stale mirror.

### 3. Optional: separate PSoC firmware package

If useful for field operation, provide a separate optional package (for example `mscc-firmware` or flash-tools), rather than putting firmware into every `mscc` installation. It ships distributable `.cyacd` and/or `.hex` artifacts only; Keil builds remain Shack-only, and the Pi must not be expected to rebuild the firmware. Define artifact versions, destination, and update/rollback behavior separately from the calibration work.

This phase is optional and must not block Linux calibration parity.

### 4. Make Save settings a host command

Save settings is one user-visible command: “park live for this radio line.” Local is mini-remote (client↔servers), so the client must not choose a Windows AppData path versus `~/.local/mscc/`, and the button must not have a local/remote cal-location special case. The connected host's servers perform the copy using that OS's paths. This applies equally to Linux↔Windows, Linux↔Pi, Pi↔Windows, and Pi↔Linux.

Detection-driven park/restore on connect/load also remains host-side. The host operation must park the host's live set to the selected/detected line and must not use client-local `CalPark` as a substitute. Until host park and the host Save-settings command exist, remote must disable or hide Save settings and explain that host-side multi-radio save is not available yet; it must not offer a button that appears to succeed while saving client-local files.

Long term, Windows, Linux, and Pi clients should share the same semantics: live while running, explicit host Save settings to park the line, and host-side detection-driven restore.

## Delivery order and non-goals

This is a plan for later, ordered one-command-at-a-time implementation—not an activated Build brief. Implement phase 1 first, then phase 2, then the UI/remote pieces in phase 4; phase 3 can proceed independently if chosen. Add host-specific briefs only when the preceding phase's gate and ownership details are ready.

Do not do more Windows park work while Linux is behind. Do not add continuous mirrors, exit auto-park, or a primary manual switch-radio workflow in v1. Do not rebuild PSoC firmware on Pi. No source code, package build, installer publication, or Build brief activation is part of this planning change.
