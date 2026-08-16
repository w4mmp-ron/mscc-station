#!/usr/bin/env python3
"""
Set CQ memory Farnsworth text WPM via ms-sdr (SET_MEM_TEXT_WPM 0x76).

Character speed stays on SET_WPM (keyer / cw.ini CW_Speed). This sets only
the overall/text WPM used for inter-letter and inter-word gaps on memory play.

  0       = off (standard packing at char WPM)
  5–60    = text/overall WPM (should be < char WPM for Farnsworth stretch)
  1–4     = treated as off by the keyer

Usage (ms-sdr RUNNING; no other MSCC client on the session):
  python3 keyer-mem-text-wpm.py 10
  python3 keyer-mem-text-wpm.py 0
  python3 keyer-mem-text-wpm.py 10 --host proficio
  python3 keyer-mem-text-wpm.py 12 --host 192.168.12.199 --port 8888

Then play a memory (keyer-mem-udp-test.py --play-only) to hear the effect.
"""

from __future__ import annotations

import argparse
import socket
import struct
import sys
import time

DEFAULT_HOST = "192.168.12.199"
DEFAULT_PORT = 8888

CMD_CHECK_GUI_STATUS = 0xFE
SET_MEM_TEXT_WPM = 0x76
CMD_SET_KEEP_ALIVE = 0xF4


def pack_op(opcode: int, value: int = 0) -> bytes:
    """Client wire format: [opcode][int16 LE] (ms-sdr uses low byte)."""
    return struct.pack("<Bh", opcode & 0xFF, int(value) & 0xFFFF)


def send_op(sock: socket.socket, addr: tuple, opcode: int, value: int = 0) -> None:
    sock.sendto(pack_op(opcode, value), addr)


def drain_startup(sock: socket.socket, seconds: float = 2.0) -> int:
    deadline = time.time() + seconds
    n = 0
    sock.settimeout(0.2)
    while time.time() < deadline:
        try:
            data, _ = sock.recvfrom(4096)
            n += 1
            if data and data[0] == CMD_SET_KEEP_ALIVE:
                continue
        except socket.timeout:
            continue
        except OSError:
            break
    return n


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Set SET_MEM_TEXT_WPM (0x76) via ms-sdr UDP"
    )
    ap.add_argument(
        "wpm",
        type=int,
        help="text/overall WPM: 0=off, 5–60=Farnsworth text speed",
    )
    ap.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"ms-sdr host (default {DEFAULT_HOST})",
    )
    ap.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"ms-sdr port (default {DEFAULT_PORT})",
    )
    ap.add_argument(
        "--drain",
        type=float,
        default=2.0,
        help="seconds to drain startup UDP after handshake (default 2)",
    )
    ap.add_argument(
        "--no-handshake",
        action="store_true",
        help="skip 0xFE session claim (only if you already own the session)",
    )
    args = ap.parse_args()

    wpm = args.wpm
    if wpm < 0 or wpm > 60:
        print("wpm must be 0..60", file=sys.stderr)
        return 2
    if wpm != 0 and wpm < 5:
        print(f"Note: keyer treats {wpm} as off (valid Farnsworth is 5–60 or 0).", file=sys.stderr)

    try:
        infos = socket.getaddrinfo(args.host, args.port, type=socket.SOCK_DGRAM)
    except socket.gaierror as e:
        print(f"Cannot resolve {args.host!r}: {e}", file=sys.stderr)
        return 2

    family, sockaddr = socket.AF_INET, None
    for fam, _, _, _, sa in infos:
        if fam == socket.AF_INET:
            family, sockaddr = fam, sa
            break
    if sockaddr is None:
        family, _, _, _, sockaddr = infos[0]

    sock = socket.socket(family, socket.SOCK_DGRAM)
    sock.bind(("", 0))

    try:
        print(f"Connect ms-sdr {args.host}:{args.port} → {sockaddr}")
        if not args.no_handshake:
            print("Handshake CMD_CHECK_GUI_STATUS (0xFE, 1)…")
            send_op(sock, sockaddr, CMD_CHECK_GUI_STATUS, 1)
            tossed = drain_startup(sock, args.drain)
            print(f"Drained {tossed} startup packet(s).")
        else:
            time.sleep(0.05)

        print(f"SET_MEM_TEXT_WPM (0x76) = {wpm}" + (" (off)" if wpm == 0 else " (text/overall WPM)"))
        send_op(sock, sockaddr, SET_MEM_TEXT_WPM, wpm)
        time.sleep(0.1)
        print("Done. (cw.ini CW_Mem_Text_WPM updated on host; no STOP sent.)")
        print("Play a memory to hear:  python3 keyer-mem-udp-test.py --play-only")
        return 0
    except OSError as e:
        print(f"UDP error: {e}", file=sys.stderr)
        return 1
    finally:
        sock.close()


if __name__ == "__main__":
    sys.exit(main())
