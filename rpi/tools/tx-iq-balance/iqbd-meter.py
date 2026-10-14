#!/usr/bin/env python3
"""
iqbd-meter.py - live level of the two beat tones from the I/Q Balance Detector.

The detector output goes to the LINE input of a sound card running at 96 kHz.
For a rig with a 12 kHz offset LO, a plain carrier gives:

    about 12 kHz   LO leakage  (LO beating with the wanted signal)
    about 24 kHz   image       (image beating with the wanted signal)

Adjust the transmit I/Q balance for the lowest IMAGE reading.

Needs:  pip install sounddevice      (numpy and matplotlib are already there)

Run:    python iqbd-meter.py --list          show the sound devices
        python iqbd-meter.py --device 3      use input device 3
        python iqbd-meter.py --device 3 --offset 12000 --width 3000

Levels are in dB below the sound card's full scale (dBFS). They are relative:
good for finding the null, not absolute figures.
"""

import argparse
import sys
import time
from collections import deque

import numpy as np

try:
    import sounddevice as sd
except ImportError:
    sys.exit("The sounddevice package is missing. Install it with:  pip install sounddevice")

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


def parse_args():
    p = argparse.ArgumentParser(description="Live level of the I/Q Balance Detector beat tones.")
    p.add_argument("--list", action="store_true", help="list the sound devices and exit")
    p.add_argument("--device", type=int, default=None, help="input device number (see --list)")
    p.add_argument("--rate", type=int, default=96000, help="sample rate, Hz (default 96000)")
    p.add_argument("--channel", type=int, default=1, help="input channel, 1 = left (default 1)")
    p.add_argument("--fft", type=int, default=65536, help="FFT length (default 65536)")
    p.add_argument("--average", type=int, default=4, help="spectra averaged per reading (default 4)")
    p.add_argument("--offset", type=float, default=12000.0, help="LO offset of the rig, Hz (default 12000)")
    p.add_argument("--width", type=float, default=3000.0,
                   help="search this far each side of the expected tone, Hz (default 3000)")
    p.add_argument("--keyed", type=float, default=-90.0,
                   help="carrier counts as present when either tone is above this, dBFS (default -90)")
    return p.parse_args()


