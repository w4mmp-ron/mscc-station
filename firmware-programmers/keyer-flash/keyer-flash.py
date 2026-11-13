#!/usr/bin/env python3
"""
keyer-flash.py: update the PIC keyer on the Proficio daughter board over USB.

    keyer-flash.py Keyer-MCU-app-YYYYMMDD.hex    update the keyer
    keyer-flash.py --check                       is the keyer app or bootloader answering?
    keyer-flash.py --exit                        leave the bootloader, run the keyer app

Path: host -> USB opcode 0x9F (CMD_KEYER_BOOT) -> radio -> I2C -> PIC.
Radio firmware with the keyer update: Black Pill 3.16, Proficio MKII PTT 3.244 / ATU 4.245, Geminus MKII 2.154, Ultimus MKII PTT 8.244 / ATU 7.245 or later.
The PIC needs the I2C bootloader (put on once with the PICkit, Keyer-MCU-full-*.hex,
bootloader version 2 or later). Only the app is written (0x400-0x3FFF); the bootloader,
the config words and the EEPROM (keyer settings, CQ memories) are left alone.

The MSCC servers must be stopped first (Pi: mscc stop; Windows: close them). If any
server is running, it says so and stops without touching the keyer.
If an update is cut short, the PIC stays in the bootloader: run the same command again.
Needs python3-usb (pyusb).
"""
import os
import subprocess
import sys
import time

import usb.core

VID, PID = 0x16C0, 0x05DC
CMD_KEYER_BOOT = 0x9F
OUT, IN = 0x40, 0xC0
TIMEOUT = 500

APP_ADDR = 0x40         # keyer app (I2C 7-bit)
BOOT_ADDR = 0x5C        # keyer bootloader
APP_START = 0x400       # words
FLASH_END = 0x4000
ROW = 32                # words per row
PIECE = 16              # bytes per load
MIN_BOOT_VERSION = 2

KB_PENDING, KB_OK, KB_FAIL, KB_REFUSED = 1, 2, 3, 4
BOOT_STATUS = {0: "OK", 1: "bad address", 2: "bad sum", 3: "bad length",
               4: "read-back differs", 5: "unknown command"}
SERVERS = ["sdrcore-recv", "sdrcore-trans", "ms-sdr"]


class Fail(Exception):
    pass


# Output hooks: the GUI (keyer-flash-gui.py) replaces these.
def say(text):
    print(text)


def progress(done, total, phase):
    """phase 'write' or 'check'; the command line prints its own lines instead."""
    pass


# ---- USB ------------------------------------------------------------------

def win_backends():
    """Windows: libusb0 first (libusb-win32, the driver the Windows ms-sdr uses), then
    libusb-1.0 (WinUSB / libusbK). DLLs next to this script or in System32."""
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


def i2c(d, addr, wr=b"", rd=0):
    """One I2C transfer through the radio: write wr (up to 18 bytes), then read rd
    (up to 16) bytes. Nothing to write or read = address probe. Returns bytes read."""
    try:
        d.ctrl_transfer(OUT, CMD_KEYER_BOOT, addr, rd, bytes(wr) if wr else None, TIMEOUT)
    except usb.core.USBError as e:
        raise Fail("radio refused the request (%s); radio firmware too old?" % e)
    t0 = time.time()
    while True:
        r = bytes(d.ctrl_transfer(IN, CMD_KEYER_BOOT, 0, 0, 2 + 16, TIMEOUT))
        if r[0] != KB_PENDING:
            break
        if time.time() - t0 > 1.0:
            raise Fail("radio did not finish the I2C transfer")
        time.sleep(0.002)
    if r[0] == KB_OK:
        return r[2:2 + r[1]]
    if r[0] == KB_REFUSED:
        raise Fail("radio refused the I2C request")
    raise Fail("no answer on I2C 0x%02X" % addr)


def answers(d, addr):
    try:
        i2c(d, addr)
        return True
    except Fail:
        return False


def boot_status(d):
    s = i2c(d, BOOT_ADDR, rd=4)
    return s[0], s[1], (s[2] << 8) | s[3]


def supported(d):
    """Radio firmware knows 0x9F?"""
    try:
        d.ctrl_transfer(IN, CMD_KEYER_BOOT, 0, 0, 2 + 16, TIMEOUT)
        return True
    except usb.core.USBError:
        return False


