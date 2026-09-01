# windows-work-tree

**Active Windows Multus MSCC stack** — GUI and servers in one place for Stew’s day-to-day work.

See also: root [`../README.md`](../README.md) for the full workspace map and dual-sync model.

---

## Contents

| Path | Role |
|------|------|
| **`mscc-mscc/mscc-new/`** | **MSCC.Wpf** client + **MSCC.Core** (UDP protocol shared with Avalonia) |
| **`ms-sdr-MKII/`** | Windows ms-sdr (USB, UDP hub, appliance/headless when ported) |
| **`SDRcore-recv/`** | Windows receive DSP / spectrum |
| **`SDRcore-trans/`** | Windows transmit DSP |
| **`mscc-init/`** | Init / setup helper sources |

Typical deploy: build client → `C:\mscc-net9\` (post-build copy). Servers also land under `C:\mscc-net9\` when built.

---

## Parallel Linux servers (keep in sync)

| This tree (Windows) | Linux counterpart |
|---------------------|-------------------|
| `ms-sdr-MKII/` | `../Linux-work-tree/ms-sdr-linux/` |
| `SDRcore-recv/` | `../Linux-work-tree/SDRcore-recv-linux/` |
| `SDRcore-trans/` | `../Linux-work-tree/SDRcore-trans-linux/` |

Protocol or behavior changes here should be ported to Linux (or listed for Ron) so Pi and Windows appliances stay compatible with the same UI.

---

## Parallel Linux UI

| This tree | Linux UI |
|-----------|----------|
| `mscc-mscc/mscc-new/` (WPF) | `../Avalonia-Migration/` (Avalonia) |

**MSCC.Core** lives here and is the **shared wire library**. Avalonia project-references this Core so opcode/API work is done once when possible.

---

## Client docs

| Doc | Location |
|-----|----------|
| WPF overview | `mscc-mscc/mscc-new/README.md` |
| User manual | `mscc-mscc/mscc-new/docs/MSCC-WPF-User-Manual.docx` |
| Keyer memory UDP behavior | `mscc-mscc/mscc-new/docs/KEYER-MEMORY-GUI-UDP-BEHAVIOR.md` |

---

## Who uses what

- **Stew:** primary working tree for WPF + Windows servers.  
- **Ron:** usually does **not** work here; he runs Linux servers on Pi and may connect with **WPF** as the Windows GUI (not Avalonia).
