# MSCC / PortAudio: mixed host APIs + sample-rate conversion

Brief synopsis for PortAudio maintainers and integrators.  
Implementation lives primarily in **SDRcore-recv-linux** and **SDRcore-trans-linux** (`manage_stream` / dual callbacks + freestanding Oboe resampler under `sources/resampler/`).

**Context:** Multus MSCC on Linux (Raspberry Pi). Radio I/Q is fixed **stereo S16 @ 96 kHz** via ALSA (USB SDR). Operator audio and digi (Pulse/PipeWire null sinks such as VirtualA/B) often use a **different PortAudio host API** and sometimes a **different sample rate** (e.g. 48 kHz phones). PortAudio does not resample, and **one full-duplex stream cannot mix host APIs** (e.g. ALSA + Pulse → error such as `-9993`).

---

## 1. Dual stream when host APIs differ

**Problem:** Full duplex with input and output on different host APIs fails.

**Approach:**

- Open **two streams**: capture-only and playback-only (or I/Q-only + AF-only).
- Bridge them with a **stereo ring buffer** (fixed capacity; underrun → silence, overrun → drop).
- **Recv (RX):** I/Q callback runs DSP @ 96 kHz → writes stereo AF into the ring → play callback reads the ring to the operator/digi device.
- **Trans (TX):** mic callback writes into the ring → I/Q play callback reads AF, modulates, outputs I/Q @ 96 kHz.

**When same host API and same rate:** keep a single **full-duplex** stream (no ring).

This is application-level composition of two PortAudio streams, not a PortAudio feature change.

---

## 2. Sample-rate conversion (when device rate ≠ 96 kHz)

**Problem:** PortAudio will not convert 96 kHz AF to 48 kHz (or the reverse) for you.

**Approach:**

- Probe output (or input) with **`Pa_IsFormatSupported`**: prefer **96 kHz**; else device default / 48000 / 44100 / …
- If play (or capture) rate **≠ 96 kHz**, force **dual stream** even if host APIs match (full duplex requires one rate).
- Insert freestanding **Oboe MultiChannelResampler** (windowed-sinc / polyphase path, **Quality::Medium**, stereo) on the AF side only:
  - **Recv:** ring holds **96 kHz** AF → resampler in **play** callback → device rate.
  - **Trans:** resampler in **mic** callback → device rate → **96 kHz** into ring → I/Q path unchanged.
- If rates match: **identity / bypass** (no resampler object).

I/Q path stays strictly **96 kHz**; only operator/digi AF is converted.

Oboe resampler is vendored under `sources/resampler/` (freestanding; no Android dependency). Thin C API: `sources/mscc_resampler.{h,cpp}`.

---

## 3. Digi vs operator devices

- Digi: Pulse **VirtualA** (recv out) / **VirtualB.monitor** (trans mic); usually opened at 96 kHz when Pulse allows.
- Operator phones: may be ALSA or Pulse, often non-96k → dual + resample.
- Custom PortAudio build with **ALSA + Pulse** host APIs (MSCC ships under `/usr/local`) so Virtual* devices appear; distro builds are often ALSA-only.

---

## 4. Why this might interest PortAudio

| Gap | Application workaround |
|-----|-------------------------|
| No mixed-host full duplex | Dual stream + ring |
| No resampling | Explicit SRC (here: Oboe freestanding resampler) before/after the stream |
| Format probe | `Pa_IsFormatSupported` to choose rate; open streams at that rate |

A future PortAudio helper (e.g. optional SRC, or documented dual-stream patterns for mixed APIs) would reduce the need for this boilerplate in multi-device SDR/audio bridges.

---

## 5. One-line summary

**Two PortAudio streams + an AF ring when host APIs or rates disagree; I/Q fixed at 96 kHz; Oboe resamples only the operator/digi AF path when the device cannot open at 96 kHz.**

---

## Related trees

| Tree | Role |
|------|------|
| `SDRcore-recv-linux` | RX dual stream, digi ring, play-side downsample |
| `SDRcore-trans-linux` | TX dual stream, mic ring, capture-side upsample |
| `mscc-portaudio` | Pulse+ALSA PortAudio `.deb` → `/usr/local` |
| `mscc-deb` | Packaging / install docs for the full stack |

*Written for handoff to PortAudio / integrators. Implementation detail may evolve; this describes the architecture as of the MSCC Linux dual-stream + Oboe work.*