# ---- servers --------------------------------------------------------------

WIN_SERVERS = ("recv.exe", "trans.exe")      # plus any ms-sdr* / sdrcore* image


def servers_running():
    """Names of the MSCC servers running now ([] = none)."""
    found = []
    if sys.platform == "win32":
        try:
            # CREATE_NO_WINDOW: no console window flashing up from the windowed .exe
            out = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True,
                                 text=True, creationflags=0x08000000).stdout
        except OSError:
            return found
        for line in out.splitlines():
            name = line.split(",")[0].strip('"')
            low = name.lower()
            if low in WIN_SERVERS or low.startswith("ms-sdr") or low.startswith("sdrcore"):
                if name not in found:
                    found.append(name)
        return found
    if os.path.isdir("/proc"):
        for p in os.listdir("/proc"):
            if p.isdigit():
                try:
                    with open("/proc/%s/comm" % p) as f:
                        name = f.read().strip()
                    if name in SERVERS and name not in found:
                        found.append(name)
                except OSError:
                    pass
    return found


def servers_message(names):
    how = "Close them" if sys.platform == "win32" else "Stop them (mscc stop)"
    return ("The MSCC servers are running (%s). %s, then run the keyer update again."
            % (", ".join(names), how))


# ---- hex file -------------------------------------------------------------

def load_hex(path):
    """Intel hex -> {word address: 14-bit word} for the app area. Config words and
    EEPROM data are skipped; anything below the app area means a full image."""
    mem = {}
    base = 0
    with open(path) as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            if line[0] != ":":
                raise Fail("%s line %d: not Intel hex" % (path, n))
            b = bytes.fromhex(line[1:])
            if sum(b) & 0xFF:
                raise Fail("%s line %d: checksum" % (path, n))
            cnt, a, typ = b[0], (b[1] << 8) | b[2], b[3]
            data = b[4:4 + cnt]
            if typ == 4:
                base = ((data[0] << 8) | data[1]) << 16
            elif typ == 0:
                for i, v in enumerate(data):
                    mem[base + a + i] = v
            elif typ == 1:
                break
    words = {}
    for ba in sorted(mem):
        if ba >= 0x10000:               # config words, EEPROM data
            continue
        wa = ba // 2
        if wa < APP_START:
            raise Fail("%s has code below 0x400: that is a full image (bootloader + app)."
                       " Use the Keyer-MCU-app-*.hex file." % os.path.basename(path))
        if wa >= FLASH_END:
            continue
        lo = mem.get(wa * 2, 0xFF)
        hi = mem.get(wa * 2 + 1, 0x3F)
        words[wa] = ((hi & 0x3F) << 8) | lo
    if APP_START not in words:
        raise Fail("%s has nothing at 0x400: not a keyer app built for the bootloader" % path)
    return words


def rows_of(words):
    """{row start: 64 bytes} for every row that holds app code."""
    rows = {}
    for wa in words:
        r = wa & ~(ROW - 1)
        if r not in rows:
            rows[r] = bytearray(b"\xFF\x3F" * ROW)
    for wa, w in words.items():
        r = wa & ~(ROW - 1)
        i = (wa - r) * 2
        rows[r][i] = w & 0xFF
        rows[r][i + 1] = (w >> 8) & 0x3F
    return rows


# ---- bootloader commands --------------------------------------------------

def write_row(d, start, data):
    i2c(d, BOOT_ADDR, bytes([0x01, start >> 8, start & 0xFF]))
    for off in range(0, ROW * 2, PIECE):
        i2c(d, BOOT_ADDR, bytes([0x05, off]) + bytes(data[off:off + PIECE]))
    i2c(d, BOOT_ADDR, bytes([0x02, sum(data) & 0xFF]))
    time.sleep(0.010)                    # erase + write ~5 ms
    st, _v, _a = boot_status(d)
    if st != 0:
        raise Fail("row 0x%04X: %s" % (start, BOOT_STATUS.get(st, "status %d" % st)))


