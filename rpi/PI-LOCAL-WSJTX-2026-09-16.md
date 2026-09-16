# Pi local WSJT-X: CAT + digital audio + ALC (2026-09-16)

**Host:** Raspberry Pi 5 (`raspberrypi`, Pi OS 64-bit, kernel `6.18.50+rpt-rpi-2712`)  
**Operator:** Ron  
**Status:** Local WSJT-X TUNE is **full power and clean** on the SA (Pi).  
**Ubuntu laptop (Stew, 2026-09-16):** same local path confirmed — good TUNE power, clean SA. 48 kHz VirtualA/B (no A↔B), recv distrust 96 kHz, `/dev/ttyUSB11`, ALC on, Avalonia **0.6.53**. Detail: [`../linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md`](../linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md). Remote still next.

This note is for Ubuntu (`linux/`) and Win11 (`mscc-ui/windows-work-tree`) so nobody re-debugs a solved local path.

Related: [`../mscc-remote-audio/LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md`](../mscc-remote-audio/LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md), [`../mscc-remote-audio/STEW-REMOTE-AUDIO.md`](../mscc-remote-audio/STEW-REMOTE-AUDIO.md).

---

## Working local topology (this Pi)

```
WSJT-X  --CAT-->  /dev/ttyUSB11  (= /dev/tnt1)  <-->  /dev/tnt0  <--  ms-sdr
WSJT-X  TX audio --> VirtualB (48 kHz)
                    VirtualB.monitor --> sdrcore-trans (Oboe 48k → 96k I/Q)
sdrcore-recv 96k I/Q --Oboe--> VirtualA (48 kHz) --> VirtualA.monitor --> WSJT-X RX
```

| Piece | Setting that works |
|-------|-------------------|
| Rig | Kenwood TS-2000 |
| CAT serial | **`/dev/ttyUSB11`** or `/dev/tnt1` (WSJT-X end) |
| Baud | 9600, handshake none |
| PTT | **CAT** (not VOX; PTY has no RTS) |
| Audio in | `VirtualA.monitor` |
| Audio out | `VirtualB` |
| Tx audio freq | **~1500 Hz** (USB TX filter is 75–2700 Hz; 3119 Hz is outside) |
| AMP | **On** (QRO). `CMD_SET_PA_BYPASS 0` means QRP / milliwatts |
| ALC | **On** (`CMD_SET_ALC_MULTIPLIER` non-zero). Meter + limiter |
| Digital mic | Saved so ALC is just into the yellow |

ms-sdr **must** hold `/dev/tnt0` (`~/ .local/mscc/comm-port.ini`). WSJT-X holds the other end. Never open the same tnt node from both.

---

## What was broken, and the fix

### 1. CAT — WSJT-X could not see the port

**Symptom:** WSJT-X Radio dropdown only had **USB** and **`/dev/ttyAMA10`**. Test CAT failed.

**Cause:**

- ms-sdr CAT was a **PTY** (`~/ms-sdr-cat` → `/dev/pts/N`). Qt/WSJT-X only lists kernel TTYs, not PTYs.
- Intended path is **tty0tty** (`/dev/tnt0` ↔ `/dev/tnt1`).
- Kernel had been upgraded **6.18.39 → 6.18.50**. Boot log: `Failed to find module 'tty0tty'`. Module existed only for the old kernel.
- udev rules left `/dev/tnt*` as `root:root 600` until fixed; WSJT-X would not list/open them.
- WSJT-X.ini had `CATSerialPort=/dev/ttyAMA10` (Pi debug UART) and `PTTport=/dev/tnt1` with PTT=VOX.

**Fix (live + packaging):**

- Rebuild/load tty0tty for the **running** kernel (`rpi/mscc-deb/packaging/DEBIAN/postinst` already does this; headers must match `uname -r`).
- udev: `GROUP=dialout MODE=0660` and **`SYMLINK+=ttyUSB10/ttyUSB11`** so WSJT-X’s combo lists a ttyUSB name.
  - Files: `linux/tty0tty-master/module/99-tty0tty.rules`, `rpi/tty0tty-master/module/99-tty0tty.rules`, `rpi/mscc-deb/packaging/usr/share/mscc/tty0tty/module/99-tty0tty.rules`
- `comm-port.ini`: `COMM_PORT_NAME=/dev/tnt0`, `PIN=1`.
- After a kernel bump: `cd /usr/share/mscc/tty0tty/module && sudo make && sudo make install` (or reinstall the mscc deb).

Kenwood smoke: `ID;` → `ID019;`. hamlib: `rigctl -m 2014 -r /dev/tnt1 -s 9600 f`.

