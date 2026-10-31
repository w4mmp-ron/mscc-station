#!/usr/bin/env python3
"""
Copy the I/Q calibration in the Pi's ini files into the radio's calibration store.

    python3 ~/utilities/iq-scripts/iq-to-radio.py              TX and RX
    python3 ~/utilities/iq-scripts/iq-to-radio.py tx           TX only (iq.ini)
    python3 ~/utilities/iq-scripts/iq-to-radio.py rx           RX only (recv-iq.ini)
    python3 ~/utilities/iq-scripts/iq-to-radio.py --dry-run    show what would be sent, send nothing
    python3 ~/utilities/iq-scripts/iq-to-radio.py --show       only show what the radio holds now

Reads ~/.local/mscc/iq.ini (TX: IQ_OFFSET, IQ_PHASE) and ~/.local/mscc/recv-iq.ini
(RX: IQ_OFFSET, phase 0), sends each band to the radio with 0xC8 (type 2 = TX I/Q,
type 1 = RX I/Q, 4 bytes: int16 offset, int16 phase, little endian, the same as ms-sdr's
APPLY), then reads the bands back and prints a table.

The ini RECORD number is the radio band number (0 = 10 m ... 9 = 160 m, 10 = 630 m,
11 = 2200 m). Bands with offset 0 and phase 0 (never calibrated) are skipped.
Safe with the servers up; do not run it while transmitting.
At the next server start ms-sdr reports "N band(s) from radio".
"""
import os
import struct
import sys

import usb.core

VID, PID = 0x16C0, 0x05DC
IN, OUT = 0xC0, 0x40
CMD_CALIBRATION = 0xC8
TYPE_RX_IQ, TYPE_TX_IQ, TYPE_VALID = 1, 2, 5
TIMEOUT = 1000
BANDS = ["10", "12", "15", "17", "20", "30", "40", "60", "80", "160", "630", "2200"]
INI_DIR = os.path.expanduser("~/.local/mscc")


def read_ini(name):
    """{record: (offset, phase)} from an iq ini file."""
    path = os.path.join(INI_DIR, name)
    values = {}
    try:
        with open(path) as f:
            for line in f:
                fields = {}
                for part in line.strip().split(","):
                    if "=" in part:
                        k, v = part.split("=", 1)
                        fields[k.strip()] = v.strip()
                if "RECORD" not in fields or "IQ_OFFSET" not in fields:
                    continue
                rec = int(fields["RECORD"])
                if 0 <= rec < len(BANDS):
                    values[rec] = (int(fields["IQ_OFFSET"]), int(fields.get("IQ_PHASE", "0")))
    except FileNotFoundError:
        print("%s not found" % path)
    return values


def radio():
    d = usb.core.find(idVendor=VID, idProduct=PID)
    if d is None:
        sys.exit("radio not found (16c0:05dc)")
    return d


def read_masks(d):
    b = bytes(d.ctrl_transfer(IN, CMD_CALIBRATION, TYPE_VALID << 8, 0, 6, TIMEOUT))
    return struct.unpack("<HHH", b)    # RX I/Q, TX I/Q, power


def read_band(d, cal_type, rec):
    b = bytes(d.ctrl_transfer(IN, CMD_CALIBRATION, (cal_type << 8) | rec, 0, 4, TIMEOUT))
    return struct.unpack("<hh", b)


def write_band(d, cal_type, rec, offset, phase):
    data = struct.pack("<hh", offset, phase)
    return d.ctrl_transfer(OUT, CMD_CALIBRATION, (cal_type << 8) | rec, 0, data, TIMEOUT)


def show(d, label, cal_type, mask, wanted):
    print("\n%s I/Q          file (offset, phase)   radio (offset, phase)" % label)
    for rec, band in enumerate(BANDS):
        held = bool(mask & (1 << rec))
        r = read_band(d, cal_type, rec) if held else None
        f = wanted.get(rec)
        if f is None and r is None:
            continue
        fs = "%5d %5d" % f if f else "    -     -"
        rs = "%5d %5d" % r if r else "  not held "
        if f and r:
            mark = "OK" if r == f else "DIFFERENT"
        elif f:
            mark = "not in radio"
        else:
            mark = "radio only"
        print("  %5s m     %s           %s     %s" % (band, fs, rs, mark))


def main():
    args = [a.lower() for a in sys.argv[1:]]
    dry = "--dry-run" in args
    only_show = "--show" in args
    do_tx = "rx" not in args or "tx" in args
    do_rx = "tx" not in args or "rx" in args

    jobs = []
    if do_tx:
        tx = {r: v for r, v in read_ini("iq.ini").items() if v != (0, 0)}
        jobs.append(("TX", TYPE_TX_IQ, tx))
    if do_rx:
        rx = {r: (v[0], 0) for r, v in read_ini("recv-iq.ini").items() if v[0] != 0}
        jobs.append(("RX", TYPE_RX_IQ, rx))

    d = radio()
    if not only_show:
        for label, cal_type, values in jobs:
            for rec in sorted(values):
                offset, phase = values[rec]
                if dry:
                    print("would send %s %4s m: offset %d, phase %d" % (label, BANDS[rec], offset, phase))
                    continue
                n = write_band(d, cal_type, rec, offset, phase)
                if n != 4:
                    print("%s %s m: write FAILED (%s)" % (label, BANDS[rec], n))
        if dry:
            return

    rx_mask, tx_mask, _ = read_masks(d)
    for label, cal_type, values in jobs:
        show(d, label, cal_type, tx_mask if cal_type == TYPE_TX_IQ else rx_mask, values)


if __name__ == "__main__":
    main()
