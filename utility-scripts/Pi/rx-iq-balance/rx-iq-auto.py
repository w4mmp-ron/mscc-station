#!/usr/bin/env python3
"""
rx-iq-auto.py - sets the receive I/Q amplitude balance by itself.

It reads the image rejection from the recv spectrum (as rx-image-meter.py
does), steps the I/Q OFFSET value in sdrcore-recv, and keeps the value with
the best rejection. The result is saved in ~/.local/mscc/recv-iq.ini.

Runs ON THE PI with sudo (it reads the spectrum off the loopback link).
rx-image-meter.py must be in the same folder.

    1. Signal generator into the rig, strong level.
    2. Client: RX IQ tab, START (the radio tunes to the band's last frequency).
    3. Set the generator 6 kHz below the frequency shown on the tab.
    4. sudo python3 rx-iq-auto.py
    5. When it says "saved", press START again to end the session.
       Next band: back to 2.

Do not move the client's I/Q OFFSET slider while this runs (it would send its
own value), and do not press APPLY or ZERO afterwards: the script saves.
The client's slider does not show the script's value.

Search: every --coarse steps over +/- --limit, then single steps around the
best. One step is 0.05 % gain.

Options:   --no-save    find the value, leave it in use, do not write the file
           --limit      search range +/- (default 200, the client's range)
           --settle     wait after each step before measuring, s (default 0.5)
           --measure    average this long for each value, s (default 1.0)
"""

import argparse
import importlib.util
import os
import socket
import struct
import sys
import time

CMD_SET_IQ_OFFSET = 0x52
CMD_SET_COMMIT_IQ = 0x57
RECV_ADDR = ("127.0.0.1", 9000)


def load_meter():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rx-image-meter.py")
    if not os.path.exists(path):
        sys.exit("rx-image-meter.py must be in the same folder as this script.")
    spec = importlib.util.spec_from_file_location("rx_image_meter", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


meter = load_meter()


class Stop(Exception):
    """The run cannot go on; the message says why."""


def parse_args():
    p = argparse.ArgumentParser(description="Automatic receive I/Q amplitude balance (run on the Pi with sudo).")
    p.add_argument("--no-save", action="store_true", help="do not write recv-iq.ini")
    p.add_argument("--limit", type=int, default=200, help="search range +/- (default 200)")
    p.add_argument("--coarse", type=int, default=10, help="coarse step (default 10)")
    p.add_argument("--settle", type=float, default=0.5, help="wait after each step, s (default 0.5)")
    p.add_argument("--measure", type=float, default=1.0, help="average time per value, s (default 1.0)")
    p.add_argument("--guard", type=float, default=1000.0,
                   help="ignore carriers closer than this to the LO point, Hz (default 1000)")
    return p.parse_args()


class Rig:
    def __init__(self, args):
        self.args = args
        self.sniff = meter.open_sniffer()
        self.cmd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.asm = meter.FrameAssembler()
        self.value = None

    def send(self, opcode, value):
        self.cmd.sendto(struct.pack("<Bi", opcode, value), RECV_ADDR)

    def set_value(self, value):
        self.send(CMD_SET_IQ_OFFSET, value)
        self.value = value

    def frames_for(self, seconds):
        """Average spectrum frame over this many seconds (frames before the start are dropped)."""
        sums, count = None, 0
        end = time.monotonic() + seconds
        no_frame_since = time.monotonic()
        while time.monotonic() < end or count == 0:
            try:
                data, addr = self.sniff.recvfrom(4096)
            except socket.timeout:
                data = None
            if time.monotonic() - no_frame_since > 3.0:
                raise Stop("No spectrum frames (client not running, or transmitting?)")
            if data is None or addr[2] == meter.PACKET_OUTGOING:
                continue
            payload = meter.udp_payload_to_ms_sdr(data)
            frame = self.asm.add(payload) if payload is not None else None
            if frame is None:
                continue
            no_frame_since = time.monotonic()
            if sums is None or len(sums) != len(frame):
                sums, count = [0] * len(frame), 0
            for i, y in enumerate(frame):
                sums[i] += y
            count += 1
        return [v / count for v in sums]

    def measure(self, value, seconds=None):
        """Set the value, settle, return (rejection dB, image above noise dB, carrier offset Hz)."""
        self.set_value(value)
        time.sleep(self.args.settle)
        self.drain()
        frame = self.frames_for(seconds or self.args.measure)
        r = meter.measure(frame, self.args.guard)
        if r is None:
            raise Stop("No usable carrier on the display (generator 3-20 kHz from the LO point?)")
        cp, cy, ip, iy, ny = r
        u = meter.UNITS_PER_DB
        return (cy - iy) / u, (iy - ny) / u, meter.point_offset_hz(cp, len(frame))

    def drain(self):
        """Drop packets queued during the settle time."""
        self.sniff.setblocking(False)
        try:
            while True:
                self.sniff.recv(4096)
        except (BlockingIOError, socket.error):
            pass
        finally:
            self.sniff.settimeout(1.0)
        self.asm = meter.FrameAssembler()


def show(value, r):
    rej, over, _ = r
    note = "" if over > 3.0 else "  (image in the noise)"
    print("  I/Q OFFSET %+4d   rejection %5.1f dB%s" % (value, rej, note))
    sys.stdout.flush()


def sweep(rig, values, results, seconds=None):
    for v in values:
        if v in results and seconds is None:
            continue
        results[v] = rig.measure(v, seconds)
        show(v, results[v])
    return max(results, key=lambda k: results[k][0])


def main():
    args = parse_args()
    rig = Rig(args)
    results = {}
    try:
        first = rig.measure(0)
        print("Carrier %+.2f kHz from the LO point." % (first[2] / 1000.0))
        print("Coarse search, every %d over +/-%d:" % (args.coarse, args.limit))
        values = list(range(-args.limit, args.limit + 1, args.coarse))
        best = sweep(rig, values, results)
        spread = max(r[0] for r in results.values()) - min(r[0] for r in results.values())
        if spread < 3.0:
            raise Stop("Rejection hardly changes (%.1f dB): recv is not taking the value. "
                       "Is START pressed on the RX IQ tab?" % spread)

        print("Fine search around %+d:" % best)
        lo, hi = max(-args.limit, best - args.coarse), min(args.limit, best + args.coarse)
        best = sweep(rig, range(lo, hi + 1, 2), results)
        print("Single steps around %+d (longer average):" % best)
        lo, hi = max(-args.limit, best - 2), min(args.limit, best + 2)
        fine = {}
        best = sweep(rig, range(lo, hi + 1), fine, seconds=args.measure * 2)

        final = rig.measure(best, args.measure * 2)
        print("\nBest I/Q OFFSET %+d: rejection %.1f dB" % (best, final[0]))
        if final[1] <= 3.0:
            print("The image is in the noise at this value: a stronger generator gives a truer null.")
        if args.no_save:
            print("Left in use, NOT saved (--no-save).")
        else:
            rig.send(CMD_SET_COMMIT_IQ, 0)
            print("Saved (recv-iq.ini). Press START on the RX IQ tab to end the session.")
    except KeyboardInterrupt:
        print("\nStopped.")
        if rig.value is not None:
            print("I/Q OFFSET %+d is still in use, NOT saved. Move the client's slider to put a value back."
                  % rig.value)
    except Stop as e:
        print("\n" + str(e))
        if rig.value is not None:
            print("I/Q OFFSET %+d is in use, NOT saved. Move the client's slider to put a value back."
                  % rig.value)
        sys.exit(1)


if __name__ == "__main__":
    main()