def read_row(d, start):
    i2c(d, BOOT_ADDR, bytes([0x01, start >> 8, start & 0xFF]))
    i2c(d, BOOT_ADDR, bytes([0x03]))
    got = b""
    for _ in range(ROW * 2 // PIECE):
        got += i2c(d, BOOT_ADDR, rd=PIECE)
    return bytes(b & 0x3F if i & 1 else b for i, b in enumerate(got))


def enter_boot(d):
    if answers(d, BOOT_ADDR):
        return
    if not answers(d, APP_ADDR):
        raise Fail("keyer does not answer (neither app 0x40 nor bootloader 0x5C)")
    say("keyer app -> bootloader")
    i2c(d, APP_ADDR, bytes([0x9F]))      # the app takes command and value as two writes
    i2c(d, APP_ADDR, bytes([0x5C]))
    t0 = time.time()
    while time.time() - t0 < 3.0:
        time.sleep(0.05)
        if answers(d, BOOT_ADDR):
            return
    raise Fail("bootloader did not answer on 0x5C (PIC has no bootloader yet? use the PICkit"
               " with Keyer-MCU-full-*.hex)")


def leave_boot(d):
    i2c(d, BOOT_ADDR, bytes([0x04, 0xA5]))
    t0 = time.time()
    while time.time() - t0 < 4.0:        # the app waits 1 s before it starts I2C
        time.sleep(0.1)
        if answers(d, APP_ADDR):
            return True
    return False


# ---- main -----------------------------------------------------------------

def check(d):
    """Returns 'boot', 'app' or None."""
    if answers(d, BOOT_ADDR):
        st, v, a = boot_status(d)
        say("keyer bootloader answering: version %d, last status %s" %
            (v, BOOT_STATUS.get(st, st)))
        return "boot"
    if answers(d, APP_ADDR):
        say("keyer app answering (0x40)")
        return "app"
    say("keyer not answering")
    return None


def flash(d, path):
    rows = rows_of(load_hex(path))
    starts = sorted(rows)
    say("%s: %d rows, 0x%04X-0x%04X" % (os.path.basename(path), len(starts),
                                        starts[0], starts[-1] + ROW - 1))
    enter_boot(d)
    st, v, _a = boot_status(d)
    if v < MIN_BOOT_VERSION:
        raise Fail("bootloader version %d is too old; put Keyer-MCU-full-*.hex on with the"
                   " PICkit once" % v)
    say("bootloader version %d" % v)

    # Row 0x400 blank first: if the update is cut short, the PIC stays in the bootloader.
    write_row(d, APP_START, b"\xFF\x3F" * ROW)
    order = [s for s in starts if s != APP_START] + [APP_START]
    t0 = time.time()
    for n, s in enumerate(order, 1):
        write_row(d, s, rows[s])
        progress(n, len(order), "write")
        if n % 16 == 0 or n == len(order):
            say("  written %d/%d" % (n, len(order)))
    for n, s in enumerate(starts, 1):
        if read_row(d, s) != bytes(rows[s]):
            raise Fail("verify: row 0x%04X differs" % s)
        progress(n, len(starts), "check")
        if n % 32 == 0 or n == len(starts):
            say("  checked %d/%d" % (n, len(starts)))
    say("written and checked in %.0f s" % (time.time() - t0))
    if leave_boot(d):
        say("keyer app running")
    else:
        raise Fail("keyer app did not answer after leaving the bootloader")


def main():
    args = sys.argv[1:]
    if not args or args[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0
    running = servers_running()
    if running:
        print(servers_message(running))
        return 1
    d = dev()
    if not supported(d):
        print("radio firmware has no keyer update (opcode 0x9F); needs Black Pill 3.16, Proficio MKII PTT 3.244 / ATU 4.245, Geminus MKII 2.154, Ultimus MKII PTT 8.244 / ATU 7.245 or later")
        return 1
    if args[0] == "--check":
        check(d)
        return 0
    if args[0] == "--exit":
        if not answers(d, BOOT_ADDR):
            print("bootloader not answering")
            return 1
        print("keyer app running" if leave_boot(d) else "keyer app did not answer")
        return 0

    path = args[0]
    if not os.path.isfile(path):
        print("no such file: %s" % path)
        return 1
    try:
        flash(d, path)
    except Fail as e:
        print("FAILED: %s" % e)
        print("The keyer stays in the bootloader; run the same command again.")
        return 1
    print("Start the servers again when you are ready (mscc start).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
