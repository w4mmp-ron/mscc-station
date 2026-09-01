#!/usr/bin/env python3
"""
Proficio STM32F411 — CLI USB vendor test jig (Pi / Linux; bare Black Pill OK).

Same control transfers as jig.py / ms-sdr. No GUI.

  pip install -r requirements.txt
  python3 jig_cli.py version
  python3 jig_cli.py temp
  python3 jig_cli.py all
"""

from __future__ import annotations

import argparse
import struct
import sys
from typing import Optional

try:
    import usb.core
    import usb.util
    from usb.core import USBError
except ImportError:
    print("Install deps:  pip install -r requirements.txt", file=sys.stderr)
    raise SystemExit(1)

VID = 0x16C0
PID = 0x05DC
WVALUE_IN = 0xA55A
WVALUE_OUT = 0x071B
TIMEOUT_MS = 1000

CMD_GET_VERSION = 0x00
CMD_GET_PIN = 0x02
CMD_GET_FREQ = 0x3A
CMD_GET_STARTUP = 0x3C
CMD_GET_KEY_DOWN = 0xA4
CMD_GET_PTT = 0xA5
CMD_GET_TRANSCEIVER_TEMP = 0xBF
CMD_GET_PPM_INT = 0x94
CMD_GET_PPM_DEC = 0x95
CMD_SET_FREQ = 0x32
CMD_SET_USRP1 = 0x50
CMD_ENTER_BOOTLOADER = 0xFE


class ProficioUsb:
    def __init__(self) -> None:
        self.dev: Optional[usb.core.Device] = None

    def open(self) -> bool:
        self.close()
        self.dev = usb.core.find(idVendor=VID, idProduct=PID)
        if self.dev is None:
            return False
        try:
            if self.dev.is_kernel_driver_active(0):
                self.dev.detach_kernel_driver(0)
        except (NotImplementedError, usb.core.USBError):
            pass
        try:
            self.dev.set_configuration()
        except usb.core.USBError:
            pass
        return True

    def close(self) -> None:
        if self.dev is not None:
            try:
                usb.util.dispose_resources(self.dev)
            except Exception:
                pass
        self.dev = None

    def ctrl_in(self, cmd: int, length: int, wvalue: int = WVALUE_IN, windex: int = 0) -> bytes:
        if self.dev is None and not self.open():
            raise RuntimeError("No device (VID 16C0 PID 05DC)")
        assert self.dev is not None
        return bytes(
            self.dev.ctrl_transfer(0xC0, cmd, wvalue, windex, length, timeout=TIMEOUT_MS)
        )

    def ctrl_out(
        self, cmd: int, payload: bytes = b"", wvalue: int = WVALUE_OUT, windex: int = 0
    ) -> int:
        if self.dev is None and not self.open():
            raise RuntimeError("No device (VID 16C0 PID 05DC)")
        assert self.dev is not None
        return int(
            self.dev.ctrl_transfer(
                0x40, cmd, wvalue, windex, payload if payload else None, timeout=TIMEOUT_MS
            )
        )


def cmd_version(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_VERSION, 2)
    if len(raw) >= 2:
        ver = struct.unpack_from("<H", raw)[0]
        print(f"GET_VERSION: 0x{ver:04X}  major={ver & 0xFF} minor={(ver >> 8) & 0xFF}  raw={raw.hex()}")
    else:
        print(f"GET_VERSION: short {raw.hex()}")


def cmd_temp(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_TRANSCEIVER_TEMP, 4)
    if len(raw) >= 4:
        be = struct.unpack_from(">i", raw)[0]
        le = struct.unpack_from("<i", raw)[0]
        print(f"GET_TEMP: raw={raw.hex()}  as_be_i32={be}  as_le_i32={le}")
    else:
        print(f"GET_TEMP: short {raw.hex()}")


def cmd_freq(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_FREQ, 4, wvalue=0)
    if len(raw) >= 4:
        print(f"GET_FREQ: {struct.unpack_from('<I', raw)[0]} Hz  raw={raw.hex()}")
    else:
        print(f"GET_FREQ: {raw.hex()}")


def cmd_startup(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_STARTUP, 4, wvalue=0)
    print(f"GET_STARTUP: raw={raw.hex()}")


def cmd_pin(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_PIN, 1)
    val = raw[0] if raw else -1
    print(f"GET_PIN: 0x{val:02X}  raw={raw.hex()}")


