# MSCC-Grok-Build — workspace map

Multus MSCC multi-platform workspace: **Windows WPF** (Stew), **Linux servers** (Ron / Pi), **Linux Avalonia UI**, **PIC keyer**, **PSoC radio firmware**.

Use this map when working across Linux and Windows so the right tree is edited and the parallel counterpart is updated intentionally.

---

## Top-level layout

| Folder | Role | Primary owner |
|--------|------|----------------|
| **`windows-work-tree/`** | Windows **UI + servers** (WPF, ms-sdr-MKII, recv, trans, init) | Stew |
| **`Linux-work-tree/`** | Linux **backend servers + packaging** (ms-sdr, recv, trans, debs, init-files) | Ron (Pi / terminal) |
| **`Avalonia-Migration/`** | Linux **GUI only** (Avalonia `mscc-ui` for Pi) | Stew (UI parity with WPF) |
| **`keyer-firmware/`** | PIC keyer work tree, bootloader, released hex | Stew / Ron (keyer) |
| **`PSoC-Firmware/`** | Proficio / Geminus / legacy PSoC work trees + release hex/cyacd | Stew / Ron (radio FW) |
| **`Backup-Refs/`** | Historical client builds and firmware references | Archive |

Root also holds **release installers** (e.g. `mscc-net9-R*-install.exe`), group docs, and punch lists.

---

## How the pieces fit (product stack)

```text
                    ┌─────────────────────┐
                    │  UI (operator PC)   │
                    │  WPF  or  Avalonia  │
                    └──────────┬──────────┘
                               │ UDP (opcodes)
                    ┌──────────▼──────────┐
                    │  ms-sdr (+ recv/trans)│
                    │  Windows  or  Linux  │
                    └──────────┬──────────┘
                               │ USB / I²C
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
        PSoC radio        PIC keyer         (optional CAT)
     (PSoC-Firmware)   (keyer-firmware)
```

Ron prefers **RPi for all terminal / server work** and **does not** use Avalonia day-to-day. Typical Ron path: **Linux servers** + optional **Windows WPF** remote client.  
Stew: **WPF + Windows servers** locally; **Avalonia** kept in sync for Pi UI when needed.

---

## Dual-sync model (the main rub)

Two independent parity tracks:

### A. Servers (must stay aligned for opcodes / behavior)

| Concern | Windows path | Linux path |
|---------|--------------|------------|
| Command hub | `windows-work-tree/ms-sdr-MKII/` | `Linux-work-tree/ms-sdr-linux/` |
| Receive DSP | `windows-work-tree/SDRcore-recv/` | `Linux-work-tree/SDRcore-recv-linux/` |
| Transmit DSP | `windows-work-tree/SDRcore-trans/` | `Linux-work-tree/SDRcore-trans-linux/` |

**When one side gets a protocol or headless change, the other needs a deliberate port** (or a punch list for the other owner).  
Examples: appliance startup, NR/AN bi-dir, keep-alive tags, keyer `0x9C` USB packing/pacing.

### B. UI (must stay aligned for operator features)

| Concern | Windows path | Linux path |
|---------|--------------|------------|
| GUI | `windows-work-tree/mscc-mscc/mscc-new/` (WPF) | `Avalonia-Migration/` (Avalonia) |
| Shared protocol library | `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` | **Referenced by Avalonia** (same Core project) |

**UI rule of thumb:** implement and stabilize on **WPF** when convenient, then port UX/behavior to **Avalonia** (or reverse if Pi-first).  
**Protocol rule of thumb:** opcode/API changes live in **MSCC.Core** under the Windows tree; Avalonia rebuilds against that Core so both UIs share one wire layer.

Cross-cutting features (e.g. keyer CQ memory) touch:

1. **keyer-firmware** (PIC)  
2. **PSoC-Firmware** (USB → I²C)  
3. **Servers** (Linux and/or Windows ms-sdr)  
4. **UI** (WPF and/or Avalonia via Core)

---

## Folder deep map

### `windows-work-tree/` — Windows all-in-one

| Subfolder | Contents |
|-----------|----------|
| `mscc-mscc/mscc-new/` | **MSCC.Wpf** + **MSCC.Core** (active Windows client) |
| `ms-sdr-MKII/` | Windows ms-sdr |
| `SDRcore-recv/`, `SDRcore-trans/` | Windows DSP cores |
| `mscc-init/` | Windows init helper sources |

Deploy target for client/servers is typically **`C:\mscc-net9`**.

### `Linux-work-tree/` — Pi servers & packaging

