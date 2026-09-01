# External WiFi SWR Meter → MSCC Integration Roadmap

**Status:** v1 implemented in MSCC client (2026-07-25)  
**Last discussed:** 2026-07-25 (Grok Build session)  
**Purpose:** Refresh context when resuming on another machine (e.g. shack PC).

---

## 1. Background

### Hardware
- **LF SWR meter** and **HF SWR meter** — ESP32 LVGL units, same protocol, different RF lookup tables.
- Firmware reference: `Backup-Refs/References/LF_SWR_R3_8/` (`LF_SWR_R3_8.ino`).
- Only **one meter is powered at a time** in real operation (HF vs LF gear is not a simple flip). Shack automation might change that later; design for two configs, one live stream.

### How data is viewed today
1. Web browser → meter IP (dashboard + `/api`)
2. Separate desktop app (UDP and/or HTTP)
3. **Years ago:** serial/USB into classic MSCC (old single-needle / mode-switch path; not dual-needle)

### What the meter already does well
- Calibrated **forward / reflected / SWR / peak**
- **Fault relay** opens amp PTT path on latched high SWR (protects finals)
- Thresholds, trip count, min-power ignore, timers — all on the meter
- Fault is reported only after those parameters are satisfied

MSCC should **trust** the meter’s `fault` bit and numbers; do not reimplement trip logic.

---

## 2. Network / protocol (no firmware change needed)

### UDP unicast (primary live path)
- Meter pushes JSON to a configured **PC IP + port** (default example: `192.168.1.254:6999`).
- Rates (firmware): ~**1000 ms** when idle / low power; ~**30 ms** when RF present.
- Enable + target IP/port set on the meter UI (UDP Enable, UDP IP, UDP Port).

### HTTP (status, reset, optional poll)
- `GET /api` → same JSON as UDP.
- Reset: `GET /api?action=reset` (meter only accepts when forward power is below its min threshold).
- Settings can also be changed via `/api?set=...&val=...` (optional for later; not v1).

### Example JSON (`buildJsonData()`)

```json
{
  "v": 0.123,
  "fwd": 12.5,
  "ref": 0.3,
  "peak": 15.0,
  "swr": 1.25,
  "fault": 0,
  "tx": 1,
  "swrThr": 2.0,
  "tripCnt": 3,
  "bypass": 0,
  "lowThr": 10.0
}
```

| Field | Meaning |
|-------|---------|
| `fwd` / `ref` / `peak` | Power (W) |
| `swr` | Standing wave ratio |
| `fault` | Latched SWR fault (1 = tripped) |
| `tx` | RF above meter min threshold |
| `swrThr` / `tripCnt` / `bypass` / `lowThr` | Meter policy (display optional; trip logic stays on meter) |

### Where MSCC sits on the network
- SWR is a **client/UI concern**, not ms-sdr / SDRcore.
- **Configure each meter’s UDP target to the machine running MSCC** (the UI PC).
- If servers run on PC A and UI on PC B: meters → **PC B**; radio UDP stays as today. Servers do not need to see the SWR meters.
- Meter and MSCC UI must be on the same LAN (or routable). Windows firewall must allow the listen port.

---

## 3. Architecture (keep separate from radio UDP)

```
[HF or LF SWR ESP32] --WiFi UDP/HTTP--> [MSCC.Core SWR service] --> [UI: S-meter slot on TX]
[ms-sdr / SDRcore]   --radio UDP------> [UdpRadioService]      --> [S-meter RX, ALC, spectrum, PTT, ...]
```

- **Do not** fold SWR JSON into `UdpRadioService`.
- New conceptual pieces:
  - Core: e.g. `ISwrMeterService` / UDP listener + optional HTTP client
  - Settings: two profiles (HF / LF)
  - ViewModel: bind values, TX face switch, fault → RX, RESET
  - Control: single-needle face reusing patterns from `AnalogSMeterControl`

Classic MSCC had `VU_MeterLibrary` single-needle modes and separate Forward/Reverse meters — **not** a Bird-style cross-needle face. New WPF analog meters are the right drawing base; do **not** revive the old DLL.

---

## 4. Dual radio + two meter configs

