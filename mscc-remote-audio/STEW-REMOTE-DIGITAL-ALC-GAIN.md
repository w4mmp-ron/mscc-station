# Remote Digital — why Stew may not reach ALC (VB-Audio)

**From:** Ron  
**To:** Stew  
**Date:** 2026-09-19 (updated)  
**For:** Grok Build / follow-up diagnosis  
**Related:** `STEW-REMOTE-AUDIO.md`, `REMOTE-AUDIO-PUNCHLIST.md`, `LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md`

---

## Part A — English (for Stew)

### What we know

In normal use, these modes can be driven hard enough that **ALC moves**:

- Local **Phones**
- Local **Digital**
- **Phones Remote**

**Remote Digital** is the odd one out on Stew’s VB-Audio setup: drive often stays below ALC.

Ron’s station is a special case. He uses **two USB audio adapters wired back-to-back**. On that path he **can** push into ALC by raising levels in the **Windows Sound** panel. That is an **extra external gain stage** beyond a plain virtual cable — not something the Remote Digital mic slider is doing.

Stew is on **VB-Audio** (virtual cable). He does **not** have that USB loop boost.

### About VB-Audio “unity gain”

VB-Cable is roughly **unity** in the sense that it is a **1:1 pipe**: it does **not** add a mic-preamp-style boost the way a physical USB adapter can when you crank Windows/hardware gain. That does **not** mean VB is always quiet. If the source (WSJT-X or another app) feeds it a hot signal, and Windows Cable levels aren’t pulled down, **plenty of audio** comes out the other side.

**Stew’s note:** VB already provides **plenty of audio for his other applications**. So “VB can’t supply enough level” is **unlikely by itself**. The shortfall for Remote Digital ALC is more likely **after** the hot VB feed enters MSCC / the network mic path / `sdrcore-trans`.

### Where the shortfall can still be

1. **VB / Windows / WSJT** — still worth a quick level check (wrong Cable device, muted/low Windows slider, WSJT TX low), but **not the leading theory** given other apps are loud on VB.  
2. **MSCC sound management** — how Remote Digital captures VB and applies gain (Remote mic slider disabled; Digital mic gain → `G_mic_volume`).  
3. **`sdrcore-trans`** — UDP mic path + `G_mic_volume` / digital line-level scaling vs phones.

### Likely picture (not yet proven)

Most likely **MSCC and/or trans**, not “VB is broken”:

1. **VB** can deliver adequate level (Stew’s other apps prove that). Ron still has an *extra* boost stage Stew doesn’t; that explains Ron hitting ALC more easily, not Stew having a dead VB.  
2. In the WPF **Remote Digital** window, the **mic volume slider is disabled** (forced to 100%). Drive into the radio for mode **3** mainly follows **Digital mic gain** via ms-sdr → `G_mic_volume`, not a Remote-window boost. If that path attenuates or under-applies Digital gain, MSCC looks gain-starved even with a hot VB feed.  
3. In **`sdrcore-trans`**, Remote Digital applies `G_mic_volume` on the MSA1/UDP mic path and treats digital as **line-level** (ALC behavior differs from phones). Capture/resample/UDP or gain scaling can sit under ALC even when VB itself is loud into other apps.

### What Stew can check quickly

1. Confirm the same VB path that is loud in other apps is what MSCC Remote Digital **Mic** is using (CABLE Output / configured VAC).  
2. Raise MSCC **Digital mic gain** (main audio controls, not the greyed Remote mic slider); watch ALC.  
3. Confirm Remote Digital is really opcode **`0x9B` = 3**, and trans log shows `REMOTE_DIGITAL` / `G_mic_volume` not stuck low.

---

## Part B — Structured (for Grok Build)

