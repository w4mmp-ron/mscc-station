#!/usr/bin/env python3
"""Read the Black Pill's audio stream counters (CMD_AUDIO_STATS 0xC9, pill 3.11+).

Uses only EP0 vendor control transfers (no interface claim), the same way ms-sdr does,
so it runs with the servers up. Needs python3-usb (pyusb).

  audio-stats.py              one read: all counters since power-up
  audio-stats.py watch [SECS] one line every 0.5 s (default 60 s): what changed in
                              that half second; lines with trouble end in <<<
  audio-stats.py mark         save the counters now as a starting point (file
                              ~/.audio-stats-mark.json); nothing on the pill is reset
                              except the gap max and FIFO min / max
  audio-stats.py since        what changed since the mark (ctrl max and loop max
                              stay since power-up)

Counters (IN = radio -> Pi, RX I/Q; OUT = Pi -> radio, TX I/Q):
  in_pkts     IN packets scheduled (about 1000 per second)
  in_empty    IN packets sent EMPTY (pill had < 95 frames ready) = a 1 ms I/Q gap
  in_gaps     times the USB frame number moved 2+ between IN packets (late / missed)
  in_gap_max  largest such move, in frames (ms)
  in_lvl      IN FIFO fill in bytes, min-max (384 bytes = 1 ms; FIFO holds 3104)
  in_drop     IN FIFO full, a 1 ms block of I/Q thrown away
  out_under   OUT FIFO ran empty (TX I/Q gap)
  out_drop    OUT FIFO nearly full, a 1 ms block thrown away
  out_lvl     OUT FIFO fill in bytes, min-max
  last reset  why the pill last started (3.12+): WATCHDOG = it hung and reset itself
  power-up    (3.5+) codec / LO I2C at power-up: OK, or failed and how many retries
              it took (3.4 retry at enumeration / 3 s, every 500 ms, 10 tries)
  ep0_cut     EP0 replies the USB core ended with data unsent (3.13+)
  usb_irqs    USB interrupts since power-up (3.13+)
  hang record (3.13+, only after a WATCHDOG reset): where the pill was stuck
  ctrl        control transfers (3.17+): setups received / replies queued / done;
              abandoned = a new SETUP came before the last was done (host gave up);
              pill max = SETUP -> reply queued (pill late), host max = queued -> taken
              (host / bus late); slow = SETUP -> done >= 100 ms;
              not ACKed = IN packets the host did not acknowledge (3.18+)
              old reply stopped = unfinished reply cleared at a new SETUP;
              core wait ran out = a USB core wait hit its limit (3.19+)
  loop        main loop timing (3.20+): longest pass, the part that took most of it,
              and the longest time of each part (beat1 TX, beat4 band relays,
              beat6 Si5351, usb = tud_task incl. control requests)
"""
import json
import os
import struct
import sys
import time

import usb.core

VID, PID = 0x16C0, 0x05DC
CMD_AUDIO_STATS = 0xC9
IN = 0xC0
TIMEOUT = 500
NAMES = ["layout", "ms", "in_pkts", "in_empty", "in_gaps", "in_gap_max", "in_lvl_min",
         "in_lvl_max", "in_drop", "out_under", "out_drop", "out_lvl_min", "out_lvl_max", "reset",
         "ep0_cut", "usb_irqs"]
HANG = ["flags", "ms", "irq_at", "irq_last", "pc", "lr", "xpsr", "iabr0", "iabr1", "iabr2",
        "gintsts", "diepempmsk", "diepctl0", "dieptsiz0", "diepint0", "doepint0"]
RESET_BITS = [(0x20, "WATCHDOG"), (0x40, "window watchdog"), (0x80, "low power"), (0x10, "software"),
              (0x08, "power-on"), (0x04, "pin (NRST)"), (0x02, "brown-out")]


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


def read(d, clear):
    try:
        b = bytes(d.ctrl_transfer(IN, CMD_AUDIO_STATS, 1 if clear else 0, 0, 64, TIMEOUT))
    except usb.core.USBError as e:
        sys.exit("read failed (%s): firmware older than 3.11?" % e)
    n = len(b) // 4
    return dict(zip(NAMES, struct.unpack("<%dI" % n, b[:n * 4])))


def read_hang(d):
    try:
        b = bytes(d.ctrl_transfer(IN, CMD_AUDIO_STATS, 0, 1, 64, TIMEOUT))
    except usb.core.USBError:
        return None
    if len(b) < 64:
        return None
    return dict(zip(HANG, struct.unpack("<16I", b[:64])))


CTRL = ["layout", "ms", "setups", "armed", "done", "abandoned", "arm_max", "done_max",
        "slow", "last_slow", "in_toc", "ep0_abort", "wait_to", "data_max"]


