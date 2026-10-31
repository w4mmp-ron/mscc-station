#!/usr/bin/env python3
"""
rx-iq-fill.py - fills the receive I/Q values of the bands you could not
calibrate, from a straight line through the bands you did calibrate.

The value rises with frequency (Ron's rig 2026-10-14: 80 m 21, 40 m 27,
30 m 37, 20 m 51), so a line through the calibrated bands gives a better
start than one value copied to every band. Bands above the highest
calibrated one are an extrapolation: a guess, not a measurement.

recv-iq.ini has no mark for "calibrated", so you name those bands:

    python3 rx-iq-fill.py --cal 20,30,40,80

It shows the table and writes only after you answer y (a backup copy of the
old file is kept next to it). 630 m and 2200 m are left alone.

recv keeps the values in memory and loads the file only when it starts. So:
write the file, then restart mscc (or reboot) BEFORE saving any I/Q value
(APPLY, rx-iq-auto.py), or recv writes its old values back.

Default file: Pi ~/.local/mscc/recv-iq.ini, Windows %LOCALAPPDATA%\\MSCC-NET9\\recv-iq.ini
(the Windows servers' file). For the Pi's file from the PC use
--file V:\\.local\\mscc\\recv-iq.ini. No sudo needed.

Options:   --cal       calibrated bands, metres, comma separated (required)
           --file      recv-iq.ini to change
           --dry-run   show the table only
           --yes       write without asking
"""

import argparse
import os
import re
import shutil
import sys
import time

# recv record number -> (band name, frequency used for the line, MHz)
BANDS = {
    0: ("10", 28.3),
    1: ("12", 24.93),
    2: ("15", 21.1),
    3: ("17", 18.1),
    4: ("20", 14.07),
    5: ("30", 10.12),
    6: ("40", 7.05),
    7: ("60", 5.35),
    8: ("80", 3.6),
    9: ("160", 1.85),
}
NOT_ON_RIG = {10: "630", 11: "2200"}
LIMIT = 200     # the client's slider range


def parse_args():
    p = argparse.ArgumentParser(description="Fill uncalibrated receive I/Q bands from the calibrated ones.")
    p.add_argument("--cal", required=True, help="calibrated bands in metres, e.g. 20,30,40,80")
    if sys.platform == "win32":
        default = os.path.join(os.environ.get("LOCALAPPDATA", os.path.expanduser("~")), "MSCC-NET9", "recv-iq.ini")
    else:
        default = os.path.expanduser("~/.local/mscc/recv-iq.ini")
    p.add_argument("--file", default=default, help="recv-iq.ini to change (default %s)" % default)
    p.add_argument("--dry-run", action="store_true", help="show the table, do not write")
    p.add_argument("--yes", action="store_true", help="write without asking")
    return p.parse_args()


def read_ini(path):
    """Lines of the file and {record: (line index, IQ_OFFSET)}."""
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    records = {}
    for i, line in enumerate(lines):
        rec = re.search(r"(?:^|,)\s*RECORD\s*=\s*(-?\d+)", line)
        val = re.search(r"(?:^|,)\s*IQ_OFFSET\s*=\s*(-?\d+)", line)
        if rec and val:
            records[int(rec.group(1))] = (i, int(val.group(1)))
    return lines, records


def set_offset(line, value):
    return re.sub(r"((?:^|,)\s*IQ_OFFSET\s*=\s*)-?\d+", lambda m: m.group(1) + str(value), line, count=1)


def fit_line(points):
    """Least-squares straight line through (MHz, value) points: (slope, intercept)."""
    n = len(points)
    mx = sum(x for x, _ in points) / n
    my = sum(y for _, y in points) / n
    sxx = sum((x - mx) ** 2 for x, _ in points)
    slope = sum((x - mx) * (y - my) for x, y in points) / sxx
    return slope, my - slope * mx


def main():
    args = parse_args()
    by_name = {name: rec for rec, (name, _) in BANDS.items()}
    cal = []
    for b in args.cal.replace(" ", "").split(","):
        b = b.lower().rstrip("m")
        if b not in by_name:
            sys.exit("Unknown band '%s'. Bands: %s" % (b, ", ".join(n for n, _ in BANDS.values())))
        cal.append(by_name[b])
    cal = sorted(set(cal))
    if len(cal) < 2:
        sys.exit("Need at least two calibrated bands for a line.")

    if not os.path.exists(args.file):
        sys.exit("File not found: %s" % args.file)
    lines, records = read_ini(args.file)
    missing = [BANDS[r][0] for r in BANDS if r not in records]
    if missing:
        sys.exit("No line for band(s) %s in %s" % (", ".join(missing), args.file))

    points = [(BANDS[r][1], records[r][1]) for r in cal]
    slope, intercept = fit_line(points)
    top = max(x for x, _ in points)
    bottom = min(x for x, _ in points)

    print("File: %s" % args.file)
    print("Line through %s m: value = %.2f x MHz %+.1f" % (
        ", ".join(BANDS[r][0] for r in cal), slope, intercept))
    print("\n  band    MHz    now   new")
    new_values = {}
    for r in sorted(BANDS, key=lambda r: BANDS[r][1]):
        name, mhz = BANDS[r]
        now = records[r][1]
        fit = intercept + slope * mhz
        if r in cal:
            print("  %4s m %6.2f  %4d  %4d   calibrated (line gives %d)" % (name, mhz, now, now, round(fit)))
            continue
        new = max(-LIMIT, min(LIMIT, int(round(fit))))
        new_values[r] = new
        note = "   guess beyond %s m" % BANDS[cal[0]][0] if mhz > top else (
            "   below %s m" % BANDS[cal[-1]][0] if mhz < bottom else "")
        print("  %4s m %6.2f  %4d  %4d%s" % (name, mhz, now, new, note))
    for r, name in NOT_ON_RIG.items():
        if r in records:
            print("  %4s m          %4d  %4d   not on the rig, left alone" % (name, records[r][1], records[r][1]))

    if args.dry_run:
        print("\nDry run: nothing written.")
        return
    if not args.yes:
        if input("\nWrite these values? [y/N] ").strip().lower() != "y":
            print("Nothing written.")
            return

    backup = "%s.bak-%s" % (args.file, time.strftime("%Y%m%d-%H%M%S"))
    shutil.copy2(args.file, backup)
    for r, value in new_values.items():
        i = records[r][0]
        lines[i] = set_offset(lines[i], value)
    with open(args.file, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print("Written. Old file kept as %s" % backup)
    print("Now restart mscc (or reboot) so recv loads them. Do not save any I/Q value before that.")


if __name__ == "__main__":
    main()
