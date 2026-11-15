# Note for Ron: QRP CAL / AMP CAL frequency (2026-11-15)

From Stew (written with Grok Bot). FYI only. No server change is required. The Avalonia client (1.7.04, cmd-082) now sets the cal frequency itself after 0xA1.

## What changed on the client

Stew wants QRP CAL and AMP CAL to transmit on the last frequency he used on that band (or the normal band default if he has not used it yet). The freq box and the actual TX frequency must be the same. This matters most on LF/MF, where his antenna passband is narrow: if he is on 136 kHz, that is the cal frequency.

Avalonia now does this:

- On a QRP band click (and when opening the QRP tab while already on a ham band): send **0xA1 first** (so the per-band power step still loads), then **0xB6** with the chosen frequency, and show that frequency in the box.
- AMP CAL uses the same frequency picker, then 0xB6 and 0xF9. 0xF9 does not retune, so AMP already followed the client frequency; it just needed the same picker.
- Chosen frequency: current VFO if it is already on that band, else the client's per-band last-used, else the main-tab band default (20m = 14.000, 2200m = 136.000, 630m = 474.200, …).

## Your Pi server can stay as it is

On 0xA1, `Set_freq_for_power_cal` still retunes to the server table (20m = 14.010, and so on) and sets `G_tune_freq`. That is unchanged.

A 0xB6 after 0xA1 overwrites `G_tune_freq` and queues that frequency (`CMD_SET_MAIN_FREQ` in `main-controller.c`). `Cal_tune_button` does not put the table frequency back. On key-up and key-down, `SetModeRxTx` re-queues `G_tune_freq`, not the static table value. We checked linux ms-sdr, Windows ms-sdr-MKII, and `rpi/ms-sdr-linux`: they all behave this way. So the Pi works with the new client and no Pi patch.

There is a brief hop to the table frequency (tens of ms) before 0xB6 lands. The client updates the freq box only to the intended frequency.

Power cal steps (`power_cal.ini` and the in-radio 0xC8 store) stay per-band, not per-frequency. Calibrating 20m at 14.074 or 14.000 still writes the same 20m slot. That is intended.

## Optional later (Option B)

If you want to remove that brief hop, 0xA1 could stop retuning (or take the client's frequency). That would need a change in your Pi tree as well as linux/ and Windows. Not required for Stew's rule. No rush.

## Why it showed 14.150 but transmitted 14.010

The client and server had different fixed cal-frequency tables. The client set the box from its table and sent 0xB6, then 0xA1 made the radio use the server table. The box never got a frequency report back, so it stayed on the client value. Those tables are no longer used to pick the QRP/AMP cal frequency on the Avalonia client. They are recorded here in case they matter for something else:

Band | Avalonia / WPF `GetCalFrequencyHz` (old client table) | ms-sdr `Set_freq_for_power_cal` (linux, Windows, Pi)
-----|------------------------------------------------------|-----------------------------------------------------
2200 | 135.750 | 136.000
630  | 475.000 | 475.000
160  | 1.810   | 1.810
80   | 3.510   | 3.510
60   | 5.3305  | 5.3465
40   | 7.010   | 7.010
30   | 10.110  | 10.110
20   | 14.150  | 14.010
17   | 18.110  | 18.078
15   | 21.200  | 21.010
12   | 24.900  | 24.900
10   | 28.010  | 28.010

`factory/power_cal.ini` holds power steps only, not frequencies. WPF still uses the old client table; Stew will port the last-tuned behaviour to WPF on Shack later.
