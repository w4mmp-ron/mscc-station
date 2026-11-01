# For Stew: new user documents (drafts) to review

From Ron, 2026-11-01.

## Why

Users get confused because MSCC is a client and servers, and there are several ways to set it up.
So we wrote a short document that explains this first. We also rewrote the Remote and Linux guides
so they match today's software. All the drafts are Word files in `docs/manuals/`, in the same style
as the Windows Release 1 draft you already have (`NOTE-FOR-STEW-WINDOWS-GUIDE-2026-11-01.md`).

| Document | Pages | What it is |
|---|---|---|
| `MSCC-Configurations-DRAFT.docx` | 13 | New. Explains client-server in plain words, then the five ways to set MSCC up. Meant to be read first. |
| `MSCC-Operators-Guide-Remote-Windows-Release-1-DRAFT.docx` | 20 | Rewrite of Remote Release 0. Windows client and Windows host only. |
| `MSCC-Operators-Guide-Linux-Release-1-DRAFT.docx` | 39 | Rewrite of Linux Release 0. Raspberry Pi and Ubuntu in one guide. |
| `MSCC-Operators-Guide-Windows-Release-1-DRAFT.docx` | 43 | Sent earlier (see that note). Unchanged. |

I always run a Windows client with Pi servers, so I can check that setup myself. The Windows host
and the Windows and Ubuntu packages are yours, so those parts need you.

Each draft marks open points with gray "DRAFT" boxes, and collects them in its last appendix.
Please answer here, or edit the .docx files directly. The Release 0 sources (WeasyPrint) aren't in
the repo, so the .docx files are now the source.

## What I need from you, most important first

1. **Agree (or not) with the two working rules below.** The Linux guide depends on them.
2. **Windows host and the one-client limit.** How does a Windows host keep its servers running
   for a remote client? See the Remote guide below.
3. **NB threshold in MSCC UI.** If MSCC UI sends 1..100, the new noise blanker barely reacts.
   See rule 1.
4. Read setups 1 and 2 in the Configurations document and the whole Windows Remote guide, and
   answer the questions there.
5. The Linux guide items that are yours (end of this note).

## Two working rules (Ron, 2026-11-01)

The documents are written as if both of these are already true.

### Rule 1: MSCC UI works like the current Windows client

The Linux guide and the Configurations document say MSCC UI and MSCC for Windows look and work the
same way. Today they differ in these places (Linux guide Release 0, mscc-ui 0.6.72; I haven't read
the MSCC UI code):

- **NB threshold.** Release 0 shows **Thr 1–100**. The new noise blanker in Pi recv 3.152 uses the
  Windows client's **1..1009** scale: K = 4 + 4 × ((1000 − slider) / 100). With a 1..100 slider, K
  only moves between 7.6 and 8, so the slider does almost nothing.
- **NB pulse width.** Pls 10–510 is now read as microseconds, as the Windows "uS" label says.
- **ALC switch.** The RX/TX tab still has ALC on/off. Windows removed it (ALC is always on).
- **Remote.** Phones / Digital are chosen inside the Remote Audio window. Remote is off at each
  start. One client at a time. All as in the Windows client.

### Rule 2: the Ubuntu scripts and utilities match the Pi

I compared the code. MSCC Init asks the same questions and writes the same files on both.
`mscc start/stop/status`, the status report and `install-mscc.sh` are identical apart from arm64 /
amd64. But the Pi kit is newer:

| | Raspberry Pi kit | Ubuntu kit |
|---|---|---|
| MSCC Init | package `mscc-init` 1.0.18 | package `mscc-init-gui` 1.0.13 |
| Text wizard over ssh (`mscc-init`) | yes | no |
| Re-running MSCC Init keeps other `mscc.ini` lines and adds the WiFi SWR meter defaults | yes | no |
| Fix for a hidden dialog box at start | yes | no |
| MSCC Volume (VirtualA / VirtualB levels) | package `mscc-volume` 1.0.1 | not in the kit |
| Install | apt only, in this order: portaudio → mscc → mscc-init → optional ones (`installers/rpi/INSTALL.md`) | `install-mscc.sh` for the servers |
| Servers package | mscc 1.0.72 | mscc 1.0.53 |

With both rules in place, one Linux guide covers the Pi and Ubuntu, and only the package file names
differ (`_arm64` / `_amd64`).

## Document 1: MSCC Configurations

Written for users who don't know what client-server means.

- **Chapter 1** explains that MSCC is always one client and three servers, even on one PC. It uses
  a picture and a plain comparison: the client is the ship's bridge, the servers are the engine
  room. It says what each of the three servers does.
- **Chapters 2 and 3** describe the five setups. Each has a diagram, what runs where, the client
  settings, audio, digital programs, and which guide to read.

The five setups:

