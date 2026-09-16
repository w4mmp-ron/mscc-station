# Remote WSJT-X — checklist (pick up here)

**Date:** 2026-09-16  
**Stopped:** local Ubuntu + Pi WSJT-X TUNE are good. Next is **remote** CAT + digital audio.  
**Do not mix** local WSJT-X CAT with Remote on the **same** box.

Related: [REMOTE-AUDIO-PUNCHLIST.md](REMOTE-AUDIO-PUNCHLIST.md), [linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md](../linux/UBUNTU-LOCAL-WSJTX-2026-09-16.md), [rpi/PI-LOCAL-WSJTX-2026-09-16.md](../rpi/PI-LOCAL-WSJTX-2026-09-16.md).

---

## How it works (read this first)

WSJT-X does **not** talk to the MSCC UI for CAT or audio when **local**. It talks to the **servers** (tty0tty + Pulse VirtualA/B). The UI only sets Digital / AMP / ALC and the panadapter.

| Mode | Connect | Remote | Who owns CAT + VirtualA/B | WSJT-X runs on |
|------|---------|--------|---------------------------|----------------|
| **Local** | `127.0.0.1` | **off** | **Servers** (`mscc start`) | Same PC as the radio |
| **Remote** | Pi or Win IP | **on** | **Avalonia / WPF** (Kenwood proxy + AF) | **Operator PC only** |

WSJT-X always uses **devices on the machine where it runs**. It cannot open the Pi’s `/dev/ttyUSB11` from Ubuntu.

**SETTINGS today** = **this PC** (`~/.local/mscc/`). That is what WSJT-X on **this** laptop should use. It is not the Pi’s CAT. A “radio host” block would need a new server status opcode — not done.

Most operators: one WSJT-X, one PC, flip Connect host + Remote. Stew tests every combo; don’t optimize the UI for that.

---

## Same WSJT-X profile (operator laptop)

If WSJT-X stays on **this Ubuntu PC** for local **and** remote, keep:

| WSJT-X | Value |
|--------|--------|
| Rig | Kenwood TS-2000 |
| Serial | `/dev/ttyUSB11` (or `/dev/tnt1`) |
| PTT | CAT |
| Baud | 9600 |
| Audio in | `VirtualA.monitor` |
| Audio out | `VirtualB` |
| Tx audio | ~**1500 Hz** |

Switching local → remote is **MSCC only**: Connect `127.0.0.1` vs Pi `100.x`, Remote off vs on. **Stop local servers** (`mscc stop`) before Remote on, or CAT/VirtualA fight.

If WSJT-X runs **on the Pi** for shack use, that is a **second** profile (Pi ttyUSB11 / Virtual*). Expect different settings.

---

## First remote path (tomorrow)

**Ubuntu Avalonia + WSJT-X = operator. Raspberry Pi = radio.**  
Not Ubuntu → Windows yet (extra radio OS). Windows client → Ubuntu radio already TX’d RF; CAT is the remaining client problem.

### Radio (Pi)

- [ ] `mscc start` (or kit). **No** local WSJT-X, **no** MSCC UI Remote.
- [ ] AMP on, Digital is the boot/local path (`AUDIO_DEVICE` 0/1 only).
- [ ] Tailscale up; note Pi `100.x`.
- [ ] Do not point anything at Pi `/dev/ttyUSB11` from the laptop.

### Operator (Ubuntu laptop)

- [ ] `mscc stop` — this PC is **not** the radio.
- [ ] VirtualA/B **48 kHz**, no A↔B (`mscc-virtual-audio`).
- [ ] tty0tty: `/dev/ttyUSB11` exists **here**.
- [ ] Avalonia Connect → **Pi** `100.x:8888`. Host IP stays visible (0.6.53).
- [ ] **Remote on**, Audio **Digital** (`0x9B`=3).
- [ ] SETTINGS “This PC” names = what WSJT-X uses (not the Pi).
- [ ] WSJT-X: same table as above. TUNE. ALC just into yellow on the **radio**.
- [ ] Logs: Ubuntu Avalonia Remote AF + CAT; Pi `sdrcore-trans.log` `REMOTE_DIGITAL`, `pkt ok`, `G_mic_volume` not 0.

### Then (later)

- [ ] Ubuntu client → Windows servers (same client recipe; Windows firewall / servers stay up).
- [ ] Optional: SETTINGS “Radio host” line (server-reported CAT / VirtualA names) — new opcode, not required for WSJT-X.

---

## Do not

- Remote **on** while `mscc start` is running on the **same** PC.
- WSJT-X serial = Pi device when WSJT-X is on Ubuntu.
- Copy `$HOME/.local/mscc` or `$HOME/mscc` ELFs to `rpi/` or Windows.
- Edit `rpi/` for Ubuntu-only CAT/audio experiments.
