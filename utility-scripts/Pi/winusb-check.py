#!/usr/bin/env python3
"""
winusb-check.py: read the Black Pill's WinUSB answer and check it, byte by byte.

    python3 winusb-check.py              # the radio running (app 3.17 and later)
    python3 winusb-check.py flash-mode   # the pill's bootloader (flash mode, 0483:df11)

Read only, safe with the servers up or down. Checks what Windows reads at plug-in:
USB version 2.01 or higher, the BOS block with Microsoft's OS 2.0 ID, and the
descriptor set that puts WinUSB on interface 0. Prints OK or FAIL per item;
exit code 0 only if everything passes.

flash-mode: if the radio is running, it sends the pill to flash mode first (USB
command 0xFE, as stm32-app-flash does) - stop the servers before that. The pill
then stays in flash mode: run stm32-app-flash with the app file to bring it back
(or press NRST).
"""
import struct
import sys
import time

import usb.core

RADIO = (0x16C0, 0x05DC)
FLASH_MODE = (0x0483, 0xDF11)
TIMEOUT = 1000

MS_OS_20_UUID = bytes([0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
                       0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F])
EXPECT_VENDOR_CODE = 0x4D
GUID_APP = "{0392A87C-DD95-4332-B37A-76EE39A7182C}"
GUID_BOOTLOADER = "{8942165F-183D-42A7-84B4-8DF6713CA8DD}"

fails = 0


def check(ok, text):
    global fails
    print(("OK    " if ok else "FAIL  ") + text)
    if not ok:
        fails += 1
    return ok


def find(ids):
    return usb.core.find(idVendor=ids[0], idProduct=ids[1])


def enter_flash_mode():
    d = find(FLASH_MODE)
    if d is not None:
        print("Pill already in flash mode.")
        return d
    r = find(RADIO)
    if r is None:
        print("No radio (16c0:05dc) and no pill in flash mode (0483:df11) on USB.")
        return None
    print("Radio running: sending it to flash mode (0xFE)...")
    try:
        r.ctrl_transfer(0x40, 0xFE, 0, 0, b"", 500)
    except usb.core.USBError:
        pass    # the pill may reset before it answers
    for _ in range(20):
        time.sleep(0.5)
        d = find(FLASH_MODE)
        if d is not None:
            return d
    print("Pill did not enter flash mode (no 0483:df11 after 10 s).")
    return None


def check_device(d, guid_expected):
    # Device descriptor
    dev = bytes(d.ctrl_transfer(0x80, 6, 0x0100, 0, 18, TIMEOUT))
    bcd_usb, = struct.unpack_from("<H", dev, 2)
    bcd_dev, = struct.unpack_from("<H", dev, 12)
    check(bcd_usb >= 0x0201, "USB version %x.%02x (2.01 or higher makes Windows ask for the BOS)"
          % (bcd_usb >> 8, bcd_usb & 0xFF))
    print("      device version %x.%02x" % (bcd_dev >> 8, bcd_dev & 0xFF))

    # BOS descriptor: header first, then the whole block
    try:
        hdr = bytes(d.ctrl_transfer(0x80, 6, 0x0F00, 0, 5, TIMEOUT))
    except usb.core.USBError as e:
        check(False, "BOS block: no answer (%s). Older firmware / bootloader?" % e)
        return
    total, = struct.unpack_from("<H", hdr, 2)
    bos = bytes(d.ctrl_transfer(0x80, 6, 0x0F00, 0, total, TIMEOUT))
    check(len(bos) == total and bos[1] == 0x0F, "BOS block, %d bytes, %d capability" % (len(bos), bos[4]))

    vendor_code = None
    set_len = None
    i = 5
    while i + 3 <= len(bos):
        ln, typ, cap = bos[i], bos[i + 1], bos[i + 2]
        if ln == 0:
            break
        if typ == 0x10 and cap == 0x05 and bos[i + 4:i + 20] == MS_OS_20_UUID:
            win_ver, set_len, vendor_code, alt = struct.unpack_from("<IHBB", bos, i + 20)
            check(win_ver >= 0x06030000, "Microsoft OS 2.0 entry, for Windows 8.1 and later (0x%08X)" % win_ver)
        i += ln
    if not check(vendor_code is not None, "Microsoft OS 2.0 entry found in the BOS"):
        return
    check(vendor_code == EXPECT_VENDOR_CODE, "request code 0x%02X (expected 0x%02X)"
          % (vendor_code, EXPECT_VENDOR_CODE))

    # The descriptor set, asked for the way Windows does: vendor request, wIndex 7
    ms = bytes(d.ctrl_transfer(0xC0, vendor_code, 0, 7, set_len, TIMEOUT))
    check(len(ms) == set_len, "descriptor set, %d bytes (BOS says %d)" % (len(ms), set_len))

    # A one-interface device has no subsets: the set covers interface 0.
    p = 0
    first_itf = 0
    winusb_itfs = []
    guid = None
    while p + 4 <= len(ms):
        ln, typ = struct.unpack_from("<HH", ms, p)
        if ln == 0:
            break
        if typ == 0x00:
            _, _, _, tot = struct.unpack_from("<HHIH", ms, p)
            check(tot == len(ms), "set header, total length %d" % tot)
        elif typ == 0x01:
            check(ms[p + 4] == 0, "configuration subset (configuration index 0)")
        elif typ == 0x02:
            first_itf = ms[p + 4]
        elif typ == 0x03:
            cid = ms[p + 4:p + 12].rstrip(b"\0").decode("ascii", "replace")
            if cid == "WINUSB":
                winusb_itfs.append(first_itf)
        elif typ == 0x04:
            dtype, nlen = struct.unpack_from("<HH", ms, p + 4)
            name = ms[p + 8:p + 8 + nlen].decode("utf-16-le").rstrip("\0")
            dlen, = struct.unpack_from("<H", ms, p + 8 + nlen)
            data = ms[p + 10 + nlen:p + 10 + nlen + dlen].decode("utf-16-le")
            if name == "DeviceInterfaceGUIDs" and dtype == 7:
                guid = data.split("\0")[0]
        p += ln
    check(p == len(ms), "set parsed to the end")
    check(winusb_itfs == [0], "WinUSB on interface 0 only (found: %s)" % winusb_itfs)
    check(guid == guid_expected, "interface GUID %s" % guid)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "flash-mode":
        d = enter_flash_mode()
        if d is None:
            return 2
        print("Pill bootloader (flash mode, 0483:df11)")
        check_device(d, GUID_BOOTLOADER)
        print("The pill is still in flash mode: run stm32-app-flash with the app file, or press NRST.")
    else:
        d = find(RADIO)
        if d is None:
            print("Radio not found (16c0:05dc). Is it plugged in and running (not in flash mode)?")
            return 2
        v = bytes(d.ctrl_transfer(0xC0, 0x00, 0, 0, 2, TIMEOUT))
        print("Pill firmware %d.%d" % (v[0], v[1]))
        check_device(d, GUID_APP)

    print()
    print("All checks passed." if fails == 0 else "%d check(s) FAILED." % fails)
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
