#!/usr/bin/env python3
"""Write the seamless brushed-metal tile used by the Avalonia faceplate.

Stdlib only. Seed 100. 256x256 RGBA. Streaks run left-right; the tile wraps
on every edge. Pixels are white or black only (no gray), alpha at most 22.
"""
import random
import struct
import zlib
from pathlib import Path

W = H = 256
SEED = 100
BLUR_RADIUS = 24

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "MSCC.Avalonia" / "Assets" / "face-brushed.png"


def horizontal_blur(noise, radius):
    """Wrap-around box blur. Window is 2*radius+1 samples."""
    span = 2 * radius + 1
    # Triple the row so a window centered on the middle copy never falls off.
    tripled = noise + noise + noise
    prefix = [0.0]
    for value in tripled:
        prefix.append(prefix[-1] + value)
    out = []
    for x in range(W):
        center = x + W
        start = center - radius
        end = center + radius
        out.append((prefix[end + 1] - prefix[start]) / span)
    return out


def build_rows():
    rng = random.Random(SEED)
    blurred = []
    for _y in range(H):
        noise = [rng.uniform(-1.0, 1.0) for _x in range(W)]
        blurred.append(horizontal_blur(noise, BLUR_RADIUS))
    # 25% of the row above (wrap) so the tile is seamless top/bottom.
    mixed = []
    for y in range(H):
        above = blurred[(y - 1) % H]
        mixed.append([0.75 * blurred[y][x] + 0.25 * above[x] for x in range(W)])
    return mixed


def rgba_bytes(rows):
    raw = bytearray(W * H * 4)
    i = 0
    for y in range(H):
        for v in rows[y]:
            mag = abs(v)
            if mag > 1.0:
                mag = 1.0
            alpha = int(round(mag * 22))
            if alpha > 22:
                alpha = 22
            if v > 0:
                raw[i] = 255
                raw[i + 1] = 255
                raw[i + 2] = 255
            else:
                raw[i] = 0
                raw[i + 1] = 0
                raw[i + 2] = 0
            raw[i + 3] = alpha
            i += 4
    return bytes(raw)


def png_chunk(tag, data):
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def write_png(path, rgba):
    ihdr = struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)
    scan = b"".join(b"\x00" + rgba[y * W * 4 : (y + 1) * W * 4] for y in range(H))
    png = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(scan, 9))
        + png_chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def main():
    write_png(OUT, rgba_bytes(build_rows()))
    print(f"wrote {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
