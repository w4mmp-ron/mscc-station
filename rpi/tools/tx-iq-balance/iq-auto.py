#!/usr/bin/env python3
"""
iq-auto.py - sets the transmit I/Q amplitude balance by itself.

It listens to the I/Q Balance Detector on a 96 kHz sound card (as iqbd-meter.py
does), steps the amplitude value in sdrcore-trans on the Pi, and stops on the
value with the lowest image (the 24 kHz line). The result is saved in iq.ini on
the Pi.

You key the rig; the script does the rest:

    1. Dummy load on the rig, detector on the sound card.
    2. Client: pick the band, open the TX IQ tab and START (carrier on).
    3. python iq-auto.py --device 33 --host proficio
    4. When it says "saved", turn TX off. Next band: back to 2.

Close iqbd-meter.py first: only one program can use the sound card input.
Do not move the client's own slider while this runs.

Options:   --phase      also null the phase (needs the trans build with phase)
           --no-save    find the value, leave it in use, do not write iq.ini
           --list       show the sound devices

Needs:  pip install sounddevice
"""

import argparse
import socket
import struct
import sys
import threading
import time

import numpy as np

CMD_SET_IQ_PHASE = 0x2A
CMD_GET_IQ_PHASE = 0x2B
CMD_SET_IQ_OFFSET = 0x52
CMD_SET_COMMIT_IQ = 0x57
BANDS = ["10 m", "12 m", "15 m", "17 m", "20 m", "30 m", "40 m", "60 m", "80 m", "160 m", "630 m", "2200 m"]
PHASE_STEP_DEG = 0.05
PHASE_LIMIT = 400


class Stop(Exception):
    """The run cannot go on; the message says why."""


def parse_args():
    p = argparse.ArgumentParser(description="Automatic transmit I/Q amplitude balance.")
    p.add_argument("--list", action="store_true", help="list the sound devices and exit")
    p.add_argument("--device", type=int, default=None, help="input device number (see --list)")
    p.add_argument("--host", default="proficio", help="name or address of the Pi (default proficio)")
    p.add_argument("--port", type=int, default=9200, help="sdrcore-trans UDP port (default 9200)")
    p.add_argument("--rate", type=int, default=96000, help="sample rate, Hz (default 96000)")
    p.add_argument("--channel", type=int, default=1, help="input channel, 1 = left (default 1)")
    p.add_argument("--offset", type=float, default=12000.0, help="LO offset of the rig, Hz (default 12000)")
    p.add_argument("--limit", type=int, default=100, help="amplitude value stays within +/- this (default 100)")
    p.add_argument("--settle", type=float, default=2.0,
                   help="wait this long after each step before measuring, s (default 2.0)")
    p.add_argument("--keyed", type=float, default=-90.0,
                   help="carrier counts as present when a tone is above this, dBFS (default -90)")
    p.add_argument("--max-time", type=float, default=600.0, help="give up after this long, s (default 600)")
    p.add_argument("--phase", action="store_true", help="also null the phase")
    p.add_argument("--no-save", action="store_true", help="do not write iq.ini")
    return p.parse_args()


class Rig:
    """sdrcore-trans on the Pi."""

    def __init__(self, host, port):
        try:
            self.addr = (socket.gethostbyname(host), port)
        except OSError as e:
            raise Stop(f"Cannot find {host}: {e}")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def _send(self, opcode, value=0):
        self.sock.sendto(struct.pack("<Bi", opcode, value), self.addr)

    def status(self):
        """(record, phase, amplitude) as sdrcore-trans has them now."""
        self.sock.settimeout(0.01)
        try:
            while True:                 # drop answers to earlier commands
                self.sock.recvfrom(64)
        except OSError:
            pass
        self.sock.settimeout(0.5)
        for _ in range(4):
            self._send(CMD_GET_IQ_PHASE)
            try:
                data, _ = self.sock.recvfrom(64)
            except OSError:
                continue
            if len(data) >= 10 and data[0] == CMD_GET_IQ_PHASE:
                _, record, phase, offset = struct.unpack("<BBii", data[:10])
                return record, phase, offset
        raise Stop(f"No answer from sdrcore-trans at {self.addr[0]}:{self.addr[1]}.")

    def set_amplitude(self, value):
        self._send(CMD_SET_IQ_OFFSET, value)

    def set_phase(self, value):
        self._send(CMD_SET_IQ_PHASE, value)

    def save(self):
        self._send(CMD_SET_COMMIT_IQ)


