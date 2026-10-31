#!/usr/bin/env python3
"""
rx-image-meter.py - shows the receive image rejection, read from the spectrum
that sdrcore-recv sends to ms-sdr.

Proof of concept: it only listens and measures. Move the RX IQ slider in the
client yourself and watch the figure.

recv sends its spectrum to ms-sdr on the Pi (UDP 127.0.0.1:8888, opcode 0xD5,
400 points per packet). The script copies those packets off the loopback link
with a raw socket, so it must run ON THE PI with sudo. Nothing is changed in
recv or ms-sdr, and the client keeps working as normal.

    1. Signal generator (or WWV) into the rig, receiver on that band.
    2. Tune so the carrier is 3-20 kHz from the LO point (the VFO -12 kHz
       point on the display), not near it: recv levels the area within
       about 375 Hz of the LO point.
    3. sudo python3 rx-image-meter.py
    4. Move the RX IQ slider; the image rejection follows. Ctrl+C to stop.

The display covers 72 kHz: 24 kHz on one side of the LO point, 48 kHz on the
other. The image is the carrier mirrored about the LO point, so the carrier
must be within 24 kHz of it or the image is off the display.

Levels are spectrum units converted to dB the way the client does (75 units
per dB), relative only. Keep SMOOTHING at one setting during a run.

Options:   --interval  seconds between lines (default 0.5)
           --guard     ignore carriers closer than this to the LO point, Hz
                       (default 1000)
"""

import argparse
import socket
import struct
import sys
import time

MS_SDR_PORT = 8888
CMD_GET_SET_PANADAPTER = 0xD5
POINTS_PER_PACKET = 400
PACKET_SIZE = 2 + 2 * POINTS_PER_PACKET
UNITS_PER_DB = 75.0
FFT_SIZE = 4096
FFT_HZ_PER_BIN = 96000.0 / FFT_SIZE
FFT_FIRST_BIN = 1024            # recv plots FFT bins 1024..4095 (72 kHz)
FFT_LO_POS = FFT_SIZE / 2 - 0.5  # LO point lies between bins 2047 and 2048
ETH_P_IP = 0x0800
PACKET_OUTGOING = 4             # on lo every packet is seen twice; keep the incoming copy
MIRROR_WINDOW = 2               # image level = largest point within +/- this of the mirror


def parse_args():
    p = argparse.ArgumentParser(description="Receive image rejection from the recv spectrum (run on the Pi with sudo).")
    p.add_argument("--interval", type=float, default=0.5, help="seconds between lines (default 0.5)")
    p.add_argument("--guard", type=float, default=1000.0,
                   help="ignore carriers closer than this to the LO point, Hz (default 1000)")
    return p.parse_args()


_ranges_cache = {}


def point_ranges(points):
    """FFT bins [first, end) behind each display point, exactly as recv's doPanadapter picks them."""
    if points not in _ranges_cache:
        ranges = []
        bpos = FFT_FIRST_BIN
        fpos = float(FFT_FIRST_BIN)
        fstep = (FFT_SIZE - FFT_FIRST_BIN) / points
        for _ in range(points):
            bpos = min(max(bpos, 0), FFT_SIZE - 1)
            fpos += fstep
            bend = int(fpos)
            if bend <= bpos:
                bend = bpos + 1
            bend = min(bend, FFT_SIZE)
            ranges.append((bpos, bend))
            bpos = int(fpos)
        _ranges_cache[points] = ranges
    return _ranges_cache[points]


def point_offset_hz(p, points):
    """Distance of display point p from the LO point, Hz (negative = left of it)."""
    first, end = point_ranges(points)[p]
    return ((first + end - 1) / 2.0 - FFT_LO_POS) * FFT_HZ_PER_BIN


def mirror_points(p, points):
    """Display points covering the mirror (about the LO point) of p's bins; [] if off the display."""
    first, end = point_ranges(points)[p]
    m_first, m_last = FFT_SIZE - end, FFT_SIZE - 1 - first
    return [i for i, (a, b) in enumerate(point_ranges(points)) if a <= m_last and b > m_first]