**Parser gaps still in ms-sdr** (not blocking Test CAT / CAT PTT via `TX;`):

- hamlib often sends **`TX0;`** — only exact `TX;` is handled.
- `TX;` / `RX;` call `Set_PTT` but **do not write a reply**.
- `AI0;` discarded.

### 2. TX — CAT keyed, almost no RF

**Symptom:** WSJT-X TUNE put the radio in TX; SA showed little power.

WSJT-X TUNE is **USB PTT + tone**, not MSCC TUN (`tune_modulate`). SSB with no in-band audio = suppressed carrier.

**Causes stacked:**

1. TxFreq **3119 Hz** vs USB TX high cut **2700 Hz** (default). Move Tx marker to ~1500 Hz, or widen high cut.
2. **AMP off** / QRP. Opcode is inverted in trans: `CMD_SET_PA_BYPASS` **0** → `G_QRP_mode=1`. Turn **AMP on** in the UI (red).
3. VirtualB default **60% (−13 dB)** on the TX cable. Defaults are now **100%**.
4. Digital mic / WSJT Pwr too low.

### 3. Spikes every ~375 Hz on TX SA and WSJT-X waterfall

**375 Hz = 48000 / 128** (PipeWire quantum on a 48 kHz clock).

**Causes:**

1. **`pw-link` A↔B bidirectional.** Recv 96 kHz AF was mixed into VirtualB and looped back. That comb was on **RX waterfall and TX RF**.
2. **VirtualA created at 96 kHz** while Pi PipeWire `clock.allowed-rates = [ 48000 ]`. Format 96k, graph 48k, quantum 128.
3. **recv `pick_play_sample_rate`** trusted Pulse `Pa_IsFormatSupported(96000)` and opened VirtualA at 96 kHz with `resample=0`. Pulse then resampled anyway.

**Fix:**

- **No A↔B `pw-link`.** Recv owns VirtualA; WSJT-X/trans own VirtualB.
  - `linux/helpers/mscc-virtual-audio.sh`
  - `rpi/mscc-deb/packaging/usr/share/mscc/bin/mscc-virtual-audio.sh`
- VirtualA/B (and `_TX`) created at **48000**.
- recv: same **distrust 96k if device default ≠ 96k** already used on trans mic.
  - `linux/SDRcore-recv-linux/sources/main.c`
  - `rpi/SDRcore-recv-linux/sources/main.c`  
  Log: `pick_play_sample_rate. distrust 96k probe (default=48000)` then `play=48000 resample=1`.

**Rebuild recv on the Pi** (already done live into `$HOME/mscc/sdrcore-recv`):

```bash
mscc stop
cd ~/src/mscc-station/rpi/SDRcore-recv-linux && make clean && make
# binary lands in $HOME/mscc/sdrcore-recv
mscc start
mscc-virtual-audio   # recreate 48 kHz sinks, no loop
```

### 4. ALC meter dead; easy overdrive

**Symptom:** Needle parked at 0; WSJT-X could splatter if drive was high.

**Cause:** Avalonia `ALC_ON=0` on connect → `CMD_SET_ALC_MULTIPLIER 0` → trans `G_Do_ALC=FALSE`. That disables **both** `doALC()` and `ALC_Meter_thread`. The RX/TX tab **ALC** button was a **disabled stub**.

WPF already has a working ALC toggle. Avalonia did not.

**Fix (Avalonia):**

- Wire **ALC** button (`ToggleAlcCommand` / `SetAlcOnAsync`).
- Default `AlcOn = true`.
- Persist `ALC_ON=1` in `~/.config/MSCC/mscc-avalonia.ini`.

On this Pi, trans was also kicked with UDP `0x23` data `1` to `:9200` so the limiter ran immediately.

**How to set drive:** AMP on, ALC on, WSJT-X TUNE, raise D Mic / Pwr until ALC is **just into the yellow**, then a hair back. That is the saved local point.

**Still twitchy:** D Mic 0–100 goes through ~16 dB analog boost (`framesToComplex` stereo `gain = 6.324`). Useful range is a small slice. Flattening digital gain is **not** in this drop — do it after remote.

### 5. WSJT-X menu icon

Debian `wsjtx.desktop` is `Categories=AudioVideo;Audio;HamRadio;` → **Sounds and Video**.

Override: `Categories=X-MSCC;HamRadio;` (same as MSCC Start/UI).

- Live: `~/.local/share/applications/wsjtx.desktop` and `/usr/local/share/applications/wsjtx.desktop`
- Packaging: `rpi/mscc-deb/packaging/usr/share/mscc/desktop/wsjtx.desktop` + `postinst` copy to `/usr/local/share/applications/`

---

## Files touched in this repo