def read_ctrl(d):
    try:
        b = bytes(d.ctrl_transfer(IN, CMD_AUDIO_STATS, 0, 2, 64, TIMEOUT))
    except usb.core.USBError:
        return None
    if len(b) < 40 or struct.unpack("<I", b[:4])[0] not in (1 | (10 << 16), 2 | (11 << 16), 3 | (13 << 16), 4 | (14 << 16)):
        return None   # pill older than 3.17 (answers with the audio counters)
    n = len(b) // 4
    return dict(zip(CTRL, struct.unpack("<%dI" % n, b[:n * 4])))


def print_ctrl(c):
    if c is None:
        return
    print("ctrl        setups %d  queued %d  done %d  abandoned %d%s" %
          (c["setups"], c["armed"], c["done"], c["abandoned"],
           "  not ACKed %d" % c["in_toc"] if "in_toc" in c else ""))
    if "ep0_abort" in c:
        print("ctrl fixes  old reply stopped %d  core wait ran out %d" % (c["ep0_abort"], c["wait_to"]))
    print("ctrl max    pill %d ms%s  host %d ms  slow(>=100 ms) %d%s" %
          (c["arm_max"],
           " (after host data %d ms)" % c["data_max"] if "data_max" in c else "",
           c["done_max"], c["slow"],
           "  last %.1f s ago" % ((c["ms"] - c["last_slow"]) / 1000.0) if c["slow"] else ""))


LOOP_PARTS = ["cal_poll", "usb (tud_task)", "audio", "led", "beat0", "beat1 TX_Main",
              "beat2 tx hold / keyer", "beat3", "beat4 Band_Main", "beat5 temps",
              "beat6 si5351", "cw tail"]


def read_loop(d):
    try:
        b = bytes(d.ctrl_transfer(IN, CMD_AUDIO_STATS, 0, 3, 64, TIMEOUT))
    except usb.core.USBError:
        return None
    if len(b) < 64 or struct.unpack("<I", b[:4])[0] != (1 | (16 << 16)):
        return None   # pill older than 3.20
    return struct.unpack("<16I", b[:64])


def print_loop(w):
    if w is None:
        return
    part = w[3] & 0xFF
    print("loop max    %.1f ms, mostly %s, %.1f s ago" %
          (w[2] / 1000.0, LOOP_PARTS[part] if part < len(LOOP_PARTS) else "?",
           ((w[1] & 0xFFFFFF) - (w[3] >> 8)) % 0x1000000 / 1000.0))
    print("loop parts  " + "  ".join("%s %.1f" % (n.split()[0], w[4 + i] / 1000.0)
                                     for i, n in enumerate(LOOP_PARTS)) + "  (ms)")


def print_hang(h):
    if h is None or not h["flags"] & 1:
        print("hang record none")
        return
    exc = h["xpsr"] & 0x1FF
    where = "main loop" if exc == 0 else ("USB interrupt" if exc == 83 else "exception %d" % exc)
    print("hang record (before the last watchdog reset):")
    print("  stuck in    %s, PC 0x%08X  LR 0x%08X" % (where, h["pc"], h["lr"]))
    print("  at          %d ms after power-up, still stuck %.1f s later" %
          (h["ms"], (h["flags"] >> 8) / 10.0 - 0.5))
    print("  usb irqs    %d while stuck" % ((h["irq_last"] - h["irq_at"]) & 0xFFFFFFFF))
    for k in HANG[6:]:
        print("  %-11s 0x%08X" % (k, h[k]))


def lvl(lo, hi):
    return "-" if lo == 0xFFFF else "%d-%d" % (lo, hi)


def reset_text(f):
    return ", ".join(n for b, n in RESET_BITS if f & b) or "unknown"


def i2c_retry_text(r):
    """Pill 3.5+: power-up I2C retry record (stats word 13, bits 16-27)."""
    tries = r & 0xFF
    failed = [n for b, n in ((0x100, "codec"), (0x200, "LO")) if r & b]
    if not failed:
        return "I2C OK at start, no retry needed"
    fixed = [n for b, n in ((0x400, "codec"), (0x800, "LO")) if r & b]
    text = "%s failed at start, %d retr%s" % (" + ".join(failed), tries, "y" if tries == 1 else "ies")
    return text + (", fixed: " + " + ".join(fixed) if fixed else ", NOT fixed")


def once(d):
    s = read(d, False)
    for k in NAMES[1:]:
        if k in s and k not in ("reset", "ep0_cut", "usb_irqs"):
            print("%-11s %d" % (k, s[k]))
    if "reset" in s:   # pill 3.12+
        for k in ("ep0_cut", "usb_irqs"):
            if k in s:
                print("%-11s %d" % (k, s[k]))
        f = s["reset"] & 0xFF
        print("%-11s 0x%02X = %s" % ("last reset", f, reset_text(f)))
        print("%-11s %s" % ("power-up", i2c_retry_text(s["reset"] >> 16)))
    if "ep0_cut" in s:  # pill 3.13+
        print_hang(read_hang(d))
        print_ctrl(read_ctrl(d))
        print_loop(read_loop(d))


