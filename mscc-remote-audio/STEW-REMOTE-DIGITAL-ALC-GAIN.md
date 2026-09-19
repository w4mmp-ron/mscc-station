# Remote Digital — why Stew may not reach ALC (VB-Audio)

**From:** Ron  
**To:** Stew  
**Date:** 2026-09-19  
**For:** Grok Build / follow-up diagnosis  
**Related:** `STEW-REMOTE-AUDIO.md`, `REMOTE-AUDIO-PUNCHLIST.md`, `LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md`

---

## Part A — English (for Stew)

### What we know

In normal use, these modes can be driven hard enough that **ALC moves**:

- Local **Phones**
- Local **Digital**
- **Phones Remote**

**Remote Digital** is the odd one out for a typical VB-Audio setup: drive often stays below ALC.

Ron’s station is a special case. He uses **two USB audio adapters wired back-to-back**. On that path he **can** push into ALC by raising levels in the **Windows Sound** panel. That is an **external** gain stage, not something the Remote Digital mic slider is doing.

Stew is on **VB-Audio** (virtual cable). He does **not** have that USB loop / Windows hardware boost. So if he cannot reach ALC on Remote Digital, the shortfall is somewhere in:

1. **VB / Windows / WSJT levels** (upstream of MSCC), or  
2. **MSCC sound management** (how Remote Digital applies gain), or  
3. **`sdrcore-trans`** (UDP mic path + `G_mic_volume` / digital scaling).

### Likely picture (not yet proven)

Most likely a **stack**, not one bug:

1. **VB-Audio** is roughly unity/line. WSJT TX + Cable Output may simply be quieter than Ron’s cranked USB loop. Virtual cables often have less headroom than a physical adapter.
2. In the WPF **Remote Digital** window, the **mic volume slider is disabled** (forced to 100%). Drive into the radio for mode **3** mainly follows **Digital mic gain** via ms-sdr → `G_mic_volume`, not a Remote-window boost. If Digital mic gain is modest, MSCC looks gain-starved.
3. In **`sdrcore-trans`**, Remote Digital applies `G_mic_volume` on the MSA1/UDP mic path and treats digital as **line-level** (ALC behavior differs from phones). A “full” VB feed can still sit under ALC until something upstream is hotter, or until Digital gain in trans is actually high enough.

### What Stew can check quickly (no code)

1. Raise **WSJT-X TX** level and Windows **CABLE Output** (and related VB) levels; watch ALC on a TUNE/TX.  
2. Raise MSCC **Digital mic gain** (main audio controls, not the greyed Remote mic slider).  
3. Confirm Remote Digital is really opcode **`0x9B` = 3**, and trans log shows `REMOTE_DIGITAL` / `G_mic_volume` not stuck low.

---

## Part B — Structured (for Grok Build)

```yaml
doc: STEW-REMOTE-DIGITAL-ALC-GAIN
date: 2026-09-19
from: Ron
to: Stew
status: diagnosis-notes-only  # no code changes implied by this file alone

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
  stew_setup:
    audio: VB-Audio_virtual_cable
    has_ron_usb_gain_stage: false

hypotheses:
  - id: H1_vb_windows_wsjt
    title: Lack of gain in VB / Windows / WSJT chain
    summary: >
      VAC is ~unity/line. WSJT TX + Cable Output levels may be too low;
      VB often lacks headroom of Ron's physical USB boost.
    check:
      - raise WSJT-X TX level
      - raise Windows Sound levels on CABLE Output / VB devices
      - confirm Remote Mic device is CABLE Output (or configured VAC)
      - watch peak on UDP / trans pkt ok if logged
    code_hints:
      - mscc-ui/.../MSCC.Wpf/RemoteAfWindow.xaml.cs (VAC device selection)
      - RemoteAudio/RemoteMicSender.cs (client capture volume)

  - id: H2_mscc_sound_mgmt
    title: Lack of gain in MSCC sound management
    summary: >
      Remote Digital disables RemoteAfWindow mic slider (forced 100%).
      Mode 3 drive uses Digital mic gain via ms-sdr → G_mic_volume.
    check:
      - confirm MicVolumeSlider disabled when IsDigitalAudio
      - raise main Digital mic gain in MSCC
      - verify ms-sdr forwards Digital gain for opcode 3 (not Phones gain)
      - log G_mic_volume at CMD_SET_TX_ON / mode 3
    code_hints:
      - RemoteAfWindow.xaml.cs RefreshPath digi branch
      - ms-sdr user_controls / Digital mic gain path
      - LINUX-REMOTE-MIC-TX-TROUBLESHOOT.md (DIGITAL_MIC_GAIN=0 case)

  - id: H3_sdrcore_trans
    title: Lack of gain in sdrcore-trans remote digital path
    summary: >
      UDP mic → remote_mic → framesToComplex * G_mic_volume;
      digital/remote treated as line-level; ALC curve differs from phones.
    check:
      - confirm G_audio_mode == REMOTE_DIGITAL_AUDIO (3) at PTT
      - remote_mic_ready + remote_mic_fill used (not local VirtualB)
      - G_mic_volume value while TX
      - compare doALC / configured_drive vs local Digital
    code_hints:
      - windows-work-tree/SDRcore-trans/sources/remote_mic.c
      - dsputils.c framesToComplex
      - driver.c configured_drive for DIGITAL / REMOTE_DIGITAL
      - alc.c digital vs phones scaling

# IMPORTANT for Build agents
build_rules:
  examine_one_hypothesis_at_a_time: true
  procedure:
    - Pick exactly one of H1, H2, H3.
    - Investigate and report evidence for that one only.
    - Do not mix fixes or conclusions across hypotheses in the same pass.
    - After one hypothesis is confirmed or ruled out, stop and ask before the next.
```

---

## Build instruction (short)

**Examine only one possibility at a time** (H1 → then H2 → then H3, or as directed). Finish and report that one before touching the next.
