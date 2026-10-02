# Linux calibration parity with Windows and remote Save settings

**Status:** Phase 1-2 done on stew-HP (cmd-060/061, may be unpushed). Phase 4 activated as **cmd-062** (Windows ms-sdr 0x29 + WPF Save to host).
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

## Ordered phases

### 1. Ship per-line factory data on Linux

Package the repository `factory/` tree in the `mscc` Debian package for both Pi and Ubuntu. The package should install the per-line factory templates for the calibration families (`iq`, `freq`, and `power`, keyed by the supported line names) in a stable, package-owned location such as:

```text
/usr/share/mscc/factory/...
```

Keeping the templates in the package makes them updateable with a package release without treating a user's live or parked calibration as package-owned data. The exact installed path may instead be next to the binaries if that is the existing Linux packaging convention, but it must be stable and shared by the server/UI implementation.

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

### 3. Optional: bundle PSoC firmware in Linux installers

If useful for field operation, include the distributable PSoC firmware artifacts (`.cyacd` and/or `.hex`) in the Linux installer(s) so Linux can flash or update easily. This is packaging only: Keil builds remain Shack-only, and the Pi must not be expected to rebuild the firmware. Define artifact versions, destination, and update/rollback behavior separately from the calibration work.

This phase is optional and must not block Linux calibration parity.

### 4. Make remote Save settings target the host

The WPF Save settings button must choose its destination by connection mode:

- **Local Windows:** copy live files to the Windows AppData park.
- **Remote Windows client connected to Linux:** copy the host's live set to the host's `~/.local/mscc/cal/<line>/` park, through the supported host-side operation; do not use Windows AppData as the effective park.
- **Local Linux/Avalonia:** use the same Linux host live/park behavior.

Until Linux host park/restore exists, disable or hide Save settings in remote mode and show a clear explanation that host-side multi-radio save is not available yet. Do not offer a button that appears to succeed while only saving client-local files.

Long term, local Windows, local Linux, and remote Windows-to-Linux should share the same user-visible semantics: live while running, explicit Save settings to the machine hosting the calibration, and detection-driven restore on that host.

## Delivery order and non-goals

This is a plan for later, ordered one-command-at-a-time implementation—not an activated Build brief. Implement phase 1 first, then phase 2, then the UI/remote pieces in phase 4; phase 3 can proceed independently if chosen. Add host-specific briefs only when the preceding phase's gate and ownership details are ready.

Do not do more Windows park work while Linux is behind. Do not add continuous mirrors, exit auto-park, or a primary manual switch-radio workflow in v1. Do not rebuild PSoC firmware on Pi. No source code, package build, installer publication, or Build brief activation is part of this planning change.
