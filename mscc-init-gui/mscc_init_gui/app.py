#!/usr/bin/env python3
"""
MSCC Init GUI — wizard for Raspberry Pi OS.
Writes $HOME/.local/mscc (same as CLI mscc-init).
Digi audio is fixed: VirtualA / VirtualB.monitor.
Operator devices: any rate (servers resample if not 96 kHz).
"""

from __future__ import annotations

import shutil
import subprocess
import tkinter as tk
from tkinter import messagebox, ttk
from typing import List, Optional, Tuple

from . import __version__
from .config import (
    MSCC_DIGI_MIC,
    MSCC_DIGI_SPEAKER,
    apply_all,
    config_dir,
    is_pty_name,
)
from .devices import (
    AudioDevice,
    SerialChoice,
    list_audio_devices,
    list_serial_ports,
    read_multus_serial,
)

# Processes that hold audio/CAT — must not reconfigure while running
_MSCC_SERVER_PROCS = ("sdrcore-recv", "sdrcore-trans", "ms-sdr")


def _running_mscc_servers() -> List[str]:
    """Return names of MSCC server processes that appear to be running."""
    found: List[str] = []
    for name in _MSCC_SERVER_PROCS:
        try:
            r = subprocess.run(
                ["pgrep", "-x", name],
                capture_output=True,
                text=True,
                timeout=5,
            )
            if r.returncode == 0 and (r.stdout or "").strip():
                found.append(name)
        except Exception:
            continue
    return found


def _resolve_mscc_cmd() -> Optional[List[str]]:
    """Return argv prefix to run mscc, or None."""
    cmd = shutil.which("mscc")
    if cmd:
        return [cmd]
    from pathlib import Path

    script = Path.home() / "mscc" / "mscc.sh"
    if script.is_file():
        return ["bash", str(script)]
    ctl = shutil.which("mscc-desktop-ctl")
    if ctl:
        return [ctl]
    return None


class MsccInitApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(f"MSCC Init  v{__version__}")
        self.minsize(640, 480)
        self.geometry("720x560")

        # Wizard state
        self.serial = "UNKNOWN"
        self.serial_status = ""
        # MKII = ms-sdr PTT sense thread; legacy Proficio → False
        self.proficio_mkii = tk.BooleanVar(value=True)
        self.keyer = tk.BooleanVar(value=False)
        self.serial_choices: List[SerialChoice] = []
        self.cat_index = tk.IntVar(value=0)
        self.pin = tk.IntVar(value=0)
        self.speakers: List[AudioDevice] = []
        self.mics: List[AudioDevice] = []
        self.speaker_index = tk.IntVar(value=0)
        self.mic_index = tk.IntVar(value=0)
        self.audio_error: Optional[str] = None
        self._abort = False

        self._step = 0
        self._frames: List[ttk.Frame] = []

        # Servers hold PortAudio/CAT — stop before reconfiguring
        if not self._check_servers_before_init():
            self._abort = True
            self.destroy()
            return

        outer = ttk.Frame(self, padding=12)
        outer.pack(fill=tk.BOTH, expand=True)

        self.hdr = ttk.Label(
            outer,
            text="MSCC Init — configure this Pi",
            font=("TkDefaultFont", 14, "bold"),
        )
        self.hdr.pack(anchor=tk.W)

        self.sub = ttk.Label(
            outer,
            text=f"Config directory: {config_dir()}",
            foreground="#444",
        )
        self.sub.pack(anchor=tk.W, pady=(0, 8))

        self.body = ttk.Frame(outer)
        self.body.pack(fill=tk.BOTH, expand=True)

        nav = ttk.Frame(outer)
        nav.pack(fill=tk.X, pady=(12, 0))
        self.btn_back = ttk.Button(nav, text="← Back", command=self._back)
        self.btn_back.pack(side=tk.LEFT)
        self.btn_next = ttk.Button(nav, text="Next →", command=self._next)
        self.btn_next.pack(side=tk.RIGHT)
        self.btn_cancel = ttk.Button(nav, text="Cancel", command=self.destroy)
        self.btn_cancel.pack(side=tk.RIGHT, padx=(0, 8))

        self._build_steps()
        self._show_step(0)

    def _check_servers_before_init(self) -> bool:
        """
        If MSCC servers are running, offer to stop them.
        Returns True to continue the wizard, False to exit (user declined or stop failed).
        """
        running = _running_mscc_servers()
        if not running:
            return True

        names = ", ".join(running)
        yes = messagebox.askyesno(
            "MSCC servers are running",
            "MSCC servers are running on this Pi:\n\n"
            f"  {names}\n\n"
            "Configuration cannot safely change while they hold audio/CAT.\n\n"
            "Stop the servers and continue with MSCC Init?\n\n"
            "Yes = stop servers and continue\n"
            "No  = exit without changing configuration",
            icon=messagebox.WARNING,
            default=messagebox.YES,
        )
        if not yes:
            messagebox.showinfo(
                "MSCC Init",
                "Initialization cancelled.\n"
                "Stop servers first (menu: MSCC Stop, or: mscc stop), then run MSCC Init again.",
            )
            return False

        ok, detail = self._stop_mscc_servers()
        if not ok:
            messagebox.showerror(
                "Could not stop servers",
                "Failed to stop MSCC servers:\n\n"
                f"{detail}\n\n"
                "Stop them manually (mscc stop), then run MSCC Init again.",
            )
            return False

        still = _running_mscc_servers()
        if still:
            messagebox.showerror(
                "Servers still running",
                "These processes are still running after stop:\n\n"
                f"  {', '.join(still)}\n\n"
                "Stop them manually, then run MSCC Init again.",
            )
            return False

        messagebox.showinfo(
            "MSCC Init",
            "Servers stopped. Continuing with configuration.",
        )
        return True

    def _stop_mscc_servers(self) -> Tuple[bool, str]:
        """Run mscc stop (or mscc-desktop-ctl stop). Returns (ok, message)."""
        argv = _resolve_mscc_cmd()
        if not argv:
            return False, (
                "mscc not found on PATH.\n"
                "Install the mscc package, or ensure $HOME/mscc/mscc.sh exists."
            )
        try:
            r = subprocess.run(
                argv + ["stop"],
                capture_output=True,
                text=True,
                timeout=60,
            )
            out = ((r.stdout or "") + (r.stderr or "")).strip()
            if r.returncode == 0:
                return True, out or "mscc stop OK"
            return False, out or f"stop failed (exit {r.returncode})"
        except Exception as e:
            return False, str(e)

    def _build_steps(self) -> None:
        self._frames = [
            self._step_welcome(),
            self._step_keyer(),
            self._step_cat(),
            self._step_audio(),
            self._step_finish(),
        ]
        for f in self._frames:
            f.place(in_=self.body, x=0, y=0, relwidth=1, relheight=1)

    def _step_welcome(self) -> ttk.Frame:
        f = ttk.Frame(self.body, padding=8)
        ttk.Label(
            f,
            text="Step 1 of 4 — Transceiver USB (optional)",
            font=("TkDefaultFont", 11, "bold"),
        ).pack(anchor=tk.W)
        ttk.Label(
            f,
            text=(
                "Looks for Multus/Proficio USB control (16c0:05dc) and reads the serial.\n"
                "You can continue without a radio attached."
            ),
            wraplength=640,
            justify=tk.LEFT,
        ).pack(anchor=tk.W, pady=8)

        self.lbl_usb = ttk.Label(f, text="Click “Scan USB” to probe.", wraplength=640)
        self.lbl_usb.pack(anchor=tk.W, pady=8)
        ttk.Button(f, text="Scan USB", command=self._scan_usb).pack(anchor=tk.W)
        ttk.Label(
            f,
            text=(
                "Digi audio is fixed by install (not selectable):\n"
                f"  digital speaker = {MSCC_DIGI_SPEAKER}\n"
                f"  digital mic     = {MSCC_DIGI_MIC}\n"
                "Create sinks with:  mscc-virtual-audio"
            ),
            wraplength=640,
            justify=tk.LEFT,
            foreground="#333",
        ).pack(anchor=tk.W, pady=(16, 0))
        return f

    def _step_keyer(self) -> ttk.Frame:
        f = ttk.Frame(self.body, padding=8)
        ttk.Label(
            f,
            text="Step 2 of 4 — Radio family & keyer",
            font=("TkDefaultFont", 11, "bold"),
        ).pack(anchor=tk.W)
        ttk.Checkbutton(
            f,
            text="Proficio MKII transceiver is attached (not a legacy Proficio)",
            variable=self.proficio_mkii,
        ).pack(anchor=tk.W, pady=(12, 4))
        ttk.Label(
            f,
            text=(
                "Checked: ms-sdr runs the MKII rear PTT sense thread "
                "(PROFICIO-MKII=1).\n"
                "Unchecked: legacy — that thread is skipped (PROFICIO-MKII=0)."
            ),
            wraplength=640,
            foreground="#333",
        ).pack(anchor=tk.W)
        ttk.Checkbutton(
            f,
            text="Proficio MKII keyer is installed",
            variable=self.keyer,
        ).pack(anchor=tk.W, pady=12)
        ttk.Label(
            f,
            text="Writes cw.ini, mscc.ini (after finish), and i2c.ini defaults.",
            wraplength=640,
        ).pack(anchor=tk.W)
        return f

    def _step_cat(self) -> ttk.Frame:
        f = ttk.Frame(self.body, padding=8)
        ttk.Label(
            f,
            text="Step 3 of 4 — Kenwood CAT port & PTT pin",
            font=("TkDefaultFont", 11, "bold"),
        ).pack(anchor=tk.W)

        ttk.Label(f, text="CAT port:").pack(anchor=tk.W, pady=(8, 0))
        self.cat_list = tk.Listbox(f, height=10, exportselection=False)
        self.cat_list.pack(fill=tk.BOTH, expand=True, pady=4)
        self.cat_list.bind("<<ListboxSelect>>", self._on_cat_select)

        pin_fr = ttk.Frame(f)
        pin_fr.pack(fill=tk.X, pady=8)
        ttk.Label(pin_fr, text="PTT pin (ms-sdr on CAT port):").pack(side=tk.LEFT)
        for val, lab in (
            (0, "0 none"),
            (1, "1 CTS (tty0tty)"),
            (2, "2 DCD"),
        ):
            ttk.Radiobutton(pin_fr, text=lab, variable=self.pin, value=val).pack(
                side=tk.LEFT, padx=6
            )

        ttk.Label(
            f,
            text=(
                "Tip: PTY = CAT only. For digi PTT use tty0tty (e.g. /dev/tnt0) with PIN=1 (CTS)."
            ),
            wraplength=640,
            foreground="#333",
        ).pack(anchor=tk.W)
        return f

    def _step_audio(self) -> ttk.Frame:
        f = ttk.Frame(self.body, padding=8)
        ttk.Label(
            f,
            text="Step 4 of 4 — Operator audio",
            font=("TkDefaultFont", 11, "bold"),
        ).pack(anchor=tk.W)
        ttk.Label(
            f,
            text=(
                "Select operator speaker and microphone (any sample rate is OK;\n"
                "sdrcore resamples when the device is not 96 kHz).\n"
                "Proficio/Multus radio I/Q devices are hidden here (not phones/mic).\n"
                f"Digi is fixed: {MSCC_DIGI_SPEAKER} / {MSCC_DIGI_MIC}"
            ),
            wraplength=640,
            justify=tk.LEFT,
        ).pack(anchor=tk.W, pady=6)

        ttk.Button(f, text="Refresh audio devices", command=self._load_audio).pack(
            anchor=tk.W
        )
        self.lbl_audio_msg = ttk.Label(f, text="", wraplength=640, foreground="#a00")
        self.lbl_audio_msg.pack(anchor=tk.W)

        pan = ttk.Panedwindow(f, orient=tk.HORIZONTAL)
        pan.pack(fill=tk.BOTH, expand=True, pady=8)

        left = ttk.Frame(pan)
        right = ttk.Frame(pan)
        pan.add(left, weight=1)
        pan.add(right, weight=1)

        ttk.Label(left, text="Operator speaker (playback)").pack(anchor=tk.W)
        self.spk_list = tk.Listbox(left, exportselection=False)
        self.spk_list.pack(fill=tk.BOTH, expand=True)

        ttk.Label(right, text="Operator microphone (capture)").pack(anchor=tk.W)
        self.mic_list = tk.Listbox(right, exportselection=False)
        self.mic_list.pack(fill=tk.BOTH, expand=True)
        return f

    def _step_finish(self) -> ttk.Frame:
        f = ttk.Frame(self.body, padding=8)
        ttk.Label(
            f,
            text="Apply configuration",
            font=("TkDefaultFont", 11, "bold"),
        ).pack(anchor=tk.W)
        self.summary = tk.Text(f, height=14, wrap=tk.WORD, state=tk.DISABLED)
        self.summary.pack(fill=tk.BOTH, expand=True, pady=8)
        ttk.Label(
            f,
            text=(
                "Press “Write config” to save.\n"
                "You will then be asked whether to start the MSCC servers (Yes / No)."
            ),
            wraplength=640,
            foreground="#333",
            justify=tk.LEFT,
        ).pack(anchor=tk.W)
        return f

    def _scan_usb(self) -> None:
        sn, msg = read_multus_serial()
        self.serial = sn
        self.serial_status = msg
        self.lbl_usb.configure(text=f"{msg}\nStored serial: {sn}")

    def _load_serial(self) -> None:
        self.serial_choices = list_serial_ports()
        self.cat_list.delete(0, tk.END)
        for i, c in enumerate(self.serial_choices):
            self.cat_list.insert(tk.END, f"[{i}]  {c.path}  —  {c.label}")
        if self.serial_choices:
            self.cat_list.selection_set(0)
            self._on_cat_select()

    def _on_cat_select(self, _evt=None) -> None:
        sel = self.cat_list.curselection()
        if not sel:
            return
        idx = int(sel[0])
        self.cat_index.set(idx)
        port = self.serial_choices[idx].path
        if is_pty_name(port):
            self.pin.set(0)
        elif "tnt" in port:
            self.pin.set(1)
        else:
            if self.pin.get() == 0:
                pass  # leave user choice

    def _load_audio(self) -> None:
        self.spk_list.delete(0, tk.END)
        self.mic_list.delete(0, tk.END)
        self.speakers, err1 = list_audio_devices(want_input=False, require_96k=False)
        self.mics, err2 = list_audio_devices(want_input=True, require_96k=False)
        msg = err1 or err2
        self.audio_error = msg
        if msg:
            self.lbl_audio_msg.configure(text=msg, foreground="#a00")
        elif not self.speakers and not self.mics:
            self.lbl_audio_msg.configure(
                text=(
                    "No operator audio devices found.\n"
                    "Check PortAudio/Pulse and that the sound card is connected."
                ),
                foreground="#a00",
            )
        else:
            self.lbl_audio_msg.configure(
                text=f"Found {len(self.speakers)} speaker(s), {len(self.mics)} mic(s).",
                foreground="#060",
            )
        for d in self.speakers:
            extra = f"  ← {d.hint}" if d.hint else ""
            self.spk_list.insert(
                tk.END,
                f"[{d.index}] ch={d.channels}  {d.host_api}  {d.name}{extra}",
            )
        for d in self.mics:
            extra = f"  ← {d.hint}" if d.hint else ""
            self.mic_list.insert(
                tk.END,
                f"[{d.index}] ch={d.channels}  {d.host_api}  {d.name}{extra}",
            )
        if self.speakers:
            self.spk_list.selection_set(0)
        if self.mics:
            self.mic_list.selection_set(0)

    def _show_step(self, n: int) -> None:
        self._step = n
        for i, fr in enumerate(self._frames):
            if i == n:
                fr.lift()
        self.btn_back.configure(state=tk.NORMAL if n > 0 else tk.DISABLED)
        if n == 0:
            self.btn_next.configure(text="Next →")
        elif n == len(self._frames) - 1:
            self.btn_next.configure(text="Write config")
        elif n == len(self._frames) - 2:
            self.btn_next.configure(text="Review →")
        else:
            self.btn_next.configure(text="Next →")

        if n == 2:
            self._load_serial()
        if n == 3:
            self._load_audio()
        if n == 4:
            self._fill_review()

    def _selected_cat(self) -> SerialChoice:
        sel = self.cat_list.curselection()
        idx = int(sel[0]) if sel else 0
        if not self.serial_choices:
            return SerialChoice("PTY", "")
        return self.serial_choices[max(0, min(idx, len(self.serial_choices) - 1))]

    def _selected_speaker(self) -> Optional[AudioDevice]:
        sel = self.spk_list.curselection()
        if not sel or not self.speakers:
            return None
        return self.speakers[int(sel[0])]

    def _selected_mic(self) -> Optional[AudioDevice]:
        sel = self.mic_list.curselection()
        if not sel or not self.mics:
            return None
        return self.mics[int(sel[0])]

    def _fill_review(self) -> None:
        cat = self._selected_cat()
        pin = self.pin.get()
        if is_pty_name(cat.path):
            pin = 0
        sp = self._selected_speaker()
        mic = self._selected_mic()
        text = (
            f"Serial:     {self.serial}\n"
            f"MKII radio: {'yes' if self.proficio_mkii.get() else 'no (legacy)'}\n"
            f"Keyer:      {'yes' if self.keyer.get() else 'no'}\n"
            f"CAT:        {cat.path}\n"
            f"PIN:        {pin}\n"
            f"Digi out:   {MSCC_DIGI_SPEAKER}  (fixed)\n"
            f"Digi in:    {MSCC_DIGI_MIC}  (fixed)\n"
            f"Op speaker: {sp.name if sp else '(none)'}\n"
            f"Op mic:     {mic.name if mic else '(none)'}\n"
            f"\nDirectory:  {config_dir()}\n"
            "\nPress “Write config” to save.\n"
            "Then Yes/No: start MSCC servers?"
        )
        self.summary.configure(state=tk.NORMAL)
        self.summary.delete("1.0", tk.END)
        self.summary.insert(tk.END, text)
        self.summary.configure(state=tk.DISABLED)

    def _back(self) -> None:
        if self._step > 0:
            self._show_step(self._step - 1)

    def _next(self) -> None:
        if self._step == 3:
            if not self._selected_speaker() or not self._selected_mic():
                if not messagebox.askyesno(
                    "Missing audio",
                    "No operator speaker and/or mic selected.\n"
                    "Continue to review anyway?",
                ):
                    return
        if self._step >= len(self._frames) - 1:
            self._write()
            return
        self._show_step(self._step + 1)

    def _start_mscc_servers(self) -> Tuple[bool, str]:
        """Run `mscc start` (or $HOME/mscc/mscc.sh start). Returns (ok, message)."""
        argv = _resolve_mscc_cmd()
        if not argv:
            return False, (
                "mscc not found on PATH.\n"
                "Install the mscc package, or ensure $HOME/mscc/mscc.sh exists."
            )
        try:
            r = subprocess.run(
                argv + ["start"],
                capture_output=True,
                text=True,
                timeout=60,
            )
            out = ((r.stdout or "") + (r.stderr or "")).strip()
            if r.returncode == 0:
                return True, out or "mscc start OK"
            return False, out or f"mscc start failed (exit {r.returncode})"
        except Exception as e:
            return False, str(e)

    def _write(self) -> None:
        cat = self._selected_cat()
        pin = int(self.pin.get())
        if is_pty_name(cat.path):
            pin = 0
        sp = self._selected_speaker()
        mic = self._selected_mic()
        if not sp or not mic:
            messagebox.showerror(
                "Cannot write",
                "Operator speaker and microphone are required.\n"
                "Go back and select devices (install python3-pyaudio if needed).",
            )
            return
        try:
            summary = apply_all(
                serial=self.serial,
                keyer_installed=bool(self.keyer.get()),
                cat_port=cat.path,
                cat_pin=pin,
                operator_speaker=sp.name,
                operator_mic=mic.name,
                proficio_mkii=bool(self.proficio_mkii.get()),
            )
        except OSError as e:
            messagebox.showerror("Write failed", str(e))
            return

        # Keep dialogs short — long path dumps push OK off-screen on small displays
        cfg = str(config_dir())
        level_notes = [
            ln
            for ln in summary
            if "ALSA" in ln or "100%" in ln or "Remembered" in ln or "card" in ln.lower()
        ]
        level_hint = ""
        if any("Remembered" in ln or "100%" in ln for ln in level_notes):
            level_hint = (
                "\n\nALSA operator levels set to 100%.\n"
                "mscc start re-applies them every time (not .asoundrc)."
            )
        elif level_notes:
            level_hint = (
                "\n\nNote: could not match ALSA card for levels; "
                "use alsamixer once if quiet."
            )
        start_now = messagebox.askyesno(
            "Start MSCC servers?",
            "Configuration written.\n\n"
            f"Directory:\n  {cfg}"
            f"{level_hint}\n\n"
            "Start MSCC servers now?\n"
            "(sdrcore-recv, sdrcore-trans, ms-sdr)\n\n"
            "Yes = start servers\n"
            "No  = finish without starting\n"
            "    (start later: menu MSCC Start, or mscc start)",
            icon=messagebox.QUESTION,
            default=messagebox.YES,
        )
        if start_now:
            ok, detail = self._start_mscc_servers()
            if ok:
                messagebox.showinfo(
                    "MSCC Init",
                    "MSCC servers started.\n\n"
                    "Connect with the MSCC client on your PC.",
                )
            else:
                err = (detail or "unknown error").replace("\n", " ").strip()
                if len(err) > 160:
                    err = err[:157] + "..."
                messagebox.showwarning(
                    "MSCC Init",
                    "Servers did not start.\n\n"
                    f"{err}\n\n"
                    "Start manually:  mscc start\n"
                    "(or menu: MSCC Start)",
                )
        else:
            messagebox.showinfo(
                "MSCC Init",
                "Done.\n\n"
                "Start servers later:  mscc start\n"
                "(or menu: MSCC Start)",
            )
        self.destroy()


def main() -> None:
    app = MsccInitApp()
    if getattr(app, "_abort", False):
        return
    app.mainloop()


if __name__ == "__main__":
    main()