def main():
    args = parse_args()
    if args.list:
        print(sd.query_devices())
        return

    rate, nfft = args.rate, args.fft
    if 2 * args.offset + args.width >= rate / 2:
        sys.exit(f"A {rate} Hz sample rate cannot show the image tone at {2 * args.offset:.0f} Hz.")

    window = np.hanning(nfft)
    scale = 2.0 / window.sum()          # a full-scale sine reads 0 dBFS
    freqs = np.fft.rfftfreq(nfft, 1.0 / rate)
    hop = nfft // 2
    ring = np.zeros(nfft + hop * (args.average - 1), dtype=np.float32)
    state = {"clip": False, "status": "", "got": 0}

    def callback(indata, frames, time, status):
        if status:
            state["status"] = str(status)
        x = indata[:, args.channel - 1]
        state["got"] += len(x)
        if np.max(np.abs(x)) >= 0.99:
            state["clip"] = True
        n = len(x)
        if n >= len(ring):
            ring[:] = x[-len(ring):]
        else:
            ring[:-n] = ring[n:]
            ring[-n:] = x

    def spectrum_db():
        acc = np.zeros(len(freqs))
        for k in range(args.average):
            seg = ring[k * hop:k * hop + nfft]
            acc += np.abs(np.fft.rfft(seg * window)) ** 2
        amp = np.sqrt(acc / args.average) * scale
        return 20.0 * np.log10(np.maximum(amp, 1e-12))

    def peak(db, centre):
        sel = (freqs >= centre - args.width) & (freqs <= centre + args.width)
        i = np.argmax(db[sel])
        return freqs[sel][i], db[sel][i]

    lo_f, im_f = args.offset, 2.0 * args.offset

    fig, ax = plt.subplots(figsize=(10, 6))
    fig.canvas.manager.set_window_title("I/Q Balance Detector")
    line, = ax.plot(freqs / 1000.0, np.full(len(freqs), -140.0), lw=0.8)
    ax.set_xlim(0, min(rate / 2000.0, (im_f + 4 * args.width) / 1000.0))
    ax.set_ylim(-140, 0)
    ax.set_xlabel("kHz")
    ax.set_ylabel("dBFS")
    ax.grid(True, alpha=0.3)
    for f, name in ((lo_f, "LO leakage"), (im_f, "image")):
        ax.axvspan((f - args.width) / 1000.0, (f + args.width) / 1000.0, alpha=0.12)
        ax.text(f / 1000.0, -137, name, ha="center", va="bottom")
    lo_mark, = ax.plot([], [], "v", ms=9)
    im_mark, = ax.plot([], [], "v", ms=9)
    readout = ax.text(0.02, 0.97, "", transform=ax.transAxes, va="top", family="monospace", fontsize=14)
    lowest = {"image": None, "rel": None}
    # the hold runs only with the carrier present, and skips the readings that
    # still have part of a key-down or key-up in the buffer
    settle = len(ring) / rate + 0.5
    keyed = {"since": None, "recent": deque()}

    def on_key(event):
        if event.key == "r":
            lowest["image"] = None
            lowest["rel"] = None

    fig.canvas.mpl_connect("key_press_event", on_key)

    def update(_):
        db = spectrum_db()
        line.set_ydata(db)
        lf, ld = peak(db, lo_f)
        mf, md = peak(db, im_f)
        lo_mark.set_data([lf / 1000.0], [ld + 4])
        im_mark.set_data([mf / 1000.0], [md + 4])
        now = time.monotonic()
        carrier = state["got"] >= len(ring) and max(ld, md) >= args.keyed
        if carrier:
            if keyed["since"] is None:
                keyed["since"] = now
            keyed["recent"].append((now, md, md - ld))
            while keyed["recent"] and keyed["recent"][0][0] <= now - settle:
                t, v, rel = keyed["recent"].popleft()
                if t < keyed["since"] + settle:
                    continue
                if lowest["image"] is None or v < lowest["image"]:
                    lowest["image"] = v
                # against LO leakage: does not move with carrier power
                if lowest["rel"] is None or rel < lowest["rel"]:
                    lowest["rel"] = rel
        else:
            keyed["since"] = None
            keyed["recent"].clear()
        low = "   --- " if lowest["image"] is None else f"{lowest['image']:7.1f}"
        low_rel = "   --- " if lowest["rel"] is None else f"{lowest['rel']:+7.1f}"
        text = (f"IMAGE       {md:7.1f} dBFS  at {mf / 1000.0:6.2f} kHz   lowest {low}  (r = reset)\n"
                f"LO leakage  {ld:7.1f} dBFS  at {lf / 1000.0:6.2f} kHz\n"
                f"image is {md - ld:+6.1f} dB against LO leakage     lowest {low_rel}")
        if not carrier:
            text += "\nno carrier - lowest hold paused"
        if state["clip"]:
            text += "\nINPUT CLIPPING - turn the input level down"
            state["clip"] = False
        if state["status"]:
            text += "\n" + state["status"]
            state["status"] = ""
        readout.set_text(text)
        return line, lo_mark, im_mark, readout

    try:
        stream = sd.InputStream(device=args.device, samplerate=rate, channels=args.channel,
                                dtype="float32", callback=callback)
    except Exception as e:
        sys.exit(f"Could not open the sound input at {rate} Hz: {e}\n"
                 f"Run with --list, pick the line input with --device, and set it to {rate} Hz in Windows.")

    with stream:
        anim = FuncAnimation(fig, update, interval=250, blit=False, cache_frame_data=False)
        plt.show()
        del anim


if __name__ == "__main__":
    main()
