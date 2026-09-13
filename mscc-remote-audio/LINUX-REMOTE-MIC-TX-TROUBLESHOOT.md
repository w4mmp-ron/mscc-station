# Linux trans: remote mic TX (WSJT-X TUNE, no RF)

**For:** Grok Build on the **Ubuntu radio host** (`linux/` tree only — do **not** edit `rpi/`).  
**Date:** 2026-09-13  
**Operator is weak at Linux** — you run the commands; paste findings, then fix in `linux/SDRcore-trans-linux` (and ms-sdr only if the opcode never arrives).

Related: [REMOTE-AUDIO-PUNCHLIST.md](REMOTE-AUDIO-PUNCHLIST.md), [STEW-REMOTE-AUDIO.md](STEW-REMOTE-AUDIO.md).

---

## Symptom

Windows WPF client (Tailscale) + WSJT-X + **Remote Digital**.

| Action | Result |
|--------|--------|
| MSCC **TUN** | Radio TX **and** RF power — OK |
| WSJT-X **TUNE** | Radio goes **TX** (CAT PTT `0xBA`) — **no power** |
| Remote **RX** phones/digital | Audio OK |
| CAT | Freq / PTT OK |

MSCC TUN synthesizes a carrier in trans DSP (`MODE_TUNE` / `tune_modulate`). It does **not** use UDP mic.  
WSJT-X TUNE is CAT PTT + **VAC audio** → MSA1 UDP **9101** → `remote_mic` ring → `ssb_modulate`. Silence on that ring = TX with no RF.

---

## Already proven — do not re-debug

- **ufw inactive** — not a firewall drop.
- **tcpdump** on Ubuntu (TUNE on **or** off, same):

  `tailscale0  IP 100.98.121.94.xxxxx > 100.76.242.96.9101: UDP, length 976`

  976 = MSA1 header 16 + 480 × int16. Client **is sending**; packets **hit this box** on Tailscale.
- Windows client: opcode **`0x9B` = 3** (R-Digital), mic device **CABLE Output**, TX host `100.76.242.96:9101`.
- Windows client Tailscale IP: **`100.98.121.94`**. Linux radio Tailscale IP: **`100.76.242.96`**.
- ALSA / JACK walls (`snd_func_refer`, `Unknown PCM surround51`, `jack server is not running`) are **PortAudio device probes**. They are **not** this bug. TUN making RF proves I/Q out works.

---

## What you must determine

Packets on the NIC ≠ trans **using** them.

1. Did trans get **`CMD_SET_AUDIO_DEVICE` 3** (`REMOTE_DIGITAL`)?  
2. Is **`remote_mic`** logging **`pkt ok=`** from `100.98.121.94`?  
3. If `pkt ok` but still no RF: is `G_audio_mode` 2/3 at PTT, and is `remote_mic_fill_stereo_96k` feeding `process_mic_to_iq` (not NULL / local VirtualB)?

---

## Commands (run these)

Logs (Ubuntu user home, not `/home/pi` unless that is this account):

```bash
echo "HOME=$HOME USER=$USER"
ls -l "$HOME/.local/mscc/sdrcore-trans.log" "$HOME/.local/mscc/remote-mic.ini"
ss -ulnp | grep -E '9101|8888'
```

If more than **one** process is bound to **9101**, that is a bug (SO_REUSEADDR); packets may go to the wrong socket.

```bash
grep -E 'remote_mic|REMOTE_DIGITAL|REMOTE done|CMD_SET_AUDIO_DEVICE|CMD_SET_TX_ON' \
  "$HOME/.local/mscc/sdrcore-trans.log" | tail -50

grep -E 'CMD_SET_AUDIO_DEVICE|0x9B|REMOTE' \
  "$HOME/.local/mscc/ms-sdr.log" | tail -30
```

Expect:

```text
CMD_SET_AUDIO_DEVICE REMOTE_DIGITAL done ... ready=1
remote_mic: listen UDP :9101
remote_mic: pkt ok=1 ... from 100.98.121.94
remote_mic: pkt ok=500 ...
```

While WSJT-X TUNE is held (optional extra):

```bash
sudo tcpdump -n -i tailscale0 udp port 9101 -c 5
```

---

## Code to read (`linux/` only)

