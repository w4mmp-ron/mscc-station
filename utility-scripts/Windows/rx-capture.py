#!/usr/bin/env python3
"""
rx-capture.py: record 1 s of the radio's raw receive I/Q from "Microphone (Multus Sound)"
and say what it looks like. For the "spectrum all yellow, S9+40, no audio" fault.

    python rx-capture.py          (1 s)
    python rx-capture.py 3        (3 s)

It can run with MSCC open (Windows shared mode). Run it BEFORE pressing NRST, while the
fault is still there. Also run it once when all is well, to
have a normal reading to compare with.

Saves the recording as %USERPROFILE%\\.local\\mscc\\rx-capture-<date-time>.wav and prints,
per channel: level, DC, peak, how many samples are near full scale, how many different
values there are, and whether the bytes look shifted. Needs: pip install sounddevice numpy
"""
import datetime
import os
import sys
import wave

import numpy as np
import sounddevice as sd

RATE = 96000
FULL = 32767.0


def find_device():
    """The Multus microphone: WASAPI first (shared, works while MSCC runs; same 16-bit
    samples at 96 kHz), then WDM-KS (raw from the driver, only with MSCC closed)."""
    apis = {h["name"]: i for i, h in enumerate(sd.query_hostapis())}
    for api in ("Windows WASAPI", "Windows WDM-KS"):
        if api not in apis:
            continue
        for i, d in enumerate(sd.query_devices()):
            if d["hostapi"] == apis[api] and "Multus" in d["name"] and d["max_input_channels"] >= 2:
                return i, api
    return None, None


def db(x):
    return 20.0 * np.log10(max(x, 1e-9))


def describe(name, ch):
    x = ch.astype(np.float64)
    rms = np.sqrt(np.mean(x * x))
    near_full = np.mean(np.abs(x) > 0.9 * FULL) * 100.0
    print("%-2s  RMS %6.1f dBFS  DC %7.0f  peak %6d  near full scale %5.1f %%  different values %d"
          % (name, db(rms / FULL), np.mean(x), int(np.max(np.abs(x))), near_full, len(np.unique(ch))))
    return rms, near_full


def shifted_rms(raw):
    """RMS (dBFS) of the same bytes read one byte later: if the stream had slipped by a
    byte, this reading is the sensible one and is far quieter than the original."""
    b = raw.tobytes()[1:]
    b = b[: len(b) // 4 * 4]
    s = np.frombuffer(b, dtype="<i2").astype(np.float64)
    return db(np.sqrt(np.mean(s * s)) / FULL)


def main():
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 1.0
    dev, api = find_device()
    if dev is None:
        sys.exit("Microphone (Multus Sound) not found. Radio plugged in?")
    print("Recording %.1f s from device %d (%s), %d Hz stereo 16-bit..." % (secs, dev, api, RATE))
    try:
        data = sd.rec(int(secs * RATE), samplerate=RATE, channels=2, dtype="int16", device=dev)
        sd.wait()
    except Exception as e:
        sys.exit("Recording failed (%s). Try again with MSCC closed." % e)

    folder = os.path.join(os.environ.get("USERPROFILE", "."), ".local", "mscc")
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, "rx-capture-%s.wav" % datetime.datetime.now().strftime("%Y%m%d-%H%M%S"))
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data.tobytes())
    print("Saved %s" % path)
    print()

    rms_l, full_l = describe("L", data[:, 0])
    rms_r, full_r = describe("R", data[:, 1])
    rms_db = db(max(rms_l, rms_r) / FULL)
    shift_db = shifted_rms(data)
    print("Bytes read one byte later: RMS %.1f dBFS (normal stream: about the same or louder)" % shift_db)

    # Spectrum of I + jQ: noise floor (median) and the strongest line
    iq = data[:, 0].astype(np.float64) + 1j * data[:, 1].astype(np.float64)
    n = min(len(iq), 65536)
    spec = np.abs(np.fft.fftshift(np.fft.fft(iq[:n] * np.hanning(n)))) / n
    spec_db = 20 * np.log10(spec / FULL + 1e-12)
    floor = np.median(spec_db)
    peak_bin = int(np.argmax(spec_db))
    print("Spectrum: floor %.1f dBFS, strongest line %.1f dBFS at %+.0f Hz"
          % (floor, spec_db[peak_bin], (peak_bin - n / 2) * RATE / n))
    print()

    unique = min(len(np.unique(data[:, 0])), len(np.unique(data[:, 1])))
    if unique <= 4:
        print("LOOKS LIKE: stuck data (one or a few repeating values) - converter / I2S not delivering.")
    elif shift_db < rms_db - 20:
        print("LOOKS LIKE: byte slip - the samples are read one byte off (data path in the pill).")
    elif max(full_l, full_r) > 5 or rms_db > -6:
        print("LOOKS LIKE: real but huge input (overload) - receive hardware / converter clipping.")
    else:
        print("LOOKS LIKE: normal receive data.")


if __name__ == "__main__":
    main()
