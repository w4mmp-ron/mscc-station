# Raspberry Pi OS (64-bit) — install

**This is not the Ubuntu kit.** Use [../linux/](../linux/) on an x86_64 PC.

Need: Pi **4 or 5** (or a **Zero 2 W**, see below), 64-bit Raspberry Pi OS (desktop recommended).

## Download

The packages are **not in git**. Download them from the **Releases** page:
<https://github.com/w4mmp-ron/mscc-station/releases> (newest release at the top; older
releases stay there for roll-back). Put the `.deb` files in one folder on the Pi.

| Package | Version | |
|---|---|---|
| `mscc` | 1.0.110 | required: servers (ms-sdr 3.217, recv 3.158, trans 3.165), PortAudio, MSCC Init, MSCC Volume, PSoC + Black Pill + keyer firmware and flash tools (window "Black Pill Firmware Update" since 1.0.85) |
| `mscc-avalonia` | 1.7.08 | optional: MSCC Avalonia client on the Pi (not needed with the Windows client) |
| `mscc` (Zero) | 1.0.110+zero1 | Pi Zero 2 W only: use `mscc_*+zero*_arm64.deb` instead of the normal `mscc` |

```bash
cd /path/to/the/downloaded/debs
sudo apt install -y ./mscc_*_arm64.deb             # everything for the radio
sudo apt install -y ./mscc-avalonia_*_arm64.deb    # optional
```

**Pi Zero 2 W:** install `mscc_*+zero*_arm64.deb` instead of the normal package. Its one USB
line can't run a USB sound card next to the radio, so the Zero package has no local operator
audio: it adds two dummy sound devices (OperatorNull, OperatorNullMic) and always uses them.
Use the client's Digital or Remote audio; Phones is greyed out. Log out and back in (or reboot)
after the first install so the dummy devices appear.

**Remote audio over WiFi:** a WiFi link can hold packets back about 0.1 s, so the transmit
power dips briefly. Give the remote mic a bigger buffer in `~/.local/mscc/remote-mic.ini`:

    BUFFER_MS=200

then `mscc stop` / `mscc start` (default 100, range 50 to 300; 200 adds 0.1 s of transmit
delay, fine for FT8). The Zero package sets 200 for you. A wired connection needs none of
this; with both connected, give the client the wired IP address (the `.local` name may give the WiFi one).

Since mscc 1.0.79 the old separate packages (`mscc-portaudio`, `mscc-init`, `mscc-volume`,
`mscc-firmware`) are part of `mscc`; installing it removes them. Installing `mscc` asks before it
stops running servers (answer `y`).

Then: log out/in (first install) → **MSCC Init** → **MSCC Start** → connect the client
(Windows client to the Pi's IP, or MSCC Avalonia on the Pi).

## Firmware upload

- PSoC radios: BOOT jumper → Morse LOADER → menu **MSCC → Firmware Upload**
  (`psoc-bootloader.gui`), or `psoc-bootloader file.cyacd`.
  PSoC files: `/usr/share/mscc/firmware/<Radio>/` (same versions as the Windows installer).
- Black Pill (25 MHz board), files in `/usr/share/mscc/firmware/black-pill/`:
  menu **MSCC → Black Pill Firmware Update**, or
  `mscc stop; stm32-app-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-<ver>-app.bin`
  (USB, app) or `sudo stm32-bootloader-flash /usr/share/mscc/firmware/black-pill/BlackPill-25MHz-<ver>-boot-app.bin` (ST-Link).
- Keyer (PIC on the daughter board; Black Pill 3.16 or later, or the MKII PSoC firmware in the
  Addendum): stop the servers (`mscc stop`), then
  menu **MSCC → Keyer Firmware Update** (`keyer-flash.gui`): Load File → Program. Or
  `keyer-flash /usr/share/mscc/firmware/keyer/Keyer-MCU-app-20261126.hex`. Settings and CQ
  memories are kept. **Power-cycle the radio afterwards** (no RF until then). The `Keyer-MCU-full-` file is for the PICkit only (bootloader + app).

## Stored CW carrier (Black Pill 3.25 or later, optional)

```bash
mscc stop
echo 'CW_CARRIER=1;' >> ~/.local/mscc/mscc.ini     # once; to turn it off, change the 1 to 0
mscc start
```

With `CW_CARRIER=1;` the radio makes the CW carrier itself from a 1 ms block the servers load
into it, and the servers stop the I/Q audio streams while they are not needed (receive pauses
during voice, digital and TUNE transmit when Monitor is off). Off by default: without the line, or
with `CW_CARRIER=0;`, the servers work as before. The servers read it only at start, so restart
them after a change. A PSoC radio, or a Black Pill older than 3.25, ignores it (ms-sdr logs "off").
MSCC Init keeps the line when it rewrites `mscc.ini`. Pi servers only (not the Windows or Ubuntu
servers).

Full how-to: [`../../rpi/mscc-deb/INSTALL-FOR-PI.md`](../../rpi/mscc-deb/INSTALL-FOR-PI.md).
