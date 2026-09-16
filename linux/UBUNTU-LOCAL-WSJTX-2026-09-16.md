# Ubuntu local WSJT-X (2026-09-16)

**Host:** `stew-HP-Notebook`, Ubuntu 26.04 x86_64  
**Status:** Local MSCC + WSJT-X **TUNE** puts out good RF; SA looks clean. Same recipe as the Pi note.

For Pi: [`../rpi/PI-LOCAL-WSJTX-2026-09-16.md`](../rpi/PI-LOCAL-WSJTX-2026-09-16.md).  
Win11 WPF already has recent-host combo + ALC; do **not** copy Ubuntu `~/.local/mscc` INIs onto Windows.

---

## What was wrong on this laptop

| Symptom | Cause |
|---------|--------|
| Weak / dirty WSJT-X TUNE | VirtualA at **96 kHz** + **A↔B loop**; old `mscc-virtual-audio` (CRLF, never ran) |
| ALC needle parked | Avalonia `ALC_ON=0` → trans `G_Do_ALC: 0` |
| CAT hard to find in WSJT-X | No `/dev/ttyUSB11` until udev `tnt1` → `ttyUSB11` |
| UI Host IP vanished after Connect | ComboBox `Clear()` of recent hosts set `Host=""` (black empty box) |

---

## What works here now

```
WSJT-X CAT  →  /dev/ttyUSB11  (= /dev/tnt1)  ↔  /dev/tnt0  ←  ms-sdr
WSJT-X TX   →  VirtualB @ 48 kHz  →  VirtualB.monitor  →  sdrcore-trans
sdrcore-recv 96k I/Q --resample--> VirtualA @ 48 kHz → VirtualA.monitor → WSJT-X RX
```

| Piece | Setting |
|-------|---------|
| UI | Avalonia **0.6.53** (`installers/linux/mscc-ui_0.6.53_amd64.deb`) |
| Servers | `$HOME/mscc` from **`linux/`** (recv distrust 96 kHz) |
| VirtualA/B | **48 kHz**, **no A↔B** (`linux/helpers/mscc-virtual-audio.sh`) |
| CAT (ms-sdr) | `/dev/tnt0` |
| CAT (WSJT-X) | **`/dev/ttyUSB11`** or `/dev/tnt1`, 9600, PTT **CAT** |
| Audio in / out | `VirtualA.monitor` / `VirtualB` |
| Tx audio | **~1500 Hz** (not 3119) |
| AMP | **On** (QRO) |
| ALC | **On** (UI button selected) |
| Remote | **Off** while local WSJT-X holds CAT |

Host IP: black box, **white** text, stays after Connect. Dropdown on the right is recent IPs only (does not wipe Host).

---

## For the other builds

**Raspberry Pi (Ron)**  
Same local recipe. Kit: `installers/rpi/` (`mscc_1.0.43_arm64.deb`, `mscc-ui_0.6.49_arm64.deb` until you rebuild arm64 UI to 0.6.53). Do not merge Ubuntu `$HOME/mscc` ELFs into `rpi/mscc-binaries/`.

**Win11 (Build / WPF)**  
WPF already has host history + ALC. Local WSJT-X uses VAC + com0com, not tty0tty. Remote Digital / CAT is still the next team step; do not enable Remote on the radio host while local WSJT-X is using CAT.

**Next (this laptop)**  
Remote WSJT-X: [`../mscc-remote-audio/REMOTE-WSJTX-CHECKLIST.md`](../mscc-remote-audio/REMOTE-WSJTX-CHECKLIST.md).

Rebuild UI: `./linux-build/mscc-ui-x64.sh` then `./linux-build/build-mscc-ui-deb-amd64.sh` (use `pwd -P` — `~/mscc-station` symlink vs `Documents/GitHub` breaks XAML publish).
