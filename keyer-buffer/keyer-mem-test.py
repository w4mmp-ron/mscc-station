#!/usr/bin/env python3
"""
Bench test: store/play keyer CQ memory via Proficio USB (no ms-sdr).

USB: vendor OUT, bRequest=0x9C, wValue=0x071B, wIndex=0,
     2 data bytes: [param, seq]  (seq increments each transfer)

Usage (ms-sdr STOPPED):
  sudo apt install -y python3-usb
  sudo python3 keyer-mem-test.py              # store default CQ + play
  sudo python3 keyer-mem-test.py -p           # play only (already stored)
  sudo python3 keyer-mem-test.py --play-only
  sudo python3 keyer-mem-test.py --store-only
  sudo python3 keyer-mem-test.py -m "TEST DE W4MMP"
"""

from __future__ import annotations

import argparse
import struct
import sys
import time

try:
    import usb.core
    import usb.util
except ImportError:
    print("Need pyusb:  sudo apt install -y python3-usb", file=sys.stderr)
    sys.exit(1)

VID = 0x16C0
PID = 0x05DC
WVALUE = 0x0700 + 27  # 0x071B
CMD_SET_KEYER_MEMORY = 0x9C
DEFAULT_GAP_S = 0.08
DEFAULT_MSG = "CQ CQ CQ DE W4MMP W4MMP W4MMP KN"

MEM_PLAY = 0
MEM_STORE_BEGIN = 1
MEM_STORE_END = 2
MEM_SELECT = 3

_seq = 0


def _init_seq() -> None:
    """Avoid Proficio ignoring play when seq restarts at 1 after a prior run."""
    global _seq
    _seq = int(time.time() * 1000) & 0xFF
    if _seq == 0:
        _seq = 1


def open_proficio():
    dev = usb.core.find(idVendor=VID, idProduct=PID)
    if dev is None:
        print(f"Proficio not found (VID={VID:#06x} PID={PID:#06x}).", file=sys.stderr)
        print("Plug in radio; stop ms-sdr:  mscc stop", file=sys.stderr)
        sys.exit(2)

    # Prefer not to fight another claim
    try:
        for cfg in dev:
            for intf in cfg:
                num = intf.bInterfaceNumber
                try:
                    if dev.is_kernel_driver_active(num):
                        dev.detach_kernel_driver(num)
                except (NotImplementedError, usb.core.USBError):
                    pass
    except usb.core.USBError as e:
        print(f"detach note: {e}", file=sys.stderr)

    try:
        dev.set_configuration()
    except usb.core.USBError as e:
        # Often "Resource busy" if already configured — OK to continue
        print(f"set_configuration: {e} (continuing)", file=sys.stderr)

    try:
        usb.util.claim_interface(dev, 0)
    except usb.core.USBError as e:
        print(f"claim_interface: {e} (continuing)", file=sys.stderr)

    return dev


def radio_send(dev, param: int, gap_s: float) -> None:
    """Control write 2 bytes: param + rolling sequence (never 0)."""
    global _seq
    _seq = (_seq % 255) + 1
    data = struct.pack("<BB", int(param) & 0xFF, _seq)
    # host-to-device | vendor | device
    n = dev.ctrl_transfer(0x40, CMD_SET_KEYER_MEMORY, WVALUE, 0, data, timeout=2000)
    if n != 2:
        raise RuntimeError(f"ctrl_transfer wrote {n} bytes, expected 2")
    time.sleep(gap_s)


def send_mem(dev, param: int, gap_s: float) -> None:
    if 0x20 <= param <= 0x7E:
        print(f"  0x9C  '{chr(param)}' ({param}) seq→")
    else:
        print(f"  0x9C  param={param}")
    radio_send(dev, param, gap_s)


def select_slot(dev, slot: int, gap_s: float) -> None:
    slot = max(0, min(3, int(slot)))
    print(f"Select slot {slot}…")
    send_mem(dev, MEM_SELECT, gap_s)
    send_mem(dev, slot, gap_s)


def store_message(dev, text: str, gap_s: float) -> None:
    if len(text) > 48:
        print(f"Message truncated to 48 chars (was {len(text)})", file=sys.stderr)
        text = text[:48]
    print(f"Store begin ({len(text)} chars)…")
    send_mem(dev, MEM_STORE_BEGIN, gap_s)
    for ch in text:
        o = ord(ch)
        if o < 0x20 or o > 0x7E:
            print(f"  skip non-printable 0x{o:02x}", file=sys.stderr)
            continue
        send_mem(dev, o, gap_s)
    print("Store end…")
    send_mem(dev, MEM_STORE_END, gap_s)


def play_message(dev, gap_s: float) -> None:
    print("Play…")
    send_mem(dev, MEM_PLAY, gap_s)


def main() -> int:
    ap = argparse.ArgumentParser(description="Proficio keyer CQ memory USB test (0x9C)")
    ap.add_argument("-m", "--message", default=DEFAULT_MSG, help="text to store (with store)")
    ap.add_argument(
        "--slot",
        type=int,
        default=0,
        choices=(0, 1, 2, 3),
        help="memory slot 0..3 (sent as 0x9C,3 then 0x9C,n)",
    )
    ap.add_argument(
        "-p",
        "--play-only",
        action="store_true",
        help="play current slot only (do not store)",
    )
    ap.add_argument(
        "-s",
        "--store-only",
        action="store_true",
        help="store message only (do not play)",
    )
    ap.add_argument("--gap", type=float, default=DEFAULT_GAP_S, help="seconds between USB xfers")
    args = ap.parse_args()

    if args.store_only and args.play_only:
        print("Pick at most one of -s/--store-only or -p/--play-only", file=sys.stderr)
        return 1

    print(f"Opening Proficio {VID:#06x}:{PID:#06x} …")
    print("Tip: mscc stop   # free the USB device")
    _init_seq()
    dev = open_proficio()
    print("OK.")

    try:
        select_slot(dev, args.slot, args.gap)
        if not args.play_only:
            store_message(dev, args.message, args.gap)
            # let Configure_CW drain I2C before play
            time.sleep(0.5)
        if not args.store_only:
            play_message(dev, args.gap)
            print("Listen for keyer sidetone / key line.")
    except usb.core.USBError as e:
        print(f"USB error: {e}", file=sys.stderr)
        print("Stop anything using the Proficio (mscc stop) and retry.", file=sys.stderr)
        return 3
    finally:
        try:
            usb.util.release_interface(dev, 0)
        except Exception:
            pass

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
