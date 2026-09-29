"""mscc-init CLI — text wizard for SSH / headless (Python port of mscc-init-linux).

Writes the same $HOME/.local/mscc files as the GUI (config.py):
  mscc.ini, i2c.ini, cw.ini, comm-port.ini,
  operator speaker / microphone (user picks),
  digital speaker / mic fixed: VirtualA / VirtualB.monitor.
No tkinter import, so it runs without a desktop.
"""

from __future__ import annotations

import socket
import subprocess
import sys
from typing import List

from . import __version__
from .config import (
    MS_SDR_PORT,
    MSCC_DIGI_MIC,
    MSCC_DIGI_SPEAKER,
    MSCC_PORT,
    config_dir,
    ensure_config_dir,
    is_pty_name,
    write_comm_port_ini,
    write_cw_ini,
    write_fixed_digital,
    write_i2c_ini,
    write_device_name,
    write_mscc_ini,
)
from .devices import list_audio_devices, list_serial_ports, read_multus_serial

TOTAL_STEPS = 4
_summary: List[str] = []


def banner(title: str) -> None:
    print("\n" + "=" * 60 + f"\n  {title}\n" + "=" * 60)


def step_banner(step: int, title: str) -> None:
    print("\n" + "-" * 60 + f"\n  Step {step} of {TOTAL_STEPS} — {title}\n" + "-" * 60)


def read_line(prompt: str) -> str | None:
    """Input with trim; None on EOF."""
    try:
        return input(prompt).strip()
    except EOFError:
        return None


def prompt_int(question: str, lo: int, hi: int, default: int) -> int:
    while True:
        line = read_line(question)
        if line is None:
            print(f"\n(EOF — using {default})")
            return default
        if not line:
            return default
        try:
            v = int(line)
        except ValueError:
            print(f"  Please enter a number from {lo} to {hi} (or Enter for default {default}).")
            continue
        if lo <= v <= hi:
            return v
        print(f"  Out of range — use {lo}…{hi} (or Enter for {default}).")


def prompt_yes_no(question: str, default_yes: bool) -> bool:
    while True:
        line = read_line(f"{question} [{'Y/n' if default_yes else 'y/N'}]: ")
        if not line:
            return default_yes
        c = line[0].lower()
        if c == "y":
            return True
        if c == "n":
            return False
        print("  Please answer y or n (or Enter for default).")


def wrote(path, what: str) -> None:
    print(f"  OK  wrote {what}\n      → {path}")


def running_servers() -> List[str]:
    found = []
    for name in ("sdrcore-recv", "sdrcore-trans", "ms-sdr"):
        try:
            r = subprocess.run(["pgrep", "-x", name], capture_output=True, timeout=5)
            if r.returncode == 0:
                found.append(name)
        except Exception:
            pass
    return found


def choose_cat_and_pin() -> None:
    ports = list_serial_ports()  # PTY first, rest sorted
    print("\n  Available CAT ports:")
    for i, p in enumerate(ports):
        print(f"    [{i:2d}]  {p.path:<16}  {p.label}")
    print("\n  Tips:")
    print("    • PTY  — easiest for CAT only; digital apps use ~/ms-sdr-cat")
    print("    • tty0tty — put ms-sdr on one end (e.g. tnt0), digi app on the other (tnt1)")
    print("    • PIN=1 (CTS) is the usual PTT sense when digi asserts RTS on the pair")

    idx = prompt_int("\n  Select CAT port number (Enter = 0 / PTY): ", 0, len(ports) - 1, 0)
    port = ports[idx].path

    if not is_pty_name(port):
        custom = read_line(f"  Using {port}. Press Enter to accept, or type another full path: ")
        if custom:
            port = custom

    if is_pty_name(port):
        print("\n  PTY selected — hardware PTT pins are not available.")
        pin = 0
    else:
        default_pin = 1 if "tnt" in port else 0
        print("\n  PTT pin sense (ms-sdr reads this on the CAT port):")
        print("    0 = none     CAT only, no pin PTT")
        print("    1 = CTS      recommended for tty0tty (digi app RTS → this end CTS)")
        print("    2 = DCD      carrier-detect style PTT")
        pin = prompt_int(f"  Select PIN (Enter = {default_pin}): ", 0, 2, default_pin)

    path = write_comm_port_ini(port, pin)
    name = "PTY" if is_pty_name(port) else port
    if name == "PTY":
        print("  CAT = PTY  → ms-sdr will create $HOME/ms-sdr-cat")
    else:
        print(f"  CAT = {name}")
    pin_desc = ("off (CAT only)",
                "CTS (digi app: assert RTS on the other end of the pair)", "DCD")[pin]
    print(f"  PIN = {pin}  → {pin_desc}")
    _summary.append(f"comm-port.ini     CAT={name}  PIN={pin} ({('off', 'CTS', 'DCD')[pin]})")
    wrote(path, "comm-port.ini")