| File | What |
|------|------|
| `linux/SDRcore-trans-linux/sources/remote_mic.c` | Bind 9101, parse MSA1, ring, `pkt ok` / `pkt bad`, `remote_mic_fill_stereo_96k` (48 kHz mono → 96 kHz stereo) |
| `linux/SDRcore-trans-linux/sources/udp_thread.c` | `CMD_SET_AUDIO_DEVICE` cases **2** and **3** → `G_audio_mode`, `manage_stream` |
| `linux/SDRcore-trans-linux/sources/main.c` | `sdrAudioCallback` / `sdrIqPlayOnlyCallback`: fill remote ring **only if** `G_audio_mode` is 2 or 3 **and** `remote_mic_ready()` |
| `linux/SDRcore-trans-linux/sources/commands.h` | `REMOTE_AUDIO 2`, `REMOTE_DIGITAL_AUDIO 3` |
| `linux/ms-sdr-linux/source/user_controls.c` | Forwards `0x9B`; **3** must use **Digital** mic gain (not Phones) |

Stale comment in `STEW-REMOTE-AUDIO.md`: Digital “never” uses remote mic — **wrong** for opcode **3**.

---

## Ranked hypotheses

1. **Trans never got opcode 3** — still Digital (0) or Phones (1). Then `process_mic_to_iq` uses local PortAudio / NULL, not the 9101 ring. UDP still shows on tcpdump because `remote_mic` always listens. **Fix:** ms-sdr forward + trans switch; confirm log `REMOTE_DIGITAL`.
2. **`pkt bad` / parse fail** — magic/size. Client sends 976 bytes; parser must accept that. Log `pkt bad` vs `pkt ok`.
3. **Wrong socket on 9101** — two binds, SO_REUSEADDR, leftover trans. `ss -ulnp | grep 9101` must be **this** trans PID.
4. **Mode 3 but callback not using the ring** — `sdrIqPlayOnlyCallback` vs duplex `sdrAudioCallback`; split-stream Linux path must call `remote_mic_fill_stereo_96k` for 2 **and** 3. If it only does so in one callback, I/Q play-only would TX zeros.
5. **Digital mic gain 0** — opcode 3 should get Digital mic volume from ms-sdr. If 0, SSB of zeros → no RF. **This was it.**
6. **Windows VAC silence** — only if `pkt ok` with near-zero samples. Client **9.13.4** logs `Mic TX … peak=`. Peak ~0 = WSJT Output not **CABLE Input**. That is **not** a Linux fix.

---

## If you change code

- Edit **`linux/SDRcore-trans-linux`** (and `linux/ms-sdr-linux` only if 3 is not forwarded).
- Do **not** edit `rpi/`.
- Rebuild trans on this Ubuntu box; restart servers; retest WSJT-X TUNE.
- Add a log if missing: first TX after opcode 3 should print `G_audio_mode`, `remote_mic_ready()`, and whether fill or PortAudio was used.

---

## Success (2026-09-13)

**Confirmed:** Windows WPF client (Tailscale `100.98.121.94`) + WSJT-X **Remote Digital** → Ubuntu radio (`100.76.242.96`) **transmits and puts out RF**. Tailscale login was required after a host lockup (`NeedsLogin`); daemon was already running.

**Root cause (hypothesis 5):** `$HOME/.local/mscc/user_controls.ini` had `DIGITAL_MIC_GAIN=0`. Opcode 3 forwarded that to trans → `G_mic_volume=0` → `framesToComplex` zeroed the MSA1 mic. Packets were fine (`REMOTE_DIGITAL done`, `pkt ok` from Windows, one bind on 9101). MSCC **TUN** still had RF because `tune_modulate` does not use the mic.

**Fix (`linux/` only):** remote mode 2/3 floors mic volume **0 → 50**. `pkt ok` log includes **peak**. `CMD_SET_TX_ON` logs `audio_mode`, `remote_ready`, `G_mic_volume`.

**Next test:** Windows **servers** + Ubuntu **client** (Avalonia) + WSJT-X on this laptop. Same Tailscale IPs; Windows firewall / Private profile / servers must stay up without a local WPF session.

**Also confirmed (same day):** Ubuntu **local** MSCC + WSJT-X — audio, CAT, and WSJT-X **TUNE** put out RF. **Remote also works** (operator seat + VAC/CAT). Both local-shack Digital and Remote Digital are live.
