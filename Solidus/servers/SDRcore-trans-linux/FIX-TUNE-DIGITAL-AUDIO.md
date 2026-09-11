# Fix: TUNE mode produces no I/Q when Audio P/D is Digital (D)

**Component:** `SDRcore-trans-linux`  
**Symptom (Ron / RPi5 MSCC Linux port):** In TUNE mode, with the Audio **P/D** control on **D** (digital), `sdrcore-trans` does not generate I/Q and there is no RF power. With **P** (operator/phones), TUNE works. TUNE must work in both P and D.

## Root cause

Two related Linux-port issues stacked:

### 1. Invalid digital mic index stopped the PortAudio stream

`G_digital_input_device_index` is initialized to `NO_INPUT_DEVICE` (**100**). It is only set when `~/.local/mscc/digital-microphone.ini` matches a PortAudio device name.

Linux correctly refuses to match an **empty** digital name against every device (Windows historically matched empty `strstr` on all devices, so a digital index was always “found”). If the digital mic is missing or misnamed:

- `G_digital_input_device_index` stays **100**
- `CMD_SET_AUDIO_DEVICE` (P/D → D) does:

  ```c
  manage_stream(1, G_digital_input_devices[G_digital_input_device_index]...);
  ```

- Index **100** is past `G_digital_input_devices[50]` → out-of-bounds read, bad device/channel, `Pa_OpenStream` / start fails
- `stream_running` stays false → **no audio callback → no I/Q at all**

Operator (P) always uses the validated operator mic, so TUNE works in P.

### 2. NULL mic input zeroed I/Q (TUNE does not need mic)

Even with a valid digital device (e.g. ALSA loopback / MSCC virtual cable with nothing writing into it), PortAudio can deliver `inputBuffer == NULL` on underflows. The old callback path was:

```c
if (inputBuffer == NULL) {
    /* write silence to Multus I/Q out — never call tune_modulate() */
}
```

`tune_modulate()` synthesizes the carrier from DC-offset LO samples; it does **not** need mic audio (`framesToComplex` already forces gain 0 in TUNE/CW). Skipping the modulator on NULL input meant **no TUNE carrier** precisely when digital capture was idle—typical when using D without a digital app running.

## Fix (applied)

| File | Change |
|------|--------|
| `sources/main.c` | After device enum: if digital mic not found, **fall back** to operator mic index/record so D still has a valid stream. |
| `sources/main.c` | `sdrAudioCallback`: for **MODE_TUNE** / **MODE_CW**, if `inputBuffer == NULL`, zero complex input and still run `tune_modulate` + `fastconv` so I/Q is generated. |
| `sources/udp_thread.c` | `CMD_SET_AUDIO_DEVICE`: **bounds-check** digital/operator indices; on digital open failure, **fall back** to operator mic so the I/Q path stays alive (TUNE keeps working). |
| `sources/extern.h` | Shared `NO_INPUT_DEVICE` (100). |

## Rebuild on the Pi

```bash
cd /path/to/SDRcore-trans-linux
make clean && make
# install/restart as you usually do, e.g.:
# cp sdrcore-trans ~/mscc/   # or your deploy path
# restart MSCC stack
```

## Verify

1. Configure operator mic (`operator-microphone.ini` under `~/.local/mscc/`) so P TUNE already works.
2. Optional: set `digital-microphone.ini` to the virtual cable / digital capture name (must match PortAudio/ALSA device substring).
3. MSCC Audio **P/D → D**, enter **TUNE** → forward power / I/Q should appear (even with no digital app feeding the cable).
4. Check `~/sdrcore-trans.log` for:
   - `Digital mic not found — fallback to operator mic` (if digital ini missing)
   - `CMD_SET_AUDIO_DEVICE DIGITAL done` with `stream_status=0`
   - Not: open/start stream failed without a subsequent operator fallback

## Notes

- TUNE/CW intentionally ignore mic content (`gain = 0` in `framesToComplex`); the carrier is generated in `tune_modulate()`.
- For real digital modes (FT8, etc.) you still need a correctly named digital capture device and a writing app; the fallback only keeps the duplex stream and TUNE path alive when digital is missing or silent.
- Same class of bug can affect CW keying I/Q if digital input is null; CW uses the same modulator path and is covered by the NULL-input fix.
