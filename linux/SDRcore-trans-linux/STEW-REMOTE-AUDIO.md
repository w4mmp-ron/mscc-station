# Remote Audio (opcode 2) — notes for Stew

**From:** Ron  
**Full client handoff:** `mscc-remote-audio/STEW-REMOTE-AUDIO.md` ← start there  

---

## Server contract (`sdrcore-trans`)

`CMD_SET_AUDIO_DEVICE` (`0x9B`):

| Data | Mode | Mic source |
|------|------|------------|
| **0** | Digital | Local digi (VirtualB / digi path) |
| **1** | Phones / OPERATOR | Local operator mic |
| **2** | REMOTE | MSA1 UDP (default **9101**) from MsccRemotePhones |

Digital never uses the remote mic path.

Listen port: `~/.local/mscc/remote-mic.ini` → `PORT=` (default 9101). Selection is by opcode **2**, not INI `ENABLED`.

Log markers:

```text
CMD_SET_AUDIO_DEVICE REMOTE done … ready=1
remote_mic: pkt ok=…
```

Mic gain for mode **2** uses the Phones mic volume path (`Set_Mic_Volume`).

---

## Other cores (summary)

- **ms-sdr** — forwards **2**; Phones levels when not Digital  
- **sdrcore-recv** — **2** same as **1** (operator speaker); remote RX remains `remote-phones.ini` / UDP **9100**

---

## Client ask

Phones + **Remote Audio** checkbox → send **2**. Digital always **0**. Details and UI checklist in the handoff doc above.
