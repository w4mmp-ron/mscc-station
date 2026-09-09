#!/usr/bin/env python3
"""Quick Kenwood CAT smoke: open ~/ms-sdr-cat, send ID;, expect ID019;"""
import os
import select
import sys
import time

path = os.path.expanduser("~/ms-sdr-cat")
if not os.path.exists(path):
    print("missing", path)
    sys.exit(1)

fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
print("opened", path, "fd", fd)
time.sleep(0.3)
os.write(fd, b"ID;")
print("sent ID;")
deadline = time.time() + 3
buf = b""
while time.time() < deadline:
    r, _, _ = select.select([fd], [], [], 0.5)
    if fd in r:
        chunk = os.read(fd, 64)
        if not chunk:
            break
        buf += chunk
        print("got", chunk)
        if b";" in buf:
            break
print("reply:", buf)
os.close(fd)
if b"ID019;" in buf or (b"ID" in buf and b";" in buf):
    print("CAT_OK")
    sys.exit(0)
print("CAT_FAIL")
sys.exit(2)
