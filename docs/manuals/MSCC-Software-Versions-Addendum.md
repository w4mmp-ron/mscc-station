# MSCC Software Versions Addendum

**Release R11-12-4** · 2026-11-12

The Operator's Guides describe how MSCC works; this addendum lists the version numbers that
go with the current release. It is updated with every release, so the guides don't change
when only version numbers do. Downloads and what changed in each release:
<https://github.com/w4mmp-ron/mscc-station/releases> (Windows details: `installers/windows/CHANGELOG.md`).

## Installers

| Computer | File |
|---|---|
| Windows | `mscc-net9-R11-12-4-install.exe` (installs to `C:\mscc-net9`) |
| Raspberry Pi (64-bit Pi OS, Pi 4 / Pi 5) | `mscc_1.0.85_arm64.deb` (required) and `mscc-avalonia_1.6.96_arm64.deb` (optional) |

## MSCC programs

| Program | Windows | Raspberry Pi |
|---|---|---|
| MSCC client | WPF, title bar 11.11.4 | MSCC Avalonia 1.6.96 (optional) |
| ms-sdr (server) | 3.208 | 3.206 |
| recv (receive server) | 3.153 | 3.153 |
| trans (transmit server) | 3.158 | 3.158 |

The Windows and Raspberry Pi servers do the same job; their numbers differ only because each
build counts up on its own.

## Radio firmware

The radio's firmware must be at least the version below for this release (the calibration is
stored in the radio). After you connect, the client's title bar shows the radio's firmware as
**FW: major.minor**; the major number tells which radio it is. If yours is lower, update it
(Operator's Guide, Chapter 3).

| Radio | Firmware | Status |
|---|---|---|
| Proficio Legacy | 1.236 | |
| Geminus MKII | 2.154 | new in R11-12-3, **not tested on a radio yet** |
| Proficio MKII PTT | 3.244 | new in R11-12-3, **not tested on a radio yet** |
| Proficio MKII ATU | 4.245 | new in R11-12-3, **not tested on a radio yet** |
| Geminus Legacy | 5.153 | |
| Ultimus Legacy | 6.236 | |
| Ultimus MKII ATU | 7.245 | new in R11-12-3, **not tested on a radio yet** |
| Ultimus MKII PTT | 8.244 | new in R11-12-3, **not tested on a radio yet** |
| Black Pill board (any radio) | 3.16 | tested (Proficio MKII) |
| Black Pill update tools (Pi window new) | R11-12-4 | Pi tested; Windows not yet |

The five new PSoC versions add only the keyer update over USB. If you don't need that, you can
stay on the version before (2.153, 3.243, 4.244, 7.244, 8.243) until they are tested.

Where the files are: Windows `C:\mscc-net9\PSoC Firmware\<Radio>\` and
`C:\mscc-net9\Black Pill Firmware\`; Raspberry Pi `/usr/share/mscc/firmware/<Radio>/` and
`/usr/share/mscc/firmware/black-pill/`.

## Keyer (daughter board: Proficio, Geminus and Ultimus MKII)

| Item | Version | Status |
|---|---|---|
| Keyer firmware | `Keyer-MCU-app-20261112.hex` | tested (Proficio MKII with Black Pill) |
| Keyer bootloader (put on once with the PICkit) | version 2, in `Keyer-MCU-full-20261112.hex` | tested |
| Keyer Firmware Update (window and command line) | R11-12-4 | tested on Windows and Raspberry Pi |

Keyer Firmware Update needs radio firmware with the keyer update: Black Pill 3.16, or the PSoC
versions marked new above (2.154, 3.244, 4.245, 7.245, 8.244). Files: Windows
`C:\mscc-net9\Keyer Firmware\`, Raspberry Pi `/usr/share/mscc/firmware/keyer/`.
