# MSCC client punch list — NR & AN buttons (for Stew)

**From:** Ron  
**To:** Stew (Windows MSCC / WPF client)  
**Subject:** Full bidirectional NR and Auto-Notch (AN)

---

## Goal

Make **NR** and **AN** fully bidirectional, same as **NB**:

- Client → appliance (operator toggles / sets level)
- Appliance → client (connect sync and any later server push update the UI)

Today the appliance applies, saves, and **sends** NR/AN to the client on session connect. The Windows client needs to **receive those messages and drive the NR/AN controls** so the UI always matches the radio.

---

## Opcodes

| Control | Opcode | Name | Payload |
|--------|--------|------|---------|
| **NR** | `0xA3` | `CMD_SET_NR` | **0** = off; **non-zero** = on with level (`NR_VALUE`) |
| **AN** | `0x8E` | `CMD_GET_SET_AUTO_NOTCH` | **0** = off; non-zero/on = auto-notch enabled (`AUTO_NOTCH`) |

Appliance persistence (`user_controls.ini`): `NR_VALUE=…`, `AUTO_NOTCH=…`.

---

## What the appliance already does

1. **Start (headless)** — pushes saved NR/AN to sdrcore-recv (`User_Controls_Apply_To_Cores`).
2. **Live from client** — on `0xA3` / `0x8E`: save ini, forward to recv.
3. **Client connect** — sends to GUI:
   - `CMD_SET_NR` with current `NR_VALUE`
   - `CMD_GET_SET_AUTO_NOTCH` with current `AUTO_NOTCH`

Server does **not** invert these values; treat the payload as the true DSP state.

---

## What to implement on the client

### 1. Receive path (the main gap)

When the client receives **`CMD_SET_NR` (`0xA3`)** or **`CMD_GET_SET_AUTO_NOTCH` (`0x8E`)** from the appliance:

- Update the internal model / bindings.
- Update the NR and AN **controls** (button + any NR level/slider) to match the payload.
- Do **not** re-toggle or re-send as if the user clicked (avoid echo loops).
- Use the same pattern as **NB** for server → UI.

### 2. Send path (confirm complete)

On operator action:

- **NR:** send `CMD_SET_NR` with **0** (off) or the selected level (on).
- **AN:** send `CMD_GET_SET_AUTO_NOTCH` with off/on as appropriate.

### 3. NR value semantics

- **`NR_VALUE == 0`** → NR off in the UI.  
- **`NR_VALUE != 0`** → NR on, level = that value.  

If the UI has a separate enable and level control, both must stay consistent with that single field (no separate enable bit that fights `NR_VALUE`).

### 4. Use NB as the template

Whatever you do for **NB** receive + send, do for **NR** and **AN**.

---

## Wire flow

```
Operator sets NR/AN  →  client sends 0xA3 / 0x8E  →  ms-sdr saves + → sdrcore-recv

Session connect      →  ms-sdr sends 0xA3 / 0x8E  →  client MUST update NR/AN UI
```

---

## Acceptance tests

| # | Action | Expected |
|---|--------|----------|
| 1 | On appliance, NR on (non-zero `NR_VALUE`) and AN on; no client or client disconnected | DSP active; saved in ini |
| 2 | Connect Windows client | NR and AN UI show **on**; NR level matches `NR_VALUE` |
| 3 | Disconnect client only; leave appliance running | — |
| 4 | Reconnect | UI still matches appliance (not defaulted off) |
| 5 | Toggle NR and AN from client | Audio/DSP changes; server log updates |
| 6 | Reconnect after step 5 | UI matches last saved state |

---

## Logs (optional debug)

Appliance lines around connect:

- `User_Controls_Apply_To_Cores. CMD_SET_NR → …`
- `User_Controls_Send_To_Gui. CMD_SET_NR → …`
- `CMD_GET_SET_AUTO_NOTCH` process lines

Compare those values to the client button/slider state after connect.

Thanks,  
Ron