def watch(d, secs):
    prev = read(d, True)
    print("time         in_pkts empty gaps gapmax  in_lvl      in_drop | out_under out_drop  out_lvl")
    end = time.time() + secs
    while time.time() < end:
        time.sleep(0.5)
        s = read(d, True)
        dd = {k: (s[k] - prev[k]) & 0xFFFFFFFF for k in
              ("in_pkts", "in_empty", "in_gaps", "in_drop", "out_under", "out_drop")}
        bad = dd["in_empty"] or dd["in_gaps"] or dd["in_drop"] or dd["out_under"] or dd["out_drop"]
        print("%s %7d %5d %4d %6d  %-11s %7d | %9d %8d  %-11s%s" % (
            time.strftime("%H:%M:%S") + ".%d" % (int(time.time() * 10) % 10),
            dd["in_pkts"], dd["in_empty"], dd["in_gaps"], s["in_gap_max"],
            lvl(s["in_lvl_min"], s["in_lvl_max"]), dd["in_drop"],
            dd["out_under"], dd["out_drop"], lvl(s["out_lvl_min"], s["out_lvl_max"]),
            "  <<<" if bad else ""), flush=True)
        prev = s


MARK_FILE = os.path.join(os.path.expanduser("~"), ".audio-stats-mark.json")
SINCE_KEYS = ["in_pkts", "in_empty", "in_gaps", "in_drop", "out_under", "out_drop",
              "ep0_cut", "usb_irqs"]
SINCE_CTRL = ["setups", "abandoned", "in_toc", "ep0_abort", "wait_to", "slow"]


def mark(d):
    """Save the counters as a starting point for 'since'. Also clears the pill's
    gap max and FIFO min / max, so 'since' shows those from the mark on."""
    s = read(d, True)
    c = read_ctrl(d)
    with open(MARK_FILE, "w") as f:
        json.dump({"time": time.time(), "stats": s, "ctrl": c}, f)
    print("mark saved %s (pill up %.1f h): 'since' shows changes from now" %
          (time.strftime("%Y-%m-%d %H:%M:%S"), s["ms"] / 3600000.0))


def since(d):
    try:
        with open(MARK_FILE) as f:
            m = json.load(f)
    except (OSError, ValueError):
        sys.exit("no mark yet: run 'audio-stats.py mark' first")
    s = read(d, False)
    c = read_ctrl(d)
    ms0 = m["stats"]["ms"]
    if s["ms"] < ms0:
        print("pill restarted after the mark: counts below are since that restart")
        base, cbase = {}, {}
    else:
        base, cbase = m["stats"], m["ctrl"] or {}
    secs = (s["ms"] - base.get("ms", 0)) / 1000.0
    print("since mark  %s, %.1f min ago" % (
        time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(m["time"])), secs / 60.0))
    for k in SINCE_KEYS:
        if k in s:
            print("%-11s %d" % (k, (s[k] - base.get(k, 0)) & 0xFFFFFFFF))
    print("%-11s %d ms (since the mark)" % ("in_gap_max", s["in_gap_max"]))
    print("%-11s %s / out %s (since the mark)" % ("in_lvl", lvl(s["in_lvl_min"], s["in_lvl_max"]),
                                                 lvl(s["out_lvl_min"], s["out_lvl_max"])))
    if c is not None:
        dc = {k: (c[k] - cbase.get(k, 0)) & 0xFFFFFFFF for k in SINCE_CTRL if k in c}
        print("ctrl        setups %d  abandoned %d%s%s  slow(>=100 ms) %d" % (
            dc["setups"], dc["abandoned"],
            "  not ACKed %d" % dc["in_toc"] if "in_toc" in dc else "",
            "  old reply stopped %d  core wait ran out %d" % (dc["ep0_abort"], dc["wait_to"])
            if "ep0_abort" in dc else "", dc["slow"]))
    if "reset" in s:
        f = s["reset"] & 0xFF
        print("%-11s 0x%02X = %s" % ("last reset", f, reset_text(f)))
    print_hang(read_hang(d))
    print("(since power-up, not since the mark:)")
    if c is not None:
        print("ctrl max    pill %d ms  host %d ms" % (c["arm_max"], c["done_max"]))
    print_loop(read_loop(d))


def main():
    d = dev()
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "watch":
        watch(d, float(sys.argv[2]) if len(sys.argv) > 2 else 60)
    elif cmd == "mark":
        mark(d)
    elif cmd == "since":
        since(d)
    else:
        once(d)


if __name__ == "__main__":
    main()