def cmd_ptt(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_PTT, 1, wvalue=0)
    print(f"GET_PTT: {raw[0] if raw else '?'}  raw={raw.hex()}")


def cmd_key(usb: ProficioUsb) -> None:
    raw = usb.ctrl_in(CMD_GET_KEY_DOWN, 1, wvalue=0)
    print(f"GET_KEY_DOWN: {raw[0] if raw else '?'}  raw={raw.hex()}")


def cmd_ppm(usb: ProficioUsb) -> None:
    i = usb.ctrl_in(CMD_GET_PPM_INT, 2)
    d = usb.ctrl_in(CMD_GET_PPM_DEC, 2)
    print(f"PPM_INT raw={i.hex()}  PPM_DEC raw={d.hex()}")


def cmd_set_freq(usb: ProficioUsb, hz: int) -> None:
    n = usb.ctrl_out(CMD_SET_FREQ, struct.pack("<I", hz))
    print(f"SET_FREQ {hz} Hz → wrote {n} bytes")
    raw = usb.ctrl_in(CMD_GET_FREQ, 4, wvalue=0)
    if len(raw) >= 4:
        print(f"  readback GET_FREQ: {struct.unpack_from('<I', raw)[0]} Hz")


def cmd_tx(usb: ProficioUsb, on: bool) -> None:
    raw = usb.ctrl_in(CMD_SET_USRP1, 1, wvalue=1 if on else 0)
    print(f"SET_USRP1 tx={'ON' if on else 'OFF'}  reply={raw.hex()}")


def cmd_dfu(dev: ProficioUsb) -> None:
    try:
        dev.ctrl_out(CMD_ENTER_BOOTLOADER, b"\x00\x00\x00\x00")
        print("ENTER_BOOTLOADER (0xFE) sent")
    except USBError as e:
        # Device often disconnects mid-transfer when jumping to ROM DFU — OK
        print(f"ENTER_BOOTLOADER: transfer ended ({e}) — often OK if device reset into DFU")


def cmd_all(dev: ProficioUsb) -> None:
    for fn in (cmd_version, cmd_temp, cmd_freq, cmd_startup, cmd_pin, cmd_ptt, cmd_key, cmd_ppm):
        fn(dev)


def main() -> int:
    p = argparse.ArgumentParser(description="Proficio STM32 USB CLI test jig (Pi/Linux)")
    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("version", help="CMD_GET_VERSION")
    sub.add_parser("temp", help="CMD_GET_TRANSCEIVER_TEMP (0xBF)")
    sub.add_parser("freq", help="CMD_GET_FREQ")
    sub.add_parser("startup", help="CMD_GET_STARTUP")
    sub.add_parser("pin", help="CMD_GET_PIN")
    sub.add_parser("ptt", help="CMD_GET_PTT")
    sub.add_parser("key", help="CMD_GET_KEY_DOWN")
    sub.add_parser("ppm", help="PPM int/dec")
    sub.add_parser("all", help="Run read-only probes")

    sp = sub.add_parser("set-freq", help="CMD_SET_FREQ")
    sp.add_argument("hz", type=int)

    sp = sub.add_parser("tx", help="CMD_SET_USRP1")
    sp.add_argument("state", choices=("on", "off"))

    sub.add_parser("dfu", help="CMD_ENTER_BOOTLOADER 0xFE")

    args = p.parse_args()
    dev = ProficioUsb()
    if not dev.open():
        print("No device found (16c0:05dc)", file=sys.stderr)
        return 1

    try:
        if args.cmd == "version":
            cmd_version(dev)
        elif args.cmd == "temp":
            cmd_temp(dev)
        elif args.cmd == "freq":
            cmd_freq(dev)
        elif args.cmd == "startup":
            cmd_startup(dev)
        elif args.cmd == "pin":
            cmd_pin(dev)
        elif args.cmd == "ptt":
            cmd_ptt(dev)
        elif args.cmd == "key":
            cmd_key(dev)
        elif args.cmd == "ppm":
            cmd_ppm(dev)
        elif args.cmd == "all":
            cmd_all(dev)
        elif args.cmd == "set-freq":
            cmd_set_freq(dev, args.hz)
        elif args.cmd == "tx":
            cmd_tx(dev, args.state == "on")
        elif args.cmd == "dfu":
            cmd_dfu(dev)
    except USBError as e:
        print(f"USB error: {e}", file=sys.stderr)
        return 1
    except RuntimeError as e:
        print(str(e), file=sys.stderr)
        return 1
    finally:
        dev.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