| Subfolder | Contents |
|-----------|----------|
| `ms-sdr-linux/` | Linux ms-sdr sources + README/RESUME |
| `SDRcore-recv-linux/`, `SDRcore-trans-linux/` | Linux DSP cores |
| `mscc-deb/` | Server package (`.deb`) + install docs |
| `mscc-binaries/` | Built server binaries for Pi |
| `mscc-init-files-linux/`, `mscc-init-gui/`, `mscc-portaudio/` | Config seed, init UI, audio package |
| `mscc-client/` | **Likely a stale/old WPF+Core snapshot** (see below) — not the active Linux UI |

### `Avalonia-Migration/` — Linux UI only

Avalonia client for RPi. Packages as `mscc-ui_*.deb`.  
Project reference: **MSCC.Core** in `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/`.

### `keyer-firmware/` — PIC keyer

| Subfolder | Contents |
|-----------|----------|
| `keyer-work-tree/` | PIC sources, KEYER-MEMORY docs, test scripts |
| `bootloader/` | Keyer-related bootloader (moved here from mixed trees) |
| `Release/` | Released keyer hex |

### `PSoC-Firmware/` — radio MCU

| Subfolder | Contents |
|-----------|----------|
| `PSoC-work-trees/` | Active trees (e.g. Proficio MKII-PTT, Geminus MKII, legacy) |
| `Releases/` | Shipped `.hex` / `.cyacd` by product line |

---

## About `Linux-work-tree/mscc-client`

This tree still looks like an older **WPF conversion (`mscc-new`)** copy (README still describes Windows WPF). It is **not** the Avalonia UI and **not** a server package.

**Working assumption:** leftover reference or an old drop for Ron — **not** the source of truth for either UI.

| Active | Path |
|--------|------|
| Windows UI | `windows-work-tree/mscc-mscc/mscc-new/` |
| Linux UI | `Avalonia-Migration/` |
| Shared Core (preferred) | `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` |

If `mscc-client` is only historical, rename to e.g. `mscc-client-ARCHIVE` or move under `Backup-Refs/` when convenient.

---

## Suggested workflow (dual platform)

### Server change (either OS)

1. Implement / verify on the **owner’s** side (Ron Linux or Stew Windows).  
2. Write a short punch list or note: file + behavior.  
3. Port the same logic to the **parallel** tree (paths table above).  
4. Smoke both stacks if possible (even if UI is only WPF against Linux ms-sdr).

### UI change

1. Prefer **WPF** for full operate UI, or Avalonia if Pi-first.  
2. If protocol changes, update **MSCC.Core** once under `windows-work-tree`.  
3. Rebuild **Avalonia** against that Core; port view/viewmodel differences.  
4. Keep docs (`KEYER-MEMORY-GUI-UDP-BEHAVIOR.md`, punch lists) next to the feature or under the owning firmware folder for protocol truth.

### Keyer / PSoC change

1. Edit under `keyer-firmware/` or `PSoC-Firmware/`.  
2. Note server dependencies (e.g. `0x9C` packing) for **both** ms-sdr trees.  
3. Note UI dependencies for **both** UIs via Core.

---

## Quick “where do I edit?”

| Task | Open |
|------|------|
| WPF buttons / tabs / CW memory UI | `windows-work-tree/mscc-mscc/mscc-new/` |
| Opcodes / UDP send-receive | `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` |
| Windows ms-sdr / appliance | `windows-work-tree/ms-sdr-MKII/` |
| Linux ms-sdr / headless | `Linux-work-tree/ms-sdr-linux/` |
| Avalonia / Pi GUI | `Avalonia-Migration/` |
| PIC CQ memory | `keyer-firmware/keyer-work-tree/` |
| Proficio / Geminus firmware | `PSoC-Firmware/PSoC-work-trees/` |
| Pi `.deb` packaging | `Linux-work-tree/mscc-deb/` |

---

## Optional hygiene (not required to work)

1. Archive or clearly label `Linux-work-tree/mscc-client` if obsolete.  
2. One **canonical** keyer protocol doc: prefer `keyer-firmware/keyer-work-tree/KEYER-MEMORY.md`; other copies link to it.  
3. Root `releases/` folder for installers if root gets crowded.  
4. After server ports, keep a one-line log in each side’s `RESUME.md` / punch list (“ported NR push from Linux on date”).

---

*Updated for folder reorg (Avalonia / windows-work-tree / Linux-work-tree / keyer-firmware / PSoC-Firmware).*
