#!/usr/bin/env python3
"""
mscc-volume.py - levels of the two MSCC digital audio pipes on the Pi.

  VirtualA   receive:   sdrcore-recv plays into it, WSJT-X listens
  VirtualB   transmit:  WSJT-X plays into it, sdrcore-trans listens

Run as the desktop user (not root):   python3 mscc-volume.py
Needs python3-tk and pactl (both are on the Pi image).

Rules:
  - Opening the window changes nothing. It only shows the levels the Pi has now.
  - Moving a slider sets the level at once. Nothing is kept.
  - Save writes ~/.local/mscc/volume-levels.conf. mscc-virtual-audio.sh reads
    that file when it rebuilds the sinks: at login, at a desktop-button Start
    and when run by hand (without the file both are 100%). A plain "mscc start"
    in a terminal does not rebuild them, so the levels stay as they are.
    Forget deletes the file.
  - Only the VirtualA and VirtualB sinks are set. The monitors and the operator
    speaker / microphone are never touched.
"""

import math
import os
import re
import shutil
import subprocess
import tkinter as tk
from tkinter import messagebox, ttk

SINKS = (
    ("VirtualA", "receive audio: sdrcore-recv to WSJT-X"),
    ("VirtualB", "transmit audio: WSJT-X to sdrcore-trans"),
)
MONITORS = ("VirtualA.monitor", "VirtualB.monitor")
SAVE_FILE = os.path.join(os.path.expanduser("~"), ".local", "mscc", "volume-levels.conf")
POLL_MS = 1000
PACTL = shutil.which("pactl")


def pactl(*args):
    """Run pactl. Returns its output, or None if it failed."""
    if not PACTL:
        return None
    env = dict(os.environ, LC_ALL="C")
    try:
        r = subprocess.run([PACTL, *args], capture_output=True, text=True, timeout=5, env=env)
    except (OSError, subprocess.SubprocessError):
        return None
    return r.stdout if r.returncode == 0 else None


def get_level(kind, name):
    """(percent, muted) of a sink or source, or None if it is not there."""
    vol = pactl(f"get-{kind}-volume", name)
    mute = pactl(f"get-{kind}-mute", name)
    if vol is None or mute is None:
        return None
    # "Volume: front-left: 53740 /  82% / -5.17 dB,   front-right: ..."
    m = re.search(r"/\s*(\d+)%", vol)
    if not m:
        return None
    return int(m.group(1)), "yes" in mute.lower()


def db_text(pct):
    """Pulse percent is cubic: dB = 60 * log10(percent / 100)."""
    if pct <= 0:
        return "off"
    return f"{60.0 * math.log10(pct / 100.0):+.1f} dB"


def load_saved():
    """Returns ({sink name: (percent, muted)}, number of other lines in the file)."""
    saved, other = {}, 0
    try:
        with open(SAVE_FILE, encoding="utf-8", errors="replace") as f:
            lines = f.read().splitlines()
    except OSError:
        return saved, other
    names = [name for name, _ in SINKS]
    for line in lines:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        # key|kind|name|pct|mute   (the format mscc-virtual-audio.sh reads)
        parts = line.split("|")
        if len(parts) >= 5 and parts[1] == "sink" and parts[2] in names and parts[3].isdigit():
            saved[parts[2]] = (int(parts[3]), parts[4].strip() == "1")
        else:
            other += 1
    return saved, other


def write_saved(levels):
    """levels = {sink name: (percent, muted)}. Whole file replaced, tmp + rename."""
    os.makedirs(os.path.dirname(SAVE_FILE), exist_ok=True)
    lines = ["# mscc-volume.py - levels applied by mscc-virtual-audio.sh", "# key|kind|name|pct|mute"]
    for name, (pct, muted) in levels.items():
        lines.append(f"{name}|sink|{name}|{pct}|{1 if muted else 0}")
    tmp = SAVE_FILE + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    os.replace(tmp, SAVE_FILE)


