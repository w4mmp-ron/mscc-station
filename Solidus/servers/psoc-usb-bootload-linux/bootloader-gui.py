#!/usr/bin/env python3
"""
USB Bootloader — Pi / Linux GUI for Proficio .cyacd upload.

Mirrors the classic Windows USBBootloaderHost layout:
  Vendor/Product ID, security key, Load File, Program, progress, status log.

Requires the CLI helper ``bootloader`` (same folder, $HOME/mscc, or PATH).
Field use: BOOT jumper on → Morse LOADER (04b4:b71d) → Load File → Program.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import threading
import time
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import List, Optional

DEFAULT_VID = "04B4"
DEFAULT_PID = "B71D"
DEFAULT_KEY = "000000000000"
APP_TITLE = "USB Bootloader"


def _which_bootloader() -> Optional[Path]:
    w = shutil.which("bootloader")
    if w:
        return Path(w)
    home = Path.home() / "mscc" / "bootloader"
    if home.is_file() and os.access(home, os.X_OK):
        return home
    here = Path(__file__).resolve().parent / "bootloader"
    if here.is_file() and os.access(here, os.X_OK):
        return here
    # Packaged next to this script under /usr/share/mscc/bin
    share = Path("/usr/share/mscc/binaries/bootloader")
    if share.is_file() and os.access(share, os.X_OK):
        return share
    return None


def _device_present(vid: str, pid: str) -> bool:
    """True if USB device vid:pid is present (BOOT LOADER mode)."""
    vid = vid.lower().zfill(4)
    pid = pid.lower().zfill(4)
    try:
        r = subprocess.run(
            ["lsusb"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if r.returncode != 0:
            return False
        needle = f"{vid}:{pid}"
        return needle in (r.stdout or "").lower()
    except Exception:
        return False


def _count_cyacd_rows(path: Path) -> int:
    try:
        n = 0
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            for i, line in enumerate(f):
                if i == 0:
                    continue  # header
                if line.startswith(":"):
                    n += 1
        return max(n, 1)
    except Exception:
        return 100


class BootloaderApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(APP_TITLE)
        self.minsize(520, 420)
        self.geometry("560x460")
        self.configure(bg="#f0f0f0")

        self.cyacd_path: Optional[Path] = None
        self._busy = False
        self._poll_after: Optional[str] = None
        self._line_buf = ""

        self._build()
        self._set_connected(False)
        self._poll_device()
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build(self) -> None:
        pad = {"padx": 8, "pady": 4}
        frm = ttk.Frame(self, padding=10)
        frm.pack(fill=tk.BOTH, expand=True)

        # --- IDs (match Windows labels) ---
        row0 = ttk.Frame(frm)
        row0.pack(fill=tk.X, **pad)
        ttk.Label(row0, text="Vendor ID 0x").pack(side=tk.LEFT)
        self.vid_var = tk.StringVar(value=DEFAULT_VID)
        ttk.Entry(row0, textvariable=self.vid_var, width=8).pack(side=tk.LEFT, padx=(4, 16))
        ttk.Label(row0, text="Product ID 0x").pack(side=tk.LEFT)
        self.pid_var = tk.StringVar(value=DEFAULT_PID)
        ttk.Entry(row0, textvariable=self.pid_var, width=8).pack(side=tk.LEFT, padx=4)

        row1 = ttk.Frame(frm)
        row1.pack(fill=tk.X, **pad)
        ttk.Label(row1, text="Security Key(6bytes) 0x").pack(side=tk.LEFT)
        self.key_var = tk.StringVar(value=DEFAULT_KEY)
        ttk.Entry(row1, textvariable=self.key_var, width=16).pack(side=tk.LEFT, padx=4)
        ttk.Label(row1, text="(unused for Proficio)", foreground="#666").pack(side=tk.LEFT, padx=6)

        # --- File ---
        row2 = ttk.Frame(frm)
        row2.pack(fill=tk.X, **pad)
        self.file_var = tk.StringVar(value="")
        ttk.Entry(row2, textvariable=self.file_var).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(row2, text="Load File", command=self._load_file).pack(side=tk.LEFT, padx=(8, 0))

        # --- Program ---
        row3 = ttk.Frame(frm)
        row3.pack(fill=tk.X, **pad)
        self.conn_var = tk.StringVar(value="Disconnected")
        ttk.Label(row3, textvariable=self.conn_var, width=16).pack(side=tk.LEFT)
        self.program_btn = ttk.Button(row3, text="Program", command=self._program)
        self.program_btn.pack(side=tk.RIGHT)

        # --- Progress ---
        self.progress = ttk.Progressbar(frm, mode="determinate", maximum=100)
        self.progress.pack(fill=tk.X, **pad)

        # --- Status log ---
        ttk.Label(frm, text="Status Log").pack(anchor=tk.W, padx=8)
        log_frm = ttk.Frame(frm)
        log_frm.pack(fill=tk.BOTH, expand=True, **pad)
        self.log = tk.Text(log_frm, height=14, wrap=tk.WORD, state=tk.DISABLED, font=("Courier", 10))
        sb = ttk.Scrollbar(log_frm, command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb.pack(side=tk.RIGHT, fill=tk.Y)

        hint = (
            "1) BOOT jumper on, power on (Morse LOADER)   "
            "2) Stop ms-sdr   "
            "3) Load File → Program   "
            "4) Remove jumper, power cycle"
        )
        ttk.Label(frm, text=hint, foreground="#333", wraplength=520).pack(anchor=tk.W, padx=8, pady=(0, 4))

        bl = _which_bootloader()
        if bl:
            self._log(f"CLI helper: {bl}")
        else:
            self._log("WARNING: 'bootloader' binary not found on PATH or in ~/mscc")

    def _log(self, msg: str) -> None:
        ts = datetime.now().strftime("%H:%M:%S")
        line = f"{ts}  {msg}\n"
        self.log.configure(state=tk.NORMAL)
        self.log.insert(tk.END, line)
        self.log.see(tk.END)
        self.log.configure(state=tk.DISABLED)

    def _set_connected(self, ok: bool) -> None:
        if ok:
            self.conn_var.set("Connected")
        else:
            self.conn_var.set("Disconnected")

    def _poll_device(self) -> None:
        if not self._busy:
            vid = (self.vid_var.get() or DEFAULT_VID).strip()
            pid = (self.pid_var.get() or DEFAULT_PID).strip()
            self._set_connected(_device_present(vid, pid))
        self._poll_after = self.after(1500, self._poll_device)

    def _on_close(self) -> None:
        if self._poll_after:
            try:
                self.after_cancel(self._poll_after)
            except Exception:
                pass
        self.destroy()

    def _load_file(self) -> None:
        path = filedialog.askopenfilename(
            title="Open CYACD File",
            filetypes=[("cyacd file", "*.cyacd"), ("All files", "*.*")],
        )
        if not path:
            return
        self.cyacd_path = Path(path)
        self.file_var.set(str(self.cyacd_path))
        self._log(f"File: {self.cyacd_path.name}")

    def _program(self) -> None:
        if self._busy:
            return
        path_str = (self.file_var.get() or "").strip()
        if not path_str:
            messagebox.showwarning(APP_TITLE, "No file chosen")
            self._log("No file chosen")
            return
        cyacd = Path(path_str)
        if not cyacd.is_file() or cyacd.suffix.lower() != ".cyacd":
            messagebox.showerror(APP_TITLE, "Expected a .cyacd file")
            return

        bl = _which_bootloader()
        if not bl:
            messagebox.showerror(
                APP_TITLE,
                "bootloader helper not found.\nInstall mscc package or build psoc-usb-bootload-linux.",
            )
            return

        vid = (self.vid_var.get() or DEFAULT_VID).strip()
        pid = (self.pid_var.get() or DEFAULT_PID).strip()
        if not _device_present(vid, pid):
            messagebox.showerror(APP_TITLE, "No Device Connected")
            self._log("No Device Connected — set BOOT jumper and confirm LOADER (04b4:b71d)")
            return

        # Warn if ms-sdr holds USB
        try:
            r = subprocess.run(["pgrep", "-x", "ms-sdr"], capture_output=True, timeout=3)
            if r.returncode == 0:
                if not messagebox.askyesno(
                    APP_TITLE,
                    "ms-sdr appears to be running and may hold the USB device.\n"
                    "Stop it first for a reliable upload.\n\nContinue anyway?",
                ):
                    return
        except Exception:
            pass

        rows = _count_cyacd_rows(cyacd)
        self._busy = True
        self.program_btn.configure(state=tk.DISABLED)
        self.progress["value"] = 0
        self.progress["maximum"] = rows
        self._log(f"Bootload Started at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        self._log(f"Programming {cyacd} via USB HID {vid}:{pid} ...")

        def worker() -> None:
            env = os.environ.copy()
            # Ensure ~/mscc libs resolution if any
            try:
                proc = subprocess.Popen(
                    [str(bl), str(cyacd)],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    bufsize=1,
                    env=env,
                )
                assert proc.stdout is not None
                dots = 0
                for chunk in iter(lambda: proc.stdout.read(1), ""):
                    if chunk == ".":
                        dots += 1
                        self.after(0, lambda d=dots: self._on_dot(d))
                    elif chunk in "\r\n":
                        continue
                    elif chunk:
                        # accumulate non-dot text into lines
                        self.after(0, lambda c=chunk: self._on_char(c))
                rc = proc.wait()
                self.after(0, lambda: self._on_done(rc == 0, rc))
            except Exception as e:
                self.after(0, lambda: self._on_done(False, -1, str(e)))

        threading.Thread(target=worker, daemon=True).start()

    def _on_char(self, c: str) -> None:
        if c == "\n":
            line = self._line_buf.strip()
            self._line_buf = ""
            if line and not set(line) <= {"."}:
                self._log(line)
        else:
            self._line_buf += c

    def _on_dot(self, dots: int) -> None:
        self.progress["value"] = min(dots, int(self.progress["maximum"]))

    def _on_done(self, ok: bool, rc: int, err: str = "") -> None:
        if self._line_buf.strip():
            self._log(self._line_buf.strip())
            self._line_buf = ""
        self._busy = False
        self.program_btn.configure(state=tk.NORMAL)
        ended = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self._log(f"Bootload ended at {ended}")
        if ok:
            self.progress["value"] = self.progress["maximum"]
            self._log("Bootload successful  !!")
            messagebox.showinfo(
                APP_TITLE,
                "Bootload successful  !!\n\nPower off, remove BOOT jumper, power on.",
            )
        else:
            msg = err or f"Program failed (exit {rc})"
            self._log(msg)
            if "Communication" in msg or rc != 0:
                self._log("Program failed: Communication Error")
            messagebox.showerror(APP_TITLE, "Program failed\n\nCheck BOOT jumper / LOADER / ms-sdr stopped.")


def main(argv: Optional[List[str]] = None) -> int:
    argv = argv if argv is not None else sys.argv[1:]
    app = BootloaderApp()
    if argv:
        p = Path(argv[0])
        if p.is_file():
            app.cyacd_path = p
            app.file_var.set(str(p.resolve()))
            app._log(f"File: {p.name}")
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