def measure(frame, guard_hz):
    """
    Carrier = strongest point with its mirror on the display and outside the guard.
    Returns (carrier_point, carrier_y, image_point, image_y, noise_y) or None.
    """
    points = len(frame)
    best = None
    for p in range(points):
        if abs(point_offset_hz(p, points)) < guard_hz:
            continue
        m = mirror_points(p, points)
        if not m or m[0] - MIRROR_WINDOW < 0 or m[-1] + MIRROR_WINDOW >= points:
            continue
        if best is None or frame[p] > frame[best]:
            best = p
    if best is None:
        return None
    m = mirror_points(best, points)
    window = range(m[0] - MIRROR_WINDOW, m[-1] + MIRROR_WINDOW + 1)
    image = max(window, key=lambda i: frame[i])
    noise = sorted(frame)[points // 2]
    return best, frame[best], image, frame[image], noise


class FrameAssembler:
    """Joins the 400-point packets (sequence 0..N-1) into whole frames."""

    def __init__(self):
        self.parts = {}
        self.segments = 0   # N, learned from the highest sequence seen

    def add(self, payload):
        if len(payload) != PACKET_SIZE or payload[0] != CMD_GET_SET_PANADAPTER:
            return None
        seq = payload[1]
        if seq == 0 and self.parts:
            self.segments = max(self.segments, max(self.parts) + 1)
            self.parts = {}
        self.parts[seq] = struct.unpack_from("<%dH" % POINTS_PER_PACKET, payload, 2)
        if self.segments and len(self.parts) == self.segments and all(s in self.parts for s in range(self.segments)):
            frame = []
            for s in range(self.segments):
                frame.extend(self.parts[s])
            self.parts = {}
            return frame
        return None


def udp_payload_to_ms_sdr(ip):
    """UDP payload if this IPv4 packet goes to the ms-sdr port, else None."""
    if len(ip) < 20 or (ip[0] >> 4) != 4 or ip[9] != 17:
        return None
    ihl = (ip[0] & 0x0F) * 4
    if len(ip) < ihl + 8:
        return None
    dport = struct.unpack_from("!H", ip, ihl + 2)[0]
    if dport != MS_SDR_PORT:
        return None
    return ip[ihl + 8:]


def open_sniffer():
    try:
        s = socket.socket(socket.AF_PACKET, socket.SOCK_DGRAM, socket.htons(ETH_P_IP))
    except AttributeError:
        sys.exit("Needs Linux: run this on the Pi.")
    except PermissionError:
        sys.exit("Needs root: sudo python3 rx-image-meter.py")
    s.bind(("lo", 0))
    s.settimeout(1.0)
    return s


def main():
    args = parse_args()
    sock = open_sniffer()
    asm = FrameAssembler()
    print("Listening for the recv spectrum on lo, UDP port %d. Ctrl+C to stop." % MS_SDR_PORT)
    sums = None
    count = 0
    best_rej = None
    last = time.monotonic()
    last_frame = time.monotonic()
    try:
        while True:
            try:
                data, addr = sock.recvfrom(4096)
            except socket.timeout:
                data, addr = None, None
            now = time.monotonic()
            if data is not None and addr[2] != PACKET_OUTGOING:
                payload = udp_payload_to_ms_sdr(data)
                frame = asm.add(payload) if payload is not None else None
                if frame is not None:
                    last_frame = now
                    if sums is None or len(sums) != len(frame):
                        sums = [0] * len(frame)
                        count = 0
                    for i, y in enumerate(frame):
                        sums[i] += y
                    count += 1
            if now - last_frame > 3.0:
                print("No spectrum frames (client not running, or transmitting?)")
                last_frame = now
            if now - last < args.interval or not count:
                continue
            last = now
            frame = [v / count for v in sums]
            sums = [0] * len(frame)
            count = 0
            r = measure(frame, args.guard)
            if r is None:
                print("No usable carrier on the display")
                continue
            cp, cy, ip, iy, ny = r
            rej = (cy - iy) / UNITS_PER_DB
            img_over_noise = (iy - ny) / UNITS_PER_DB
            if best_rej is None or rej > best_rej:
                best_rej = rej
            note = "" if img_over_noise > 3.0 else "  (image in the noise)"
            print("carrier %+6.2f kHz  %6.1f dB   image %6.1f dB   rejection %5.1f dB   noise %6.1f dB%s"
                  % (point_offset_hz(cp, len(frame)) / 1000.0, cy / UNITS_PER_DB,
                     iy / UNITS_PER_DB, rej, ny / UNITS_PER_DB, note))
            sys.stdout.flush()
    except KeyboardInterrupt:
        pass
    if best_rej is not None:
        print("\nBest rejection seen: %.1f dB" % best_rej)


if __name__ == "__main__":
    main()
