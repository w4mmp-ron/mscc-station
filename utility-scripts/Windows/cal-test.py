#!/usr/bin/env python3
"""Test CMD_CALIBRATION (0xC8) on the radio: PSoC MKII 3.239+ / Black Pill 3.10+.

No save command on either: the radio writes each set value to EEPROM / flash itself.

Uses only EP0 vendor control transfers (no interface claim), the same way ms-sdr does,
so it can run with the servers up. Needs python3-usb (pyusb).
Windows: pip install pyusb; uses libusb0.dll (libusb-win32, as the Windows ms-sdr) or
libusb-1.0.dll, from System32 or next to this script (the same as audio-stats.py).

  cal-test.py valid                 valid masks [RX IQ, TX IQ, power]
  cal-test.py status                [write status, loaded from EEPROM/flash at boot]
  cal-test.py get rx|tx|pwr BAND    one band
  cal-test.py set rx|tx BAND GAIN PHASE
  cal-test.py set pwr BAND VALUE
  cal-test.py save  (or wait)       wait until the radio has written the set values
  cal-test.py dump                  every band of every type
  cal-test.py test                  write a test pattern to bands 0, 4, 11, wait, read back,
                                    and check bad requests are refused (stall)
After "test": power-cycle the radio, then run "check" to see the values came back.
  cal-test.py check

Bands: 0 = 10 m, 1 = 12 m, 2 = 15 m, 3 = 17 m, 4 = 20 m, 5 = 30 m, 6 = 40 m,
       7 = 60 m, 8 = 80 m, 9 = 160 m, 10 = 630 m, 11 = 2200 m
"""
import struct
import sys
import time

import usb.core

VID, PID = 0x16C0, 0x05DC
CMD_CALIBRATION = 0xC8
RX_IQ, TX_IQ, POWER, SAVE, VALID, CLEAR = 1, 2, 3, 4, 5, 6   # CLEAR removed from firmware: stalls
TYPES = {"rx": RX_IQ, "tx": TX_IQ, "pwr": POWER}
NAMES = {RX_IQ: "RX IQ", TX_IQ: "TX IQ", POWER: "power"}
BANDS = ["10m", "12m", "15m", "17m", "20m", "30m", "40m", "60m", "80m", "160m", "630m", "2200m"]
SAVE_STATUS = {0: "idle (nothing written since boot)", 1: "busy (not written yet)", 2: "OK", 3: "FAILED", 4: "refused (old firmware)"}
IN, OUT = 0xC0, 0x40      # vendor, device, IN / OUT
TIMEOUT = 500             # ms, same as ms-sdr

# test pattern: (type, band, values); negative values check the int16 sign survives
PATTERN = [
    (RX_IQ, 0, (-123, 45)), (RX_IQ, 4, (0, -1)), (RX_IQ, 11, (32767, -32768)),
    (TX_IQ, 0, (-80, 12)), (TX_IQ, 4, (-24, 0)), (TX_IQ, 11, (-34, 7)),
    (POWER, 0, (1000,)), (POWER, 4, (0,)), (POWER, 11, (65535,)),
]


def win_backends():
    """Windows: libusb0 first (libusb-win32, the driver the Windows ms-sdr uses), then
    libusb-1.0 (WinUSB / libusbK). DLLs next to this script or in System32."""
    import os
    import usb.backend.libusb0
    import usb.backend.libusb1
    here = os.path.dirname(os.path.abspath(__file__))
    sys32 = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32")
    for mod, dll in ((usb.backend.libusb0, "libusb0.dll"), (usb.backend.libusb1, "libusb-1.0.dll")):
        for path in (os.path.join(here, dll), os.path.join(sys32, dll)):
            if os.path.exists(path):
                be = mod.get_backend(find_library=lambda _n, p=path: p)
                if be is not None:
                    yield be
                    break


def dev():
    if sys.platform == "win32":
        for be in win_backends():
            d = usb.core.find(idVendor=VID, idProduct=PID, backend=be)
            if d is not None:
                return d
        sys.exit("radio not found (16c0:05dc): plugged in? libusb0 or WinUSB driver on Multus Control?")
    d = usb.core.find(idVendor=VID, idProduct=PID)
    if d is None:
        sys.exit("radio not found (16c0:05dc)")
    return d


def wvalue(t, band):
    return (t << 8) | band


def pack(t, vals):
    return struct.pack("<hh", *vals) if t in (RX_IQ, TX_IQ) else struct.pack("<H", *vals)


def unpack(t, data):
    return struct.unpack("<hh", data) if t in (RX_IQ, TX_IQ) else struct.unpack("<H", data)


def get(d, t, band):
    n = 4 if t in (RX_IQ, TX_IQ) else 2
    data = bytes(d.ctrl_transfer(IN, CMD_CALIBRATION, wvalue(t, band), 0, n, TIMEOUT))
    if len(data) != n:
        raise RuntimeError(f"short read {len(data)} of {n}")
    return unpack(t, data)


def put(d, t, band, vals):
    d.ctrl_transfer(OUT, CMD_CALIBRATION, wvalue(t, band), 0, pack(t, vals), TIMEOUT)


