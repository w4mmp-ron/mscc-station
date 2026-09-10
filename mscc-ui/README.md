# MSCC UI — workspace map

Multus MSCC multi-platform workspace: **Windows WPF** (Stew), **Linux servers** (Ron / Pi), **Linux Avalonia UI**, **PIC keyer**, **PSoC radio firmware**.

Use this map when working across Linux and Windows so the right tree is edited and the parallel counterpart is updated intentionally.

---

## Top-level layout

| Folder | Role | Primary owner |
|--------|------|----------------|
| **`windows-work-tree/`** | Windows **UI + servers** (WPF, ms-sdr-MKII, recv, trans, init) | Stew |
| **`../rpi/`** | Raspberry Pi **servers + packaging** (guide for Ubuntu) | Ron |
| **`../linux/`** | Ubuntu x86_64 **servers** (edit here, not `rpi/`) | Stew (laptop) |
| **`Avalonia-Migration/`** | Linux **GUI** (Avalonia `mscc-ui` — Pi arm64 and Ubuntu amd64 debs) | Stew |
| **`Release/avalonia/`** | UI/server `.deb` drop: `arm64/` (Pi) and `x86_64/` (Ubuntu) | |

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
     (Proficio-firmware)   (keyer/)
```

Ron prefers **RPi for all terminal / server work** and **does not** use Avalonia day-to-day. Typical Ron path: **Linux servers** + optional **Windows WPF** remote client.  
Stew: **WPF + Windows servers** locally; **Avalonia** kept in sync for Pi UI when needed.

---

## Dual-sync model (the main rub)

Two independent parity tracks:

### A. Servers (must stay aligned for opcodes / behavior)

| Concern | Windows path | Linux path |
|---------|--------------|------------|
| Command hub | `windows-work-tree/ms-sdr-MKII/` | `../rpi/ms-sdr-linux/` (Pi) and `../linux/ms-sdr-linux/` (Ubuntu) |
| Receive DSP | `windows-work-tree/SDRcore-recv/` | `../rpi/SDRcore-recv-linux/` / `../linux/SDRcore-recv-linux/` |
| Transmit DSP | `windows-work-tree/SDRcore-trans/` | `../rpi/SDRcore-trans-linux/` / `../linux/SDRcore-trans-linux/` |

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

1. **`keyer/`** (PIC)  
2. **`Proficio-firmware/`** (USB → I²C)  
3. **Servers** (`rpi/` and/or `linux/` and/or Windows)  
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

### Linux servers (repo root, not under `mscc-ui/`)

| Tree | Contents |
|------|----------|
| **`../rpi/`** | Ron’s Pi arm64 sources, `mscc-deb`, `pi-install` — **guide only** for Ubuntu work |
| **`../linux/`** | Ubuntu x86_64 copy — **edit here** for the laptop |

See [`../rpi/README.md`](../rpi/README.md) and [`../linux/README.md`](../linux/README.md).

### `Avalonia-Migration/` — Linux UI

Avalonia client. Pi package: `mscc-ui_*_arm64.deb`. Ubuntu package: `mscc-ui_*_amd64.deb` in `Release/avalonia/x86_64/`.  
Project reference: **MSCC.Core** in `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/`.

### PIC keyer / PSoC (repo root)

| Path | Contents |
|------|----------|
| `../keyer/` | PIC16F18326 sources, KEYER-MEMORY docs, hex |
| `../Proficio-firmware/` | Proficio MKII/Legacy PSoC + Creator bootloader |

---

## Active UI paths

| Active | Path |
|--------|------|
| Windows UI | `windows-work-tree/mscc-mscc/mscc-new/` |
| Linux UI | `Avalonia-Migration/` |
| Shared Core | `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` |

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

1. Edit under `../keyer/` or `../Proficio-firmware/`.  
2. Note server dependencies (e.g. `0x9C` packing) for **both** ms-sdr trees (`rpi/` and `linux/`).  
3. Note UI dependencies for **both** UIs via Core.

---

## Quick “where do I edit?”

| Task | Open |
|------|------|
| WPF buttons / tabs / CW memory UI | `windows-work-tree/mscc-mscc/mscc-new/` |
| Opcodes / UDP send-receive | `windows-work-tree/mscc-mscc/mscc-new/src/MSCC.Core/` |
| Windows ms-sdr / appliance | `windows-work-tree/ms-sdr-MKII/` |
| Linux ms-sdr (Pi) | `../rpi/ms-sdr-linux/` |
| Linux ms-sdr (Ubuntu) | `../linux/ms-sdr-linux/` |
| Avalonia / Linux GUI | `Avalonia-Migration/` |
| PIC CQ memory | `../keyer/` |
| Proficio firmware | `../Proficio-firmware/` |
| Pi `.deb` packaging | `../rpi/mscc-deb/` |

---

## Optional hygiene (not required to work)

1. One **canonical** keyer protocol doc: prefer `../keyer/KEYER-MEMORY.md`.  
2. After server ports, note Pi (`rpi/`) vs Ubuntu (`linux/`) separately.

---

*Updated for `rpi/` vs `linux/` split (2026-09).*
