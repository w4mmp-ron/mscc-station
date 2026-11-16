# MSCC Software Versions Addendum

**Release R11-16-1** · 2026-11-16

The Operator's Guides describe how MSCC works; this addendum lists the version numbers that
go with the current release. It is updated with every release, so the guides don't change
when only version numbers do. Downloads and what changed in each release:
<https://github.com/w4mmp-ron/mscc-station/releases> (Windows details: `installers/windows/CHANGELOG.md`).

## Installers

| Computer | File |
|---|---|
| Windows | `mscc-net9-R11-16-1-install.exe` (installs to `C:\mscc-net9`; uninstall from Apps & features) |
| Raspberry Pi (64-bit Pi OS, Pi 4 / Pi 5) | `mscc_1.0.96_arm64.deb` (required) and `mscc-avalonia_1.7.05_arm64.deb` (optional) |

## MSCC programs

| Program | Windows | Raspberry Pi |
|---|---|---|
| MSCC client | WPF, title bar 11.15.0 | MSCC Avalonia 1.7.05 (optional) |
| ms-sdr (server) | 3.214 | 3.210 |
| recv (receive server) | 3.155 | 3.156 |
| trans (transmit server) | 3.159 | 3.160 |

Avalonia 1.7.05 is built and tested on Ubuntu (Ultimus MKII): packages 12.1.6 (idle-CPU fix), the AGC button shows the saved SLO / MED / FST at start, and CMP is greyed in Digital. QRP and AMP cal still use the last-tuned frequency and select the band when you open the tab; AGC FAST RELEASE is 0–2000 ms in steps of 10. It will be attached at the next GitHub Release. The Pi arm64 package was not built in this step. LF not yet tested on Geminus.

Windows ms-sdr 3.214 talks to the radio through the Zadig driver or through Windows' own WinUSB driver, and picks the right one by itself. With Black Pill 3.17 no Zadig is needed. It also fixes the WiFi SWR meter link on Windows.

Windows and Pi server numbers differ because each build counts up on its own.

## Radio firmware

The radio's firmware must be at least the version below for this release (the calibration is
stored in the radio). After you connect, the client's title bar shows the radio's firmware as
**FW: major.minor**; the major number tells which radio it is. If yours is lower, update it
(Operator's Guide, Chapter 3).

| Radio | Firmware | Status |
|---|---|---|
| Proficio Legacy | 1.237 | tested |
| Geminus MKII | 2.156 | tested |
| Proficio MKII PTT | 3.246 | tested |
| Proficio MKII ATU | 4.247 | tested |
| Geminus Legacy | 5.154 | **built, not hardware-tested** |
| Ultimus Legacy | 6.237 | tested |
| Ultimus MKII ATU | 7.247 | tested |
| Ultimus MKII PTT | 8.246 | tested |
| Black Pill board (any radio) | 3.17 (WinUSB, no Zadig) | tested (Proficio MKII), Windows and Pi |
| Black Pill bootloader (ST-Link, new boards) | 2026-11-16 (WinUSB in flash mode) | tested on the Pi |

All eight images use the USB name Multus IQ; the MKII ones pass 0x9D (paddle Farnsworth) to the keyer (tested on all MKII radios).

Files: Windows `C:\mscc-net9\PSoC Firmware\` and `Black Pill Firmware\`; Pi `/usr/share/mscc/firmware/`.

## Keyer (daughter board: Proficio, Geminus and Ultimus MKII)

| Item | Version | Status |
|---|---|---|
| Keyer firmware | `Keyer-MCU-app-20261112.hex` | tested (Proficio MKII with Black Pill) |
| Keyer bootloader (put on once with the PICkit) | version 2, in `Keyer-MCU-full-20261112.hex` | tested |
| Keyer Firmware Update (window and command line) | R11-12-4 | tested on Windows and Raspberry Pi |

Keyer Firmware Update needs radio firmware with the keyer update: Black Pill 3.16 or later, or the MKII
PSoC versions in the table above. Files: Windows
`C:\mscc-net9\Keyer Firmware\`, Raspberry Pi `/usr/share/mscc/firmware/keyer/`.