def valid(d):
    return struct.unpack("<HHH", bytes(d.ctrl_transfer(IN, CMD_CALIBRATION, wvalue(VALID, 0), 0, 6, TIMEOUT)))


def status(d):
    s = bytes(d.ctrl_transfer(IN, CMD_CALIBRATION, wvalue(SAVE, 0), 0, 2, TIMEOUT))
    return s[0], s[1]


def save(d):
    # PSoC 3.238+ / pill 3.10+: no save command; the radio writes each set value by itself.
    # Wait until it has been idle (not busy) for 0.1 s twice in a row.
    t0 = time.time()
    time.sleep(0.1)
    while True:
        st, _ = status(d)
        if st != 1:
            time.sleep(0.1)
            st, _ = status(d)
            if st != 1:
                return st, time.time() - t0
        if time.time() - t0 > 5:
            return st, time.time() - t0
        time.sleep(0.02)


def show_valid(d):
    for t, m in zip((RX_IQ, TX_IQ, POWER), valid(d)):
        set_bands = [BANDS[b] for b in range(12) if m & (1 << b)]
        print(f"  {NAMES[t]:6} mask 0x{m:04X}  {', '.join(set_bands) or '(none)'}")


def show_status(d):
    st, loaded = status(d)
    print(f"  write: {SAVE_STATUS.get(st, st)}   loaded at boot: {'yes' if loaded else 'no'}")


def stalls(d, label, fn):
    try:
        fn()
    except usb.core.USBError as e:
        print(f"  refused as expected: {label} ({e})")
        return True
    print(f"  NOT refused: {label}")
    return False


def check_pattern(d):
    ok = True
    for t, band, vals in PATTERN:
        got = get(d, t, band)
        good = tuple(got) == tuple(vals)
        ok &= good
        print(f"  {NAMES[t]:6} {BANDS[band]:5} wrote {vals} read {got}  {'ok' if good else 'MISMATCH'}")
    m = valid(d)
    want = (1 << 0) | (1 << 4) | (1 << 11)
    for t, mask in zip((RX_IQ, TX_IQ, POWER), m):
        good = (mask & want) == want
        ok &= good
        print(f"  {NAMES[t]:6} valid 0x{mask:04X}  bands 0, 4, 11 {'set' if good else 'NOT set'}")
    return ok


def test(d):
    print("Before:")
    show_status(d)
    show_valid(d)
    print("Writing test pattern to bands 0 (10m), 4 (20m), 11 (2200m):")
    for t, band, vals in PATTERN:
        put(d, t, band, vals)
    ok = check_pattern(d)
    print("Bad requests (each should be refused):")
    ok &= stalls(d, "band 12", lambda: put(d, RX_IQ, 12, (1, 1)))
    ok &= stalls(d, "type 9", lambda: d.ctrl_transfer(IN, CMD_CALIBRATION, wvalue(9, 0), 0, 4, TIMEOUT))
    ok &= stalls(d, "RX IQ with 2 bytes", lambda: d.ctrl_transfer(OUT, CMD_CALIBRATION, wvalue(RX_IQ, 0), 0, b"\x01\x02", TIMEOUT))
    ok &= stalls(d, "save (removed)", lambda: d.ctrl_transfer(OUT, CMD_CALIBRATION, wvalue(SAVE, 0), 0, b"", TIMEOUT))
    ok &= stalls(d, "save with data", lambda: d.ctrl_transfer(OUT, CMD_CALIBRATION, wvalue(SAVE, 0), 0, b"\x00", TIMEOUT))
    ok &= stalls(d, "clear (removed)", lambda: d.ctrl_transfer(IN, CMD_CALIBRATION, wvalue(CLEAR, 0), 0, 1, TIMEOUT))
    print("Waiting for the radio to write:")
    st, secs = save(d)
    print(f"  {SAVE_STATUS.get(st, st)} after {secs:.2f} s")
    ok &= st == 2
    show_status(d)
    print("RESULT:", "PASS" if ok else "FAIL")
    print("Now power-cycle the radio and run: cal-test.py check")


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    d = dev()
    cmd = a[0]
    if cmd == "valid":
        show_valid(d)
    elif cmd == "status":
        show_status(d)
    elif cmd == "get":
        t, band = TYPES[a[1]], int(a[2])
        print(f"  {NAMES[t]} {BANDS[band]}: {get(d, t, band)}")
    elif cmd == "set":
        t, band = TYPES[a[1]], int(a[2])
        vals = tuple(int(v) for v in a[3:])
        put(d, t, band, vals)
        print(f"  {NAMES[t]} {BANDS[band]} set to {get(d, t, band)} (the radio writes it to EEPROM / flash)")
    elif cmd in ("save", "wait"):
        st, secs = save(d)
        print(f"  {SAVE_STATUS.get(st, st)} after {secs:.2f} s")
    elif cmd == "dump":
        show_status(d)
        show_valid(d)
        for t in (RX_IQ, TX_IQ, POWER):
            print(f"  {NAMES[t]}: " + "  ".join(f"{BANDS[b]}={get(d, t, b)}" for b in range(12)))
    elif cmd == "test":
        test(d)
    elif cmd == "check":
        show_status(d)
        print("RESULT:", "PASS" if check_pattern(d) else "FAIL")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