```yaml
doc: STEW-REMOTE-DIGITAL-ALC-GAIN
date: 2026-09-19
updated: 2026-09-19
from: Ron
to: Stew
status: diagnosis-notes

symptom:
  mode: remote_digital   # CMD_SET_AUDIO_DEVICE 0x9B == 3
  observation: cannot_drive_into_ALC
  contrast:
    alc_reachable:
      - local_phones
      - local_digital
      - remote_phones    # 0x9B == 2
    alc_hard_on_vb: remote_digital
  ron_exception:
    setup: dual_USB_audio_adapters_back_to_back
    gain_stage: Windows_Sound_panel
    result: CAN_drive_into_ALC_on_remote_digital
    note: extra_gain_beyond_unity_VAC_pipe
  stew_setup:
    audio: VB-Audio_virtual_cable
    has_ron_usb_gain_stage: false
    stew_report: >
      VB provides plenty of audio for his other applications.
      So VB is not "always quiet"; unity means 1:1 pipe, not no level.

vb_unity_clarification:
  meaning: 1:1_pipe_no_mic_preamp_boost
  not_meaning: always_quiet_or_insufficient_for_all_apps
  implication: >
    H1 alone is weakened. Prefer investigating level loss after VB
    enters MSCC Remote Digital / UDP / sdrcore-trans (H2, H3).

hypothesis_priority:
  - H2_mscc_sound_mgmt   # leading after Stew VB note
  - H3_sdrcore_trans
  - H1_vb_windows_wsjt   # quick check only; unlikely sole cause

hypotheses:
  - id: H1_vb_windows_wsjt
    title: VB / Windows / WSJT level or device mismatch
    priority: low_after_stew_note
    summary: >
      VB-Cable is a unity (1:1) pipe with no extra preamp boost like Ron's
      USB loop. That does not mean quiet: Stew reports plenty of audio through
      VB in other apps. H1 is only a wrong-device / muted-slider / low-WSJT
      check — not "VB cannot supply level."
    check:
      - confirm MSCC Remote Digital Mic is the same CABLE Output other apps use
      - glance at Windows Sound levels on that Cable (not pulled down/muted)
      - glance at WSJT-X TX level
      - do not treat "unity gain" as proof VB is the bottleneck
    code_hints:
      - mscc-ui/.../MSCC.Wpf/RemoteAfWindow.xaml.cs (VAC device selection)
      - RemoteAudio/RemoteMicSender.cs (client capture volume)

  - id: H2_mscc_sound_mgmt
    title: Lack of gain / attenuation in MSCC sound management
    priority: leading
    summary: >
      Remote Digital disables RemoteAfWindow mic slider (forced 100%).
      Mode 3 drive uses Digital mic gain via ms-sdr → G_mic_volume.
      Hot VB into other apps can still be under-driven or attenuated here.
    check:
      - confirm MicVolumeSlider disabled when IsDigitalAudio
      - raise main Digital mic gain in MSCC; watch ALC
      - verify ms-sdr forwards Digital gain for opcode 3 (not Phones gain)
      - log G_mic_volume at CMD_SET_TX_ON / mode 3
      - compare client capture peak vs what other loud VB apps use
    code_hints:
      - RemoteAfWindow.xaml.cs RefreshPath digi branch
      - ms-sdr user_controls / Digital mic gain path
      - LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md (DIGITAL_MIC_GAIN=0 case)

  - id: H3_sdrcore_trans
    title: Lack of gain in sdrcore-trans remote digital path
    priority: leading_or_next
    summary: >
      UDP mic → remote_mic → framesToComplex * G_mic_volume;
      digital/remote treated as line-level; ALC curve differs from phones.
      Can sit under ALC even when VB is loud into other Windows apps.
    check:
      - confirm G_audio_mode == REMOTE_DIGITAL_AUDIO (3) at PTT
      - remote_mic_ready + remote_mic_fill used (not local VirtualB)
      - G_mic_volume value while TX
      - compare doALC / configured_drive vs local Digital
      - pkt peak / fill amplitude if logged
    code_hints:
      - windows-work-tree/SDRcore-trans/sources/remote_mic.c
      - dsputils.c framesToComplex
      - driver.c configured_drive for DIGITAL / REMOTE_DIGITAL
      - alc.c digital vs phones scaling

# IMPORTANT for Build agents
build_rules:
  examine_one_hypothesis_at_a_time: true
  suggested_order: [H2_mscc_sound_mgmt, H3_sdrcore_trans, H1_vb_windows_wsjt]
  procedure:
    - Pick exactly one hypothesis (prefer suggested_order unless directed).
    - Investigate and report evidence for that one only.
    - Do not mix fixes or conclusions across hypotheses in the same pass.
    - After one hypothesis is confirmed or ruled out, stop and ask before the next.
```

---

## Build instruction (short)

**Examine only one possibility at a time.** Prefer **H2**, then **H3**, then **H1** (H1 is weakened by Stew’s “VB is loud in other apps” note). Finish and report that one before touching the next.