class Meter:
    """Level of the image and LO leakage lines from the detector, in dBFS."""

    def __init__(self, device, rate, channel, offset):
        try:
            import sounddevice as sd
        except ImportError:
            raise Stop("The sounddevice package is missing. Install it with:  pip install sounddevice")
        self.rate, self.channel, self.offset = rate, channel, offset
        self.lock = threading.Lock()
        self.chunks = []
        self.clip = False
        try:
            self.stream = sd.InputStream(device=device, samplerate=rate, channels=channel,
                                         dtype="float32", callback=self._callback)
            self.stream.start()
        except Exception as e:
            raise Stop(f"Could not open the sound input at {rate} Hz: {e}\n"
                       "Close iqbd-meter.py if it is running, and check --device (see --list).")

    def _callback(self, indata, frames, when, status):
        x = indata[:, self.channel - 1].copy()
        with self.lock:
            self.chunks.append(x)
            if np.max(np.abs(x)) >= 0.99:
                self.clip = True

    def read(self, seconds=0.5):
        """(image dBFS, LO leakage dBFS) over the next 'seconds' of input."""
        need = int(self.rate * seconds)
        with self.lock:
            self.chunks.clear()
            self.clip = False
        deadline = time.monotonic() + seconds + 2.0
        while True:
            with self.lock:
                have = sum(len(c) for c in self.chunks)
            if have >= need:
                break
            if time.monotonic() > deadline:
                raise Stop("The sound card stopped delivering samples.")
            time.sleep(0.02)
        with self.lock:
            x = np.concatenate(self.chunks)[:need]
            clipped = self.clip
        if clipped:
            raise Stop("INPUT CLIPPING - turn the sound card input level down.")
        window = np.hanning(need)
        amp = np.abs(np.fft.rfft(x * window)) * 2.0 / window.sum()
        freqs = np.fft.rfftfreq(need, 1.0 / self.rate)

        def line(centre):
            sel = (freqs >= centre - 150.0) & (freqs <= centre + 150.0)
            return 20.0 * np.log10(max(amp[sel].max(), 1e-12))

        return line(2.0 * self.offset), line(self.offset)

    def close(self):
        self.stream.stop()
        self.stream.close()


def find_null(name, set_value, measure, start, low, high, show):
    """Step a control to the lowest image. Returns (best value, its image dBFS, all readings)."""
    seen = {}

    def at(x):
        if x not in seen:
            set_value(x)
            seen[x] = measure()
            show(f"  {name} {x:+5d}   image {seen[x][0]:7.1f} dBFS   LO leakage {seen[x][1]:7.1f} dBFS")
        return seen[x][0]

    def clamp(x):
        return max(low, min(high, int(round(x))))

    # two passes of seven points, 8 steps apart and then 2
    centre = start
    for spacing in (8, 2):
        for _ in range(6):
            xs = sorted({clamp(centre + k * spacing) for k in range(-3, 4)})
            volts = [10.0 ** (at(x) / 20.0) for x in xs]
            lowest = xs[int(np.argmin(volts))]
            if lowest not in (xs[0], xs[-1]) or lowest in (low, high) or len(xs) < 3:
                # the null is inside these points: the image power against the
                # value is a parabola, its lowest point is where to look next
                centre = lowest
                a, b, _ = np.polyfit(xs, np.square(volts), 2)
                if a > 0 and xs[0] <= -b / (2.0 * a) <= xs[-1]:
                    centre = clamp(-b / (2.0 * a))
                break
            # the null is off to one side: far from it the image voltage falls in
            # a straight line, so go to where that line reaches zero
            slope, level = np.polyfit(xs, volts, 1)
            centre = lowest
            if slope != 0:
                zero = clamp(-level / slope)
                if (zero - lowest) * (lowest - xs[3]) > 0:
                    centre = zero
    # then single steps, following the null if it is at the edge of what was tried
    for _ in range(10):
        for x in range(clamp(centre - 3), clamp(centre + 3) + 1):
            at(x)
        best = min(seen, key=lambda x: seen[x][0])
        if (best - 1 in seen or best == low) and (best + 1 in seen or best == high):
            break
        centre = best
    best = min(seen, key=lambda x: seen[x][0])
    return best, seen[best][0], seen