def pick_device(want_input: bool) -> str | None:
    what = "capture devices (microphone)" if want_input else "playback devices (headphones / speaker)"
    devs, err = list_audio_devices(want_input)
    print(f"\n  Operator {what}:")
    if err:
        print(f"  ERROR: {err}")
        return None
    for i, d in enumerate(devs):
        hint = f"  ← {d.hint}" if d.hint else ""
        print(f"    [{i:2d}]  ch={d.channels}  {d.host_api:<6}  {d.name}{hint}")
    if not devs:
        print("    (none found)")
        return None
    label = "microphone" if want_input else "speaker"
    n = prompt_int(f"  Operator {label} number [0]: ", 0, len(devs) - 1, 0)
    return devs[n].name


def main() -> int:
    banner(f"mscc-init (Linux) v{__version__} — MSCC-MKII base setup")
    print("  This wizard writes the .ini files used by:")
    print("    ms-sdr · sdrcore-recv · sdrcore-trans")
    print("  Press Enter at any prompt to accept the default in [brackets].")

    running = running_servers()
    if running:
        print(f"\n  WARNING: MSCC servers running: {', '.join(running)}")
        print("  Stop them first (mscc stop); they read these files at startup.")
        if not prompt_yes_no("  Continue anyway?", False):
            return 1

    cfg = ensure_config_dir()
    print(f"\n  Config directory: {cfg}")

    # ---- Step 1: radio USB ----
    step_banner(1, "Transceiver USB (optional)")
    print("Looking for Multus/Proficio USB control (VID=0x16C0 PID=0x05DC)…")
    serial, status = read_multus_serial()
    print(f"  {status}")
    host = socket.gethostname() or "127.0.0.1"
    print(f"  This host: {host}  (MSCC client port {MSCC_PORT}, ms-sdr port {MS_SDR_PORT})")

    # ---- Step 2: family + keyer + base inis ----
    step_banner(2, "Radio family, keyer & base config")
    mkii = prompt_yes_no("  Is a Proficio MKII transceiver attached? (not a legacy Proficio)", True)
    keyer = prompt_yes_no("  Is the Proficio MKII keyer installed?", False)

    p = write_i2c_ini()
    _summary.append("i2c.ini           Fortis defaults (MFC=0 meter=0)")
    wrote(p, "i2c.ini")
    p = write_mscc_ini(serial, host, proficio_mkii=mkii)
    _summary.append(f"mscc.ini          serial={serial}  client→{host}:{MSCC_PORT}  "
                    f"ms-sdr port {MS_SDR_PORT}  MKII={int(mkii)}")
    wrote(p, "mscc.ini (network + serial + PROFICIO-MKII)")
    p = write_cw_ini(keyer)
    _summary.append(f"cw.ini            keyer {'INSTALLED' if keyer else 'not installed'}")
    wrote(p, "cw.ini")

    # ---- Step 3: CAT + PIN ----
    step_banner(3, "Kenwood CAT port & PTT pin")
    choose_cat_and_pin()

    # ---- Step 4: operator audio; digi fixed ----
    step_banner(4, "Operator audio (digi is fixed VirtualA/B)")
    print("\n  Digi audio (fixed by install — not selectable):")
    print(f"    digital speaker = {MSCC_DIGI_SPEAKER}")
    print(f"    digital mic     = {MSCC_DIGI_MIC}")
    sp, mic = write_fixed_digital()
    _summary.append(f'digital-speaker.ini "{MSCC_DIGI_SPEAKER}"')
    _summary.append(f'digital-microphone.ini "{MSCC_DIGI_MIC}"')
    wrote(sp, "digital-speaker.ini")
    wrote(mic, "digital-microphone.ini")

    print("\n  Pick operator devices only (headphones / mic for you).")
    exit_status = 0
    for fname, want_input in (("operator-speaker.ini", False), ("operator-microphone.ini", True)):
        name = pick_device(want_input)
        if name is None:
            print(f"  WARNING: no operator device — skipping {fname}.")
            exit_status = 5
            continue
        p = write_device_name(fname, name)
        _summary.append(f'{fname:<18} "{p.read_text(encoding="utf-8")}"')
        wrote(p, fname)

    banner("Done — configuration summary")
    print(f"  Directory: {config_dir()}\n")
    for line in _summary:
        print(f"  • {line}")
    print("\n  Next steps:")
    print("    1. Digi sinks:  pactl list short sinks | grep Virtual")
    print("       (if missing: mscc-virtual-audio)")
    print("    2. Start stack:  mscc start")
    print("    3. Confirm ms-sdr log shows your CAT path and PIN")
    print()
    return exit_status


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nCancelled — files written so far are kept.")
        sys.exit(130)