| Path | Change |
|------|--------|
| `linux/SDRcore-recv-linux/sources/main.c` | Distrust Pulse 96 kHz play |
| `rpi/SDRcore-recv-linux/sources/main.c` | Same |
| `linux/helpers/mscc-virtual-audio.sh` | 48 kHz sinks, 100% default, **no A↔B links** |
| `rpi/mscc-deb/packaging/usr/share/mscc/bin/mscc-virtual-audio.sh` | Same + keep Volume GUI restore |
| `linux/tty0tty-master/module/99-tty0tty.rules` | dialout + ttyUSB10/11 |
| `rpi/tty0tty-master/module/99-tty0tty.rules` | Same |
| `rpi/mscc-deb/packaging/usr/share/mscc/tty0tty/module/99-tty0tty.rules` | Same |
| `rpi/mscc-deb/packaging/usr/share/mscc/desktop/wsjtx.desktop` | MSCC menu |
| `rpi/mscc-deb/packaging/DEBIAN/postinst` | Install WSJT override; chmod udev 644 |
| `rpi/mscc-deb/packaging/DEBIAN/postrm` | Remove WSJT override on purge |
| `linux/helpers/wsjtx-mscc.desktop` | Ubuntu/helper copy |
| `mscc-ui/Avalonia-Migration/.../MainViewModel.cs` | ALC toggle + default on |
| `mscc-ui/Avalonia-Migration/.../MainWindow.axaml` | ALC button live |
| `mscc-ui/Avalonia-Migration/.../ClientSettingsStore.cs` | `AlcOn` default true |

**Not rebuilt into a new `.deb` yet.** Live Pi has the recv binary and virtual-audio script applied by hand. Next Pi kit: rebuild recv, bump `mscc` Version, `rpi/mscc-deb/build-deb.sh`, drop into `installers/rpi/`. Next UI kit: rebuild Avalonia 0.6.50 (or next) for arm64 + amd64.

---

## Remote (not fixed — team)

Local CAT + VirtualA/B is **not** the remote path.

Remote is:

- Client (Win11 WPF or Linux Avalonia) **Remote Digital** (`CMD_SET_AUDIO_DEVICE` **3**)
- TX audio: MSA1 UDP **9101** → `sdrcore-trans` `remote_mic`
- RX audio: UDP **9100** from recv
- CAT: client Kenwood engine → freq/PTT over **8888**, **or** a COM/PTY on the **client** machine — **not** `~/ms-sdr-cat` / tnt on the Pi

**Do not** enable Remote AF **on the Pi** while local WSJT-X holds `/dev/tnt1`. Avalonia `KenwoodCatPort` will try to open the same CAT slave.

Known remote pain (from `mscc-remote-audio/`):

- WSJT-X TUNE = CAT PTT + VAC, not MSCC TUN. Silence on 9101 = TX, no RF.
- Packets on Tailscale ≠ trans using them (`G_audio_mode` 2/3, `remote_mic pkt ok=`).
- Windows line-level vs 16 dB analog boost (`framesToComplex` skips boost for `REMOTE_DIGITAL_AUDIO` only).
- CAT “sometimes works”: client serial vs server Kenwood; tty0tty vs PTY; two engines on one port.

**Suggested split:**

| Person | Next |
|--------|------|
| **Ron** | Keep Pi appliance stable (tnt0, 48 kHz Virtual*, ALC on, AMP on). Capture trans log around a **failed** remote TUNE: `CMD_SET_AUDIO_DEVICE`, `remote_mic pkt ok`, `G_mic_volume`, `opmode`. |
| **Stew** | WPF/Avalonia Remote Digital: opcode 3, VAC device, TX host:9101, CAT on/off vs server. Confirm `ALC_ON` / `SetAlcOnAsync` on connect. |
| **Both** | One matrix: local WSJT on Pi vs WSJT on Win11 vs WSJT on Ubuntu, CAT vs Remote CAT, Digital vs Remote Digital. |

---

## Pi operator checklist (local)

1. `ls -l /dev/tnt0 /dev/tnt1 /dev/ttyUSB10 /dev/ttyUSB11` — dialout, 660.
2. `ms-sdr` log: `open_comm_port -> OK port=/dev/tnt0`.
3. WSJT-X: TS-2000, `/dev/ttyUSB11`, 9600, PTT CAT, audio VirtualA.monitor / VirtualB, Tx ~1500 Hz.
4. AMP **on**, ALC **on**, Digital audio.
5. `pw-link -l` — **no** VirtualA↔VirtualB. Recv → VirtualA, trans ← VirtualB.monitor.
6. `pw-top`: VirtualA `F32P 2 48000` (not format 96000 on rate 48000).