class SinkRow:
    """One sink: slider, percent + dB, mute box, 100% button."""

    def __init__(self, parent, name, what):
        self.name = name
        self.present = False
        self.pct = 0          # level the Pi has
        self.shown = None     # slider position that matches it (no set needed)
        self.dragging = False

        frame = ttk.LabelFrame(parent, text=f"{name}  -  {what}", padding=8)
        frame.pack(fill=tk.X, pady=6)

        self.scale = tk.Scale(frame, from_=0, to=100, orient=tk.HORIZONTAL,
                              showvalue=False, length=360, command=self.on_scale)
        self.scale.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.scale.bind("<ButtonPress-1>", lambda e: setattr(self, "dragging", True))
        self.scale.bind("<ButtonRelease-1>", lambda e: setattr(self, "dragging", False))

        self.label = ttk.Label(frame, width=16, anchor=tk.E)
        self.label.pack(side=tk.LEFT, padx=(8, 8))

        self.mute_var = tk.BooleanVar()
        self.mute = ttk.Checkbutton(frame, text="Mute", variable=self.mute_var,
                                    command=self.on_mute)
        self.mute.pack(side=tk.LEFT)

        self.full = ttk.Button(frame, text="100%", width=6,
                               command=lambda: self.set_level(100))
        self.full.pack(side=tk.LEFT, padx=(8, 0))

    def refresh(self):
        """Read the level from the Pi and show it. Never sets anything."""
        if self.dragging:
            return
        level = get_level("sink", self.name)
        self.present = level is not None
        state = tk.NORMAL if self.present else tk.DISABLED
        self.scale.configure(state=state)
        self.mute.configure(state=state)
        self.full.configure(state=state)
        if not self.present:
            self.label.configure(text="not found")
            return
        self.pct, muted = level
        self.mute_var.set(muted)
        self.shown = min(self.pct, 100)
        self.scale.set(self.shown)
        self.label.configure(text=f"{self.pct}%  {db_text(self.pct)}")

    def on_scale(self, value):
        # Also called when refresh() moves the slider: then value == shown.
        pct = int(float(value))
        if self.present and pct != self.shown:
            self.set_level(pct)

    def set_level(self, pct):
        if pactl("set-sink-volume", self.name, f"{pct}%") is None:
            messagebox.showerror("MSCC Volume", f"Could not set {self.name}.")
            self.dragging = False
            self.refresh()
            return
        self.pct = self.shown = pct
        self.scale.set(pct)
        self.label.configure(text=f"{pct}%  {db_text(pct)}")

    def on_mute(self):
        if pactl("set-sink-mute", self.name, "1" if self.mute_var.get() else "0") is None:
            messagebox.showerror("MSCC Volume", f"Could not mute {self.name}.")
            self.refresh()


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("MSCC Volume")
        self.resizable(True, False)

        outer = ttk.Frame(self, padding=12)
        outer.pack(fill=tk.BOTH, expand=True)

        self.rows = [SinkRow(outer, name, what) for name, what in SINKS]

        self.warn = ttk.Label(outer, foreground="#a00000", wraplength=620, justify=tk.LEFT)
        self.warn.pack(anchor=tk.W, pady=(4, 0))

        self.saved = ttk.Label(outer, wraplength=620, justify=tk.LEFT)
        self.saved.pack(anchor=tk.W, pady=(8, 8))

        buttons = ttk.Frame(outer)
        buttons.pack(fill=tk.X)
        ttk.Button(buttons, text="Save", command=self.on_save).pack(side=tk.LEFT)
        ttk.Button(buttons, text="Forget", command=self.on_forget).pack(side=tk.LEFT, padx=8)
        ttk.Button(buttons, text="Close", command=self.destroy).pack(side=tk.RIGHT)

        self.poll()

    def poll(self):
        """Once a second: show what the Pi has now (another tool may change it)."""
        self.refresh()
        self.after(POLL_MS, self.poll)

    def refresh(self):
        for row in self.rows:
            row.refresh()

        warnings = []
        if not PACTL:
            warnings.append("pactl not found: this must run on the Pi, as the desktop user.")
        elif not all(row.present for row in self.rows):
            warnings.append("Sink not found: run \"mscc-virtual-audio\" first.")
        for mon in MONITORS:
            level = get_level("source", mon)
            if level and (level[0] != 100 or level[1]):
                warnings.append(f"{mon} is at {level[0]}%{', muted' if level[1] else ''}; "
                                f"it should be 100%, not muted. This window does not change it.")
        self.warn.configure(text="\n".join(warnings))

        saved, other = load_saved()
        if saved:
            text = "Saved, applied at login or desktop Start: " + ", ".join(
                f"{name} {pct}%{' muted' if muted else ''}" for name, (pct, muted) in saved.items())
        else:
            text = "Nothing saved: both are 100% at login or desktop Start."
        if other:
            text += f"\nThe file also has {other} other line(s) from the old volume GUI. Save or Forget removes them."
        self.saved.configure(text=text)

    def on_save(self):
        self.refresh()
        if not all(row.present for row in self.rows):
            messagebox.showerror("MSCC Volume", "Not saved: a sink is missing. Run \"mscc-virtual-audio\" first.")
            return
        try:
            write_saved({row.name: (row.pct, row.mute_var.get()) for row in self.rows})
        except OSError as e:
            messagebox.showerror("MSCC Volume", f"Not saved: {e}")
        self.refresh()

    def on_forget(self):
        try:
            os.remove(SAVE_FILE)
        except FileNotFoundError:
            pass
        except OSError as e:
            messagebox.showerror("MSCC Volume", f"Could not delete the file: {e}")
        self.refresh()


if __name__ == "__main__":
    App().mainloop()
