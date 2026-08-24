#!/usr/bin/env python3
"""
Inject CMD_SET_AUDIO_DEVICE (0x9B) into a running ms-sdr session.

Sends only the audio-device opcode — no GUI handshake (0xFE), so an
active MSCC client session is left alone. Does not modify ms-sdr.

Wire format: [opcode u8][int16 LE payload]  — server uses low byte

Values:
  0 = Digital (local digi)
  1 = Phones  (local operator mic)
  2 = Remote  (MSA1 UDP mic — Phones + REMOTE AUDIO)

Usage:
  python set-audio-device.py --host proficio --mode remote
  python set-audio-device.py --host 192.168.12.199 --mode phones
  python set-audio-device.py --mode digital
  python set-audio-device.py --mode remote --direct   # → sdrcore-trans:9200
"""

from __future__ import annotations

import argparse
import socket
import struct

CMD_SET_AUDIO_DEVICE = 0x9B

MODE_MAP = {
    "digital": 0,
    "phones": 1,
    "remote": 2,
    "0": 0,
    "1": 1,
    "2": 2,
}

MODE_NAME = {0: "Digital (0)", 1: "Phones (1)", 2: "Remote (2)"}


def pack_op(opcode: int, value: int = 0) -> bytes:
    return struct.pack("<Bh", opcode & 0xFF, int(value) & 0xFFFF)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Inject CMD_SET_AUDIO_DEVICE into active ms-sdr (no handshake)"
    )
    ap.add_argument("--host", default="proficio", help="Pi hostname or IP (default: proficio)")
    ap.add_argument(
        "--mode",
        default="remote",
        choices=sorted(MODE_MAP.keys()),
        help="digital|phones|remote or 0|1|2 (default: remote)",
    )
    ap.add_argument("--port", type=int, default=0, help="UDP port (default 8888 / 9200 with --direct)")
    ap.add_argument(
        "--direct",
        action="store_true",
        help="Send to sdrcore-trans:9200 (bypass ms-sdr)",
    )
    args = ap.parse_args()

    value = MODE_MAP[args.mode.lower()]
    use_ms_sdr = not args.direct
    port = args.port if args.port > 0 else (8888 if use_ms_sdr else 9200)
    target = "ms-sdr (active session OK)" if use_ms_sdr else "sdrcore-trans (direct)"

    print(f"Set-AudioDevice → {args.host}:{port} ({target})")
    print(f"  CMD_SET_AUDIO_DEVICE 0x9B  data={MODE_NAME.get(value, value)}")
    print("  (no GUI handshake — will not steal the client session)")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        pkt = pack_op(CMD_SET_AUDIO_DEVICE, value)
        sock.sendto(pkt, (args.host, port))
        print(f"  sent {' '.join(f'{b:02X}' for b in pkt)}  OK")
        print()
        print(f"Check Pi logs for CMD_SET_AUDIO_DEVICE: {value}")
    finally:
        sock.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
