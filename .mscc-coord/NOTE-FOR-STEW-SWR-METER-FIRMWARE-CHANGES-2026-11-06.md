# For Stew: what we changed in the WiFi SWR meter firmware

From Ron / Claude, 2026-11-06.

Thanks for adding the meter source on 11-01. We copied it to `swr-meter/ron/HF_SWR_R4_1/` and made
our changes there. Your copy in `swr-meter/HF_SWR_R4_1/` is untouched. Ron is running our build
in his meter now, and everything below has been tested at his station except where it says not.

To load it over the air:

    http://hf-swr-meter.local/update
    file: swr-meter/ron/build/HF_SWR_R4_1.ino.bin

All changes are marked `W4MMP` in the source, so they are easy to find if you want any of them
in your copy. Whether to take them, and how, is your call.


## 1. Tune allowance (the auto-tune trips)

This is the fix for the trips in my 11-01 note. What we found in the source: one trip count is
one forward + reflected pair, about 2.4 ms. So the maximum count of 24 is only about 58 ms of
high SWR, far too short for a tuner that is still hunting.

What it does now:

- ms-sdr (Pi 3.191) tells the meter when TUNE is on: `/api?tune=1` just before it keys,
  `/api?tune=0` after it unkeys.
- While that flag is set, high SWR is not counted, for at most "Tune Allow" seconds (0 to 10,
  default 5, 0 = off). After that the normal fast check is back, even if `tune=0` never arrives.
- Every other transmission (voice, digital, CW, 2 TONE) gets the normal fast check from the
  first reading, exactly as before.
- Set on the meter's SETTINGS page or the web page. Saved. Shown in the JSON as `tuneGrace`
  and `tune`.

Tested: a 16 s tune on 15 m, no fault. A meter with the old firmware just ignores `tune=1`.

While we were in there: the reflected power table tops out at 25.00 W, which is why both trips
read exactly 25.00 W. Above that the reading (and the SWR worked out from it) is clipped. We
didn't change that. You know the coupler and the tables better than we do.


## 2. Meter name on the network (mDNS)

The meter now answers as `hf-swr-meter.local` (`lf-swr-meter.local` on the LF build). Ron needed
this because his VPN blocks normal name lookups on the LAN but lets this through.


## 3. WiFi keeps retrying

Before, if the meter wasn't on WiFi 15 s after power-up, it showed "AUTH FAILED", turned WiFi off
and never tried again until a power cycle. That happened to Ron with the right password,
probably the router being slow, and ms-sdr lost the meter.

Now it waits 30 s and tries again, for as long as WiFi is enabled. The screen says why:
NO NET-RETRY, JOIN FAIL-RETRY (often a wrong password) or NO WIFI-RETRY, then CONNECTING...
Turning WiFi off on the Network page stops the retries.

Tested: the meter came back on WiFi. The retry itself (router off, meter restarted) is not
tested yet.


## 4. PEAK is now a real peak hold

Before, PEAK didn't really hold. The hold was 600 ms (the comment says 5 s) and only restarted
on a reading at or above the peak. Then it fell fast, but never below the live reading, so on
the screen it looked almost live. Reflected had no hold timer of its own.

Now:

- Forward and reflected each have their own hold timer.
- Any reading within 10% of the held peak restarts the hold, so steady CW or WSJT-X keeps it up.
- Hold time "Peak Hold (s)" on the web page: 0.5 to 5 s, default 2 s. Saved. No touch control.
- After the hold it falls to zero in about 3.5 s.


## 5. AVG on the PEAK / VOLTS button

The button now steps VOLTS, PEAK, AVG (cyan, amber, green). The label beside the number changes
to match: VOLT:, PEAK: or AVG:.

AVG is the forward and reflected watts averaged over the last 1 s. It first shows once a full
second of the transmission has been measured, so the first number is already steady. It
updates at most 4 times a second, and stays on screen for the hold time after you unkey.
Ron's old PEAK / VOLTS choice is carried over the first time it starts.

Tested by Ron with WSJT-X.


## 6. PEAK SWR beside the big SWR

Added later the same day. The main screen now shows a smaller PEAK SWR to the right of the big
SWR number. To make room, "SWR:" moved to the left edge and the big number moved left with it.
Same size font as before.

- It holds the highest SWR (the same averaged SWR as the big number) while you transmit.
- Same hold rule and hold time as the PEAK watts. After the hold it goes straight back to the
  live SWR, or 1.00 when you're not transmitting.
- Amber, red when over the SWR alarm threshold. Always shown.
- Screen only: not in the JSON or on the web page. The cross-needle screen is unchanged.

Tested by Ron.


## JSON

New keys, added beside the old ones (nothing removed or renamed):

    "peakRef"  reflected peak (W)
    "peakHold" hold time (s)
    "avg"      forward 1 s average (W)
    "avgRef"   reflected 1 s average (W)
    "tuneGrace", "tune"   tune allowance (from item 1)


## Building it

We built with arduino-cli (the one inside the Arduino IDE), esp32 core 3.3.11, lvgl 9.4.0,
TFT_eSPI 2.5.43, with your `User_Setup.h` and `lv_conf.h` copied into the libraries.
Board: ESP32 Dev Module, 4 MB flash, partition scheme "Minimal SPIFFS".