def run(rig, meter, args, show=print):
    started = time.monotonic()
    record, phase0, amp0 = rig.status()
    if record >= len(BANDS):
        raise Stop("sdrcore-trans has no band yet. Pick the band in the client, open the TX IQ tab and START.")
    band = BANDS[record]
    image0, lo0 = meter.read(0.5)
    if max(image0, lo0) < args.keyed:
        raise Stop("No carrier on the detector. Key the rig (TX IQ tab, START) and run again.")
    show(f"{band}: amplitude {amp0:+d}, phase {phase0:+d}, image {image0:.1f} dBFS, LO leakage {lo0:.1f} dBFS")

    def measure():
        if time.monotonic() - started > args.max_time:
            raise Stop(f"Not finished after {args.max_time:.0f} s.")
        time.sleep(args.settle)
        last = meter.read()
        for _ in range(6):              # until two readings in a row agree
            now = meter.read()
            if max(now) < args.keyed:
                raise Stop("The carrier went away.")
            if abs(now[0] - last[0]) < 0.5:
                break
            last = now
        return (now[0] + last[0]) / 2.0, (now[1] + last[1]) / 2.0

    amp, ph = amp0, phase0
    try:
        # the amplitude opcode only works while a TX IQ session is open: check it took
        probe = amp0 - 1 if amp0 > -args.limit else amp0 + 1
        rig.set_amplitude(probe)
        time.sleep(0.2)
        if rig.status()[2] != probe:
            raise Stop("sdrcore-trans did not take the amplitude value. "
                       "The TX IQ tab must be open and started in the client.")
        show("amplitude:")
        amp, image, _ = find_null("amplitude", rig.set_amplitude, measure, amp0, -args.limit, args.limit, show)
        rig.set_amplitude(amp)
        if args.phase:
            show("phase:")
            ph, image, _ = find_null("phase", rig.set_phase, measure, phase0, -PHASE_LIMIT, PHASE_LIMIT, show)
            rig.set_phase(ph)
            show("amplitude again:")
            amp, image, _ = find_null("amplitude", rig.set_amplitude, measure, amp, -args.limit, args.limit, show)
            rig.set_amplitude(amp)
        image, lo = measure()
    except (Stop, KeyboardInterrupt):
        rig.set_amplitude(amp0)
        if args.phase:
            rig.set_phase(phase0)
        show(f"Stopped. Put back amplitude {amp0:+d}" + (f", phase {phase0:+d}." if args.phase else "."))
        raise

    show("")
    show(f"{band}: amplitude {amp0:+d} -> {amp:+d}" +
         (f", phase {phase0:+d} -> {ph:+d} ({ph * PHASE_STEP_DEG:+.2f} deg)" if args.phase else "") +
         f", image {image0:.1f} -> {image:.1f} dBFS ({image - lo:+.1f} dB against LO leakage)")
    if abs(amp) >= args.limit:
        show(f"WARNING: the value is at the end of the range (+/-{args.limit}); the null may be beyond it.")
    if args.no_save:
        show("Not saved (--no-save). The value stays in use until the next band change.")
    else:
        rig.save()
        time.sleep(0.3)
        show(f"saved in iq.ini on the Pi ({time.strftime('%H:%M:%S')}). Turn TX off.")
    return amp, ph, image


def main():
    args = parse_args()
    if args.list:
        import sounddevice as sd
        print(sd.query_devices())
        return
    if 2 * args.offset + 150 >= args.rate / 2:
        sys.exit(f"A {args.rate} Hz sample rate cannot show the image tone at {2 * args.offset:.0f} Hz.")
    meter = None
    try:
        rig = Rig(args.host, args.port)
        meter = Meter(args.device, args.rate, args.channel, args.offset)
        run(rig, meter, args)
    except Stop as e:
        sys.exit(str(e))
    except KeyboardInterrupt:
        sys.exit("Interrupted.")
    finally:
        if meter is not None:
            meter.close()


if __name__ == "__main__":
    main()
