#!/usr/bin/env python3
"""Inject CMD_SET_REMOTE_RX_HOST (0x25) and CMD_SET_REMOTE_RX_CTRL (0x28)
into a live ms-sdr session (UDP 8888). Does not steal the GUI handshake.

  ./set-remote-rx.py --dest 127.0.0.1 --enable
  ./set-remote-rx.py --dest 127.0.0.1 --disable
  ./set-remote-rx.py --listen   # count MSA1 packets on 9100
"""
from __future__ import annotations

import argparse
import socket
import struct
import sys
import time

CMD_HOST = 0x25
CMD_CTRL = 0x28
CTRL_ENABLE = 1 << 16
CTRL_MONITOR = 1 << 17


def send_host(sock: socket.socket, ms_sdr: str, port: int, dest_ip: str) -> None:
    ip = socket.inet_aton(dest_ip)
    pkt = bytes([CMD_HOST]) + ip
    sock.sendto(pkt, (ms_sdr, port))
    print(f"HOST  0x25  {dest_ip}  ({ip.hex()}) → {ms_sdr}:{port}")


def send_ctrl(
    sock: socket.socket,
    ms_sdr: str,
    port: int,
    listen_port: int,
    enable: bool,
    monitor: bool,
) -> None:
    packed = listen_port & 0xFFFF
    if enable:
        packed |= CTRL_ENABLE
    if monitor:
        packed |= CTRL_MONITOR
    pkt = bytes([CMD_CTRL]) + struct.pack("<I", packed)
    sock.sendto(pkt, (ms_sdr, port))
    print(
        f"CTRL  0x28  enable={int(enable)} monitor={int(monitor)} "
        f"port={listen_port} packed=0x{packed:08x} → {ms_sdr}:{port}"
    )


def listen_msa1(bind_port: int, seconds: float) -> int:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", bind_port))
    s.settimeout(0.5)
    n = 0
    t0 = time.time()
    print(f"listen MSA1 on UDP {bind_port} for {seconds:.1f}s …")
    while time.time() - t0 < seconds:
        try:
            data, addr = s.recvfrom(2048)
        except socket.timeout:
            continue
        n += 1
        if n == 1 or n % 50 == 0:
            magic = data[:4] if len(data) >= 4 else b""
            print(f"  pkt {n} from {addr[0]}:{addr[1]} len={len(data)} magic={magic!r}")
    s.close()
    print(f"got {n} packets")
    return n


def main() -> int:
    p = argparse.ArgumentParser(description="Live remote-phones RX opcodes (linux/ item 2)")
    p.add_argument("--ms-sdr", default="127.0.0.1", help="ms-sdr host (default 127.0.0.1)")
    p.add_argument("--ms-port", type=int, default=8888)
    p.add_argument("--dest", default="127.0.0.1", help="IPv4 recv should send MSA1 to")
    p.add_argument("--rx-port", type=int, default=9100)
    p.add_argument("--enable", action="store_true")
    p.add_argument("--disable", action="store_true")
    p.add_argument("--monitor", action="store_true", help="keep shack speaker")
    p.add_argument("--listen", action="store_true", help="count MSA1 on --rx-port")
    p.add_argument("--seconds", type=float, default=3.0)
    args = p.parse_args()

    if args.listen:
        n = listen_msa1(args.rx_port, args.seconds)
        return 0 if n > 0 else 1

    if not args.enable and not args.disable:
        p.error("need --enable, --disable, or --listen")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    send_host(sock, args.ms_sdr, args.ms_port, args.dest)
    send_ctrl(
        sock,
        args.ms_sdr,
        args.ms_port,
        args.rx_port,
        enable=bool(args.enable),
        monitor=bool(args.monitor),
    )
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