1. Windows local (client and servers on one Windows PC)
2. Windows client on PC 1, Windows servers on PC 2
3. Windows client, Pi / Linux servers (my station)
4. Pi / Linux local
5. Pi / Linux client on machine 1, Pi / Linux servers on machine 2

The main difference it explains: with a Windows host, the client looks after the COM port (SETTINGS
→ COM Port). With a Pi / Linux host, the host owns everything around the radio. MSCC Init sets up
the COM ports and audio, WSJT-X runs on the Pi, and you reach it with VNC.

**Please check setups 1 and 2** (sections 3.1 and 3.2, and those two columns in the chapter 4 table).

**Question:** see "Windows host and the one-client limit" under the Remote guide below.

## Document 2: Remote Operation (Windows)

A rewrite of Remote Release 0, for client 10.31.2. It now covers Windows client and Windows host
only. Remote with a Pi or Linux host works differently and will get its own guide.

These changes from Release 0 were checked in the client and server code:

- Phones / Digital are chosen inside the Remote Audio window. The Phones and Digital buttons on the
  main window turn Remote off. (Release 0 said both could be on together.)
- Remote is off each time MSCC starts.
- Only one client at a time. ms-sdr refuses a second client until the first presses Stop, or about
  45 s after it drops off. This is in the Windows ms-sdr too.
- In Remote Digital, the client's SETTINGS → COM Port is the CAT port.
- Closing the Remote Audio window keeps the audio running.

Questions:

1. **Windows host and the one-client limit.** The Windows guide (4.1) says Stop, or closing the
   window, also stops the servers that copy of MSCC started. And only one client can connect. So
   how does a Windows host keep its servers running for a remote client? Please give the exact
   steps. The Windows guide, Appendix B, mentions MSCC-Remote.exe, Start-MsccServers.bat and
   Install-MsccServers-AtBoot.bat: is one of those the answer?
2. **What does a refused second client show?** ms-sdr logs "SESSION IN USE - SINGLE CLIENT ONLY".
   Does the client show a message, or does it just not connect? The troubleshooting chapter needs it.
3. **Mic level in remote SSB.** The guide says to set Mic vol so the ALC meter moves on voice peaks
   but doesn't sit at the top. Is that right with your 0.45 × drive voice ceiling (trans 3.152)?
4. Should the guide tell users to give the host a fixed address in their router?
5. Pictures: chapter 3 shows client 9.27.0 and needs retaking with 10.31.2. An `ipconfig` picture
   is also needed. In the WSJT-X Audio picture, check that In and Out are the right way round.

## Document 3: Linux guide (Raspberry Pi and Ubuntu)

A rewrite of Linux Release 0, following the two rules above. The pictures are still from Release 0
(Ubuntu, MSCC UI 0.6.72).

- Install uses the Pi kit and its apt order, then log out and back in.
- MSCC Init 1.0.18, including the text wizard.
- The MSCC menu: Start, Stop, Status, Volume, Firmware Upload.
- Firmware upload from `/usr/share/mscc/firmware`.
- Calibration is stored in the radio.
- The new noise blanker and noise reduction, explained in plain words.
- Pi 4 advice: use a HAT or the headphone jack for phones, not a USB headset next to the radio.
- Appendix B: the Pi as host for a Windows client.

**Mistakes found in Release 0** (fixed in the new draft):

- **WSJT-X audio was the wrong way round.** Release 0 Appendix F said Input VirtualB.monitor and
  Output VirtualA, which are the servers' ends. The correct settings are **Input
  VirtualA.monitor, Output VirtualB**, as in the Pi install guide and the MSCC Volume package
  description.
- Appendix G (Pi install) ran `install-mscc.sh` and listed proficio-flash-tools. The Pi kit has no
  `install-mscc.sh`, and proficio-flash-tools was replaced in mscc 1.0.69.
- "Save settings parks TX IQ and QRP calibration on this computer" is out of date. They are now
  stored in the radio.

Your items in the Linux guide (Appendix C):

1. Bring the Ubuntu kit in line with the Pi kit (rule 2).
2. MSCC UI: the NB threshold scale, the pulse width in µs and the ALC switch (rule 1). Also the S/W
   window: Windows has SMOOTHING, BASELINE, AUTO SNAP, PEAK MARKER, TIME DISPLAY, TIME MARKER and
   Climbing.
3. **Firmware versions.** The guide's table uses the Windows guide's version numbers. The Linux
   package mscc-firmware 1.0.2 has older files for some radios (for example Proficio MKII PTT
   20261022 vs 20261031 on Windows). Which version is in each file, and can the Linux firmware
   package be brought up to date?
4. **Reset to factory** (TX IQ RESET ALL) still doesn't clear the values stored in the radio. This
   is the same open item as in the Windows guide.
5. **ALC target** for voice and digital. This is the same question as in the Windows guide.

Thanks, Ron
