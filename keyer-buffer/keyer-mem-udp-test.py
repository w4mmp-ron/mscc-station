#!/usr/bin/env python3
"""
UDP test jig: store + play keyer CQ memory via ms-sdr (not raw USB).

Connects to ms-sdr on port 8888, sends CMD_CHECK_GUI_STATUS (0xFE,1) so
ms-sdr claims the session and dumps startup packets (discarded), then:

  0x9C select slot 0 → begin → each ASCII char → end → play

Default message (slot 0) is a long run of O for WPM timing checks.

Usage (ms-sdr RUNNING, Proficio attached, keyer installed):
  python3 keyer-mem-udp-test.py
  python3 keyer-mem-udp-test.py --host 192.168.12.199
  python3 keyer-mem-udp-test.py --host proficio --play-only
  python3 keyer-mem-udp-test.py --store-only -m "TEST DE N8VET"

Do not run against the same ms-sdr session as a live MSCC client
(single-session lock).
"""

from __future__ import annotations

import argparse
import socket
import struct
import sys
import time

# Defaults
DEFAULT_HOST = "192.168.12.199"  # or "proficio"
DEFAULT_PORT = 8888
DEFAULT_MSG = "O O O O O O O O O O O O O O O"

CMD_CHECK_GUI_STATUS = 0xFE
CMD_SET_KEYER_MEMORY = 0x9C
CMD_SET_KEEP_ALIVE = 0xF4

MEM_PLAY = 0
MEM_STORE_BEGIN = 1
MEM_STORE_END = 2
MEM_SELECT = 3
MEM_MAX_CHARS = 48


def pack_op(opcode: int, value: int = 0) -> bytes:
    """Client wire format: [opcode][int16 LE payload] (ms-sdr uses low byte)."""
    return struct.pack("<Bh", opcode & 0xFF, int(value) & 0xFFFF)


def send_op(sock: socket.socket, addr: tuple, opcode: int, value: int = 0) -> None:
    pkt = pack_op(opcode, value)
    sock.sendto(pkt, addr)


def drain_startup(sock: socket.socket, seconds: float = 2.0) -> int:
    """Toss whatever ms-sdr sends after GUI handshake (versions, band, etc.)."""
    deadline = time.time() + seconds
    n = 0
    sock.settimeout(0.2)
    while time.time() < deadline:
        try:
            data, _ = sock.recvfrom(4096)
            n += 1
            if data:
                op = data[0]
                # keep draining; optional one-line noise
                if op == CMD_SET_KEEP_ALIVE:
                    continue
        except socket.timeout:
            continue
        except OSError:
            break
    return n


def mem_param(sock: socket.socket, addr: tuple, param: int, verbose: bool = True) -> None:
    if verbose:
        if 0x20 <= param <= 0x7E:
            print(f"  0x9C  '{chr(param)}' ({param})")
        else:
            labels = {
                MEM_PLAY: "PLAY",
                MEM_STORE_BEGIN: "STORE_BEGIN",
                MEM_STORE_END: "STORE_END",
                MEM_SELECT: "SELECT",
            }
            lab = labels.get(param, f"param={param}")
            print(f"  0x9C  {lab}")
    send_op(sock, addr, CMD_SET_KEYER_MEMORY, param)
    # Host paces USB (~20 ms). Small gap keeps UDP recv queue from bloating.
    time.sleep(0.005)


def store_slot0(sock: socket.socket, addr: tuple, text: str) -> None:
    if len(text) > MEM_MAX_CHARS:
        print(f"Message truncated to {MEM_MAX_CHARS} chars (was {len(text)})", file=sys.stderr)
        text = text[:MEM_MAX_CHARS]

    print(f"Store slot 0 ({len(text)} chars): {text!r}")
    mem_param(sock, addr, MEM_SELECT)
    mem_param(sock, addr, 0)  # slot 0
    mem_param(sock, addr, MEM_STORE_BEGIN)
    for ch in text:
        o = ord(ch)
        if o < 0x20 or o > 0x7E:
            print(f"  skip non-printable 0x{o:02x}", file=sys.stderr)
            continue
        mem_param(sock, addr, o)
    mem_param(sock, addr, MEM_STORE_END)
    print("Store complete.")


def play(sock: socket.socket, addr: tuple) -> None:
    print("Play slot 0…")
    mem_param(sock, addr, MEM_PLAY)


def main() -> int:
    ap = argparse.ArgumentParser(description="UDP keyer memory test via ms-sdr (0x9C)")
    ap.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"ms-sdr host (default {DEFAULT_HOST}; try 'proficio')",
    )
    ap.add_argument("--port", type=int, default=DEFAULT_PORT, help="ms-sdr port (default 8888)")
    ap.add_argument("-m", "--message", default=DEFAULT_MSG, help="text to store in slot 0")
    ap.add_argument("-p", "--play-only", action="store_true", help="play only (no store)")
    ap.add_argument("--store-only", action="store_true", help="store only (no play)")
    ap.add_argument(
        "--drain",
        type=float,
        default=2.0,
        help="seconds to drain startup UDP after handshake (default 2)",
    )
    args = ap.parse_args()

    if args.play_only and args.store_only:
        print("Pick at most one of --play-only / --store-only", file=sys.stderr)
        return 2

    try:
        infos = socket.getaddrinfo(args.host, args.port, type=socket.SOCK_DGRAM)
    except socket.gaierror as e:
        print(f"Cannot resolve {args.host!r}: {e}", file=sys.stderr)
        return 2

    family, _, _, _, sockaddr = infos[0]
    # Prefer IPv4 if present
    for fam, _, _, _, sa in infos:
        if fam == socket.AF_INET:
            family, sockaddr = fam, sa
            break

    sock = socket.socket(family, socket.SOCK_DGRAM)
    sock.bind(("", 0))  # ephemeral local port; ms-sdr replies here

    print(f"Connect ms-sdr {args.host}:{args.port} → {sockaddr}")
    print("Handshake CMD_CHECK_GUI_STATUS (0xFE, 1)…")
    send_op(sock, sockaddr, CMD_CHECK_GUI_STATUS, 1)

    tossed = drain_startup(sock, args.drain)
    print(f"Drained {tossed} startup packet(s) ({args.drain:.1f}s).")

    # Stop draining so store/play is not blocked on recv
    sock.settimeout(0.5)

    try:
        if not args.play_only:
            store_slot0(sock, sockaddr, args.message)
            # PIC EEPROM write after STORE_END can take ~200–400ms; play too
            # soon → Proficio I2C NACK and PLAY is dropped.
            if not args.store_only:
                time.sleep(0.45)

        if not args.store_only:
            play(sock, sockaddr)
            # leave a moment for first dits so user hears something before exit
            print("Waiting 2s (CW should be keying)…")
            time.sleep(2.0)

        print("Done. (Did not send STOP — ms-sdr left running.)")
        return 0
    except OSError as e:
        print(f"UDP error: {e}", file=sys.stderr)
        return 1
    finally:
        sock.close()


if __name__ == "__main__":
    sys.exit(main())