MSCC already selects **Proficio (HF)** vs **Geminus (LF)**.

| Radio selection | SWR profile |
|-----------------|-------------|
| Proficio / HF   | HF meter IP (+ port, enable, optional full-scale watts) |
| Geminus / LF    | LF meter IP (+ port, enable, …) |

- Each profile has its **own unique meter IP** (and port if needed).
- When the radio button is pressed, **active SWR config switches** with it.
- Only one physical meter on at a time → one live stream; two saved configs is enough.
- Optional later: distinct UDP listen ports if both meters ever stay powered (not required now).

### Config fields (v1 sketch)

Per profile (HF / LF):

- Enable external SWR  
- Meter IP (HTTP / identity / reset)  
- UDP listen port on MSCC PC (if meters use different targets)  
- Optional: FWD full-scale watts for the needle  
- Optional label for debug  

Persist in the same style as other MSCC settings (ini / SpectrumWaterfallSettings pattern — exact store TBD at implement time).

---

## 5. UI plan

### Location
- **Same footprint as the S-meter** (left analog meter on MAIN).
- **RX:** normal S-meter (unchanged).
- **TX:** replace that face with the **external SWR single-needle** meter.
- **ALC** stays ALC on the right (do not steal ALC for SWR).
- **No third permanent analog meter** (too busy).

### Single needle (not dual-needle)
- Cross-needle SWR arcs were rejected: hard to calibrate/read at small size.
- **One needle** + digital readout is the plan.

### Digital readout
- Default: show **SWR** (e.g. `1.25`).
- **Click** cycles **SWR ↔ FWD** (needle scale and label follow).
- Optional later: show REF as text only.

### TX detect for face switch
- Prefer meter `tx == 1` for “RF is present.”
- May also use MSCC PTT/TUN as a secondary cue (product choice at implement: flip on PTT immediately vs wait for `tx`).

### Scales (starting point from classic MSCC labels)
- SWR dial labels historically: 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.5, 3.0  
- FWD: QRP vs higher full-scale per profile if needed  

---

## 6. Fault protection (hardware + MSCC)

### Hardware (already on meter)
1. Trip criteria met → latch `faultState`
2. **Fault relay** breaks amp PTT (protects amp finals)
3. JSON reports `fault: 1`

### MSCC software layer (planned)
On **fault rising edge** (`0 → 1`):

1. Force radio **RX**: clear PTT and TUN, `SetTransmitAsync(false)` (existing radio path)
2. **Inhibit** re-key until fault clears
3. UI: fault presentation + **RESET** control (below)
4. Do not spam TX-off every packet; edge + latched inhibit

On **fault clear** (`1 → 0`):

1. Clear TX inhibit  
2. Stay in **RX** until operator deliberately keys again (no auto-TX)

### RESET UX in MSCC
- While `fault == 1`, the **digital readout becomes a red button: `RESET`**
- Press → HTTP reset to **active profile’s meter IP** (same as browser / desktop app)
- Meter may refuse reset if RF still above min power → keep showing RESET until `fault == 0`
- SWR↔FWD click is inactive while fault (area is RESET only)
- Other reset paths still valid: physical button on meter, browser, desktop app

### Why both layers
| Layer | Protects |
|-------|----------|
| Meter fault relay | Amp finals (PTT to amp) |
| MSCC force RX | Radio TX / drive / operator re-key |
| RESET | Clears latch after fix |

**Note:** External footswitch PTT that bypasses MSCC may fight software unkey; normal MSCC-keyed op is the primary path.

---

## 7. Scope

### v1 (shippable)
- [ ] Core SWR service: UDP listen + parse JSON; optional HTTP GET  
- [ ] HF + LF connection profiles; switch with radio selection  
- [ ] Settings UI or ini keys for IP/port/enable  
- [ ] TX: S-meter slot → single-needle SWR/FWD + clickable digital  
- [ ] RX: restore S-meter  
- [ ] Fault → force RX + TX inhibit + red RESET  
- [ ] RESET → HTTP `action=reset` on active meter  
- [ ] Fault color / clear status messaging  
- [ ] No firmware changes; no ms-sdr SWR path required  

