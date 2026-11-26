# MSCC Software Versions Addendum

**Release R11-25-2** · 2026-11-25

The Operator's Guides describe how MSCC works; this addendum lists the version numbers that
go with the current release. It is updated with every release, so the guides don't change
when only version numbers do. Downloads and what changed in each release:
<https://github.com/w4mmp-ron/mscc-station/releases> (Windows details: `installers/windows/CHANGELOG.md`).

## Installers

| Computer | File |
|---|---|
| Windows | `mscc-net9-R11-25-2-install.exe` → `C:\mscc-net9` |
| Raspberry Pi (64-bit, Pi 4 / Pi 5) | `mscc_1.0.107_arm64.deb`, `mscc-avalonia_1.7.08_arm64.deb` |

Ubuntu packages: not in this release (R11-25-0 has 1.0.105 / 1.7.08 amd64).

## MSCC programs

| Program | Windows | Raspberry Pi | Ubuntu |
|---|---|---|---|
| MSCC client | WPF 11.25.2 | Avalonia 1.7.08 | Avalonia 1.7.08 |
| ms-sdr (server) | 3.214 | 3.216 | 3.215 |
| recv (receive server) | 3.155 | 3.158 | 3.157 |
| trans (transmit server) | 3.159 | 3.164 | 3.163 |

Avalonia 1.7.08 (Pi and Ubuntu): connect keeps the server's settings. LF not yet tested on Geminus.

WPF 11.25.2 (= 11.25.1 renumbered): Launch Servers is only for a Host on this PC. Another Host turns it off and greys it out, and Connect then never starts servers on this PC. Otherwise as 11.19.2. Remote digital: a one-cable VB-Cable setup no longer sends a short burst of received audio at key-down. While keyed, MSCC stops playing receive audio into the cable, and it drops the first 400 ms of transmit audio after key-down.

Pi ms-sdr 3.216, recv 3.158 and trans 3.164: stored CW carrier (Black Pill 3.25+), on with `CW_CARRIER=1;` in the Pi's mscc.ini (Pi install guide). `mscc status` on the Pi lists packages, servers, firmware, and the newest release. Pi ms-sdr 3.214 answers the CAT S-meter read, stops transmit on a WiFi SWR fault until reset (including a CAT key from WSJT-X), and keeps the QRO calibration in Black Pill 3.24 or later. Windows ms-sdr 3.214 uses Zadig or WinUSB; Black Pill 3.17 or later needs no Zadig.

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
| Black Pill board (any radio) | 3.29 (no Zadig; fixes its receive; keeps QRO cal; HDSDR / Quisk; Line devices; soft clocks; TX stats apart) | tested (Proficio MKII), Pi + Windows |
| Black Pill bootloader | 2026-11-16 (WinUSB in flash mode) | tested on the Pi |

All eight PSoC images and the Black Pill use the USB name Multus IQ; the MKII ones pass 0x9D (paddle Farnsworth) to the keyer (tested on all MKII radios).

Files: Windows `C:\mscc-net9\PSoC Firmware\` and `Black Pill Firmware\`; Pi `/usr/share/mscc/firmware/`.

## Keyer (MKII daughter board)

| Item | Version | Status |
|---|---|---|
| Keyer firmware | `Keyer-MCU-app-20261125.hex` (Farnsworth word spacing) | tested (Proficio MKII with Black Pill) |
| Keyer bootloader (put on once with the PICkit) | version 2, in `Keyer-MCU-full-20261125.hex` | tested |
| Keyer Firmware Update (window and command line) | R11-12-4 | tested on Windows and Raspberry Pi |

Keyer update needs Black Pill 3.16 or later. Files: `C:\mscc-net9\Keyer Firmware\` and `/usr/share/mscc/firmware/keyer/`.
