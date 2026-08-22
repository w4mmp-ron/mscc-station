# Proficio firmware update — instructions for Stew

**Audience:** Stew (daughter board / station bring-up)  
**Applies to:** MKII-PTT, MKII-ATU, and Legacy Proficio  
**Date:** 2026-08-22  

There are **two flash regions** on the PSoC. Do not mix them up.

| Region | File type | How often | Tool |
|--------|-----------|-----------|------|
| **Application** (radio firmware) | `.cyacd` (preferred) or app `.hex` | Normal updates | USB bootloader host, or MiniProg3 |
| **Bootloader** (tiny “LOADER” program) | Bootloader project `.hex` only | Rarely | **MiniProg3 / PSoC Programmer only** |

Flashing a **`.cyacd` never replaces the bootloader**. If USB “enter bootloader” misbehaves, the on-chip bootloader may be outdated — see §4.

---

## 1. What you need

### Files (from Ron / `mscc-station` repo)

| Item | Where |
|------|--------|
| App `.cyacd` | `Release-Proficio-MKII-PTT/Release/` (or ATU / Legacy `Release/` after Creator build + `copy-release`) |
| Linux USB host | `psoc-usb-bootload-linux/` → build binary named **`bootloader`** |
| Windows USB host | `bootloader.exe` + `Bootloader_Utils.dll` + `CyUSB.dll` (e.g. Migration `utilities\`) |
| On-chip bootloader project | `bootloader/` (only if updating the LOADER itself) |

### USB IDs (check with `lsusb` on the Pi)

| Mode | VID:PID | What you see |
|------|---------|----------------|
| Running app | **`16c0:05dc`** | Product: **Proficio** |
| In bootloader | **`04b4:b71d`** | Cypress / **PSoC3 Bootloader**; Morse **LOADER** on LED |

---

## 2. Normal app update (most common)

Goal: load a new **Proficio application** `.cyacd` over USB.

### A. Put the radio into bootloader

**Option A1 — BOOT jumper (always works)**  
1. Power off (or unplug USB if that is how you power it).  
2. Install / short the **BOOT** jumper.  
3. Power on.  
4. LED should blink Morse **LOADER**.  
5. `lsusb` (or Windows Device Manager) should show **`04b4:b71d`**.

**Option A2 — USB command (no jumper)**  
Requires recent **app** firmware (`0x0E`) **and** updated **on-chip bootloader** (see §4).

On the Pi (ms-sdr **stopped** so it does not hold the USB device):

```bash
cd ~/psoc-usb-bootload-linux   # or wherever you built it
./bootloader --enter-bootloader
sleep 2
./bootloader --list            # expect 04b4:b71d
```

Windows: use a host that sends vendor OUT **`0x0E`**, or use the BOOT jumper.

### B. Program the `.cyacd`

**Linux (Pi):**

```bash
./bootloader /path/to/Proficio-MKII-PTT-YYYYMMDD.cyacd
```

Success message ends with: *device should reboot into the application.*  
Then `lsusb` should show **`16c0:05dc` Proficio** again (LOADER stops).

**Windows:**

```text
bootloader.exe
```

Select the `.cyacd`, connect to the HID bootloader device, program. Same end state: app `16c0:05dc`.

### C. Remove BOOT jumper

If you used the jumper, power off, **remove jumper**, power on for normal operation.

---

## 3. Soft-reboot application only (no firmware file)

Recent app firmware supports USB **`0x0F`** — restarts the app without entering LOADER.

```bash
# ms-sdr stopped
./bootloader --reboot-app
```

You may see a USB “Pipe error” message; that is often normal (device resets mid-transfer). Confirm with:

```bash
dmesg -w
# or
lsusb | grep -i 16c0
```

Expect a brief disconnect and Proficio `16c0:05dc` coming back.

---

## 4. Updating the on-chip bootloader (rare)

Only needed when bootloader **policy** or USB bootloader code changes (e.g. so `--enter-bootloader` stays in LOADER without the jumper).

1. Build Creator project: **`bootloader/bootloader.cydsn`**.  
2. Use **MiniProg3 + PSoC Programmer** (or Creator Program).  
3. Program the bootloader **`.hex`** (not a Proficio `.cyacd`).  
4. BOOT jumper is **optional** for MiniProg3 / SWD.  
5. Afterward, re-program the **application** (`.cyacd` or app `.hex`) if needed.

Until this is done once, **`--enter-bootloader` may reset and jump straight back to the app** (no `04b4:b71d`). Use the **BOOT jumper** for updates in that case.

---

## 5. Linux `bootloader` tool — quick reference

Build (on Pi):

```bash
sudo apt-get install -y build-essential libhidapi-dev libusb-1.0-0-dev
cd ~/psoc-usb-bootload-linux
make clean && make
./bootloader --help
```

Useful commands:

| Command | Meaning |
|---------|---------|
| `./bootloader --help` | Options + examples |
| `./bootloader --list` | HID devices (`04b4:b71d` when in LOADER) |
| `./bootloader --reboot-app` | Soft reset app (`0x0F`) — needs app `16c0:05dc` |
| `./bootloader --enter-bootloader` | Ask app to enter LOADER (`0x0E`) |
| `./bootloader -e -w 3 file.cyacd` | Enter LOADER, wait 3 s, then program |
| `./bootloader file.cyacd` | Program when already in LOADER |

Optional udev (avoid `sudo` for USB): copy `99-proficio-bootloader.rules` to `/etc/udev/rules.d/` and reload udev.

**Stop ms-sdr / MSCC servers** before `--reboot-app`, `--enter-bootloader`, or programming — they hold the Proficio USB device.

---

## 6. Which folder / which `.cyacd`

| Radio build | Creator / release tree |
|-------------|-------------------------|
| MKII with rear PTT | `Release-Proficio-MKII-PTT` |
| MKII ATU | `Release-Proficio-MKII-ATU` |
| Legacy Proficio | `Release-Proficio-Legacy` |

After a Creator build, use that tree’s **`copy-release.bat`** / `Release/` dated `.cyacd` (and matching `.hex` if you use MiniProg3 for the app).

---

## 7. Checklist

- [ ] Know whether you are updating **app** or **bootloader**  
- [ ] For app USB update: radio in **LOADER** (`04b4:b71d` or Morse LOADER)  
- [ ] ms-sdr stopped  
- [ ] Correct `.cyacd` for PTT / ATU / Legacy  
- [ ] After program: `16c0:05dc` Proficio, no LOADER blink  
- [ ] BOOT jumper removed for normal use  

---

## 8. Related docs in repo

| Doc | Content |
|-----|---------|
| `REBOOT-USB.txt` (this folder) | Opcode `0x0E` / `0x0F` notes |
| `psoc-usb-bootload-linux/README.md` | Linux host build/use |
| `bootloader/README.md` | On-chip bootloader launch policy |
| `docs/STEW-DAUGHTER-BOARD-PINOUT.md` (STM32 tree) | Pinout for STM daughter — separate from PSoC USB update |

Questions → Ron (W4MMP).