### Explicitly out of v1 (unless you change your mind)
- Dual-needle / Bird cross-needle face  
- Third always-on analog meter  
- Remote editing of all meter thresholds (alarm, trip count, min power) in MSCC  
- Merging radio internal FWD/REV/SWR (classic `0x0B` extended) with external meter  
- Broadcasting to multiple MSCC clients  

### Nice later
- HTTP poll fallback if UDP mis-aimed  
- Show `swrThr` / fault status line  
- Per-profile FWD full-scale  
- “Reset only when RF off” toast if HTTP reset fails  
- Discover meter / open web UI from MSCC  

---

## 8. Implementation order (when coding starts)

1. **Settings + profiles** — HF/LF IP (port, enable); wire to radio select  
2. **Core listener** — UDP bind, parse JSON, events (`Fwd`, `Ref`, `Swr`, `Fault`, `Tx`, …)  
3. **ViewModel bind** — live values for active profile only  
4. **TX face** — single-needle control (fork/adapt `AnalogSMeterControl` patterns); digital SWR↔FWD  
5. **Fault policy** — edge detect, force RX, inhibit, red RESET + HTTP  
6. **Polish** — colors, scales, firewall notes in README, shack checklist  

Suggested placement when implemented:

- Core service under `MSCC.Core/Services/`  
- WPF control under `MSCC.Wpf/Controls/`  
- Settings with existing ini/settings patterns  

---

## 9. Shack operator checklist (once)

1. Put meter on same WiFi/LAN as the **MSCC UI** PC.  
2. On meter: set **UDP Enable**, **UDP IP** = MSCC PC, **UDP Port** = MSCC listen port.  
3. In MSCC: save **HF** and **LF** meter IPs; enable external SWR.  
4. Reserve DHCP or static IPs for PC + meters if unicast targets drift.  
5. Allow inbound UDP on the listen port (Windows firewall).  
6. Test: TX → SWR face; force high SWR (dummy/safe setup) → amp relay + radio RX + red RESET → reset → RX until re-key.  

---

## 10. Decisions log (agreed)

| Topic | Decision |
|-------|----------|
| Dual-needle cross arcs | **No** — calibration/readability risk |
| Meter UI style | **Single needle** in S-meter slot on TX |
| Digital | SWR default; **click** toggles SWR ↔ FWD |
| Fault digital | **Red RESET button**; HTTP reset to meter |
| Third meter | **No** |
| ALC on TX | **Stays ALC** |
| Trip logic | **On meter only**; MSCC uses `fault` |
| Two meters live | Not required; **one on at a time** |
| Two configs | **Yes** — switch with radio select |
| Servers remote UI | SWR targets **UI PC**; independent of server host |
| Amp protection | Meter **relay** (existing) |
| Radio protection | MSCC **force RX + inhibit** on fault |
| After fault clear | Stay RX; user keys again |

---

## 11. Key code / path references

| What | Where |
|------|--------|
| SWR firmware | `Backup-Refs/References/LF_SWR_R3_8/LF_SWR_R3_8/` |
| JSON + UDP + HTTP API | `LF_SWR_R3_8.ino` — `buildJsonData()`, `sendUdpPacket()`, `setupWebServerRoutes()` |
| Current S-meter UI | `src/MSCC.Wpf/Controls/AnalogSMeterControl.*` |
| ALC meter | `src/MSCC.Wpf/Controls/AnalogAlcMeterControl.*` |
| Radio TX/PTT | `IRadioService.SetTransmitAsync`, `MainViewModel` PTT/TUN |
| Classic FWD/REV/SWR modes | Old WinForms `Main_Form` / `guiCode` + `VU_MeterLibrary` (reference only) |
| Extended radio SWR opcodes (future/other) | `UdpRadioService` 0x0B subs FORWARD/REVERSE/SWR — **not** this project |

---

## 12. One-line summary

**On TX, the S-meter slot becomes a single-needle external SWR/FWD meter (click digital to swap); HF/LF each have their own meter IP; on latched fault, MSCC forces RX, shows red RESET, and clears the meter fault over HTTP — amp PTT is already broken by the meter’s relay.**

---

*When ready to implement: start a Build session in this repo, open this file, and say “build SWR v1 from the roadmap.”*
