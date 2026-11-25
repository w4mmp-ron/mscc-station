# MSCC Software Versions Addendum

**Release R11-24-0** · 2026-11-24

The Operator's Guides describe how MSCC works; this addendum lists the version numbers that
go with the current release. It is updated with every release, so the guides don't change
when only version numbers do. Downloads and what changed in each release:
<https://github.com/w4mmp-ron/mscc-station/releases> (Windows details: `installers/windows/CHANGELOG.md`).

## Installers

| Computer | File |
|---|---|
| Windows | `mscc-net9-R11-24-0-install.exe` → `C:\mscc-net9` |
| Raspberry Pi (64-bit, Pi 4 / Pi 5) | `mscc_1.0.104_arm64.deb`, `mscc-avalonia_1.7.08_arm64.deb` |
| Ubuntu (amd64) | `mscc_1.0.102_amd64.deb`, `mscc-avalonia_1.7.08_amd64.deb` |

## MSCC programs

| Program | Windows | Raspberry Pi | Ubuntu |
|---|---|---|---|
| MSCC client | WPF 11.24.0 | Avalonia 1.7.08 | Avalonia 1.7.08 |
| ms-sdr (server) | 3.214 | 3.215 | 3.215 |
| recv (receive server) | 3.155 | 3.157 | 3.157 |
| trans (transmit server) | 3.159 | 3.163 | 3.163 |

Avalonia 1.7.08 (Pi and Ubuntu): connect keeps the server's settings. LF not yet tested on Geminus.

WPF 11.24.0 is the same client as 11.19.2 with a new number. Remote digital: a one-cable VB-Cable setup no longer sends a short burst of received audio at key-down. While keyed, MSCC stops playing receive audio into the cable, and it drops the first 400 ms of transmit audio after key-down.

`mscc status` on the Pi lists packages, servers, firmware, and the newest release. Pi ms-sdr 3.214 answers the CAT S-meter read, stops transmit on a WiFi SWR fault until reset (including a CAT key from WSJT-X), and keeps the QRO calibration in Black Pill 3.24 or later. Pi ms-sdr 3.215, recv 3.157 and trans 3.163 carry their version. Pi ms-sdr 3.211 says "RECEIVE FIXED ITSELF" when a Black Pill fixed its receive. Windows ms-sdr 3.214 uses Zadig or WinUSB; Black Pill 3.17 or later needs no Zadig.

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
| Black Pill board (any radio) | 3.27 (no Zadig; receive fixes itself; keeps the QRO cal; HDSDR / Quisk; Line devices as the PSoC) | tested (Proficio MKII), Pi |
| Black Pill bootloader (ST-Link, new boards) | 2026-11-16 (WinUSB in flash mode) | tested on the Pi |

All eight PSoC images and the Black Pill use the USB name Multus IQ; the MKII ones pass 0x9D (paddle Farnsworth) to the keyer (tested on all MKII radios).

Files: Windows `C:\mscc-net9\PSoC Firmware\` and `Black Pill Firmware\`; Pi `/usr/share/mscc/firmware/`.

## Keyer (MKII daughter board)

| Item | Version | Status |
|---|---|---|
| Keyer firmware | `Keyer-MCU-app-20261112.hex` | tested (Proficio MKII with Black Pill) |
| Keyer bootloader (put on once with the PICkit) | version 2, in `Keyer-MCU-full-20261112.hex` | tested |
| Keyer Firmware Update (window and command line) | R11-12-4 | tested on Windows and Raspberry Pi |

Keyer update needs Black Pill 3.16 or later. Files: `C:\mscc-net9\Keyer Firmware\` and `/usr/share/mscc/firmware/keyer/`.
