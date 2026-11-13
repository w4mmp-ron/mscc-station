#!/usr/bin/env python3
"""
keyer-flash-gui.py: window for updating the PIC keyer on the Proficio daughter board.

    python3 keyer-flash-gui.py          (Pi)
    python keyer-flash-gui.py           (Windows)

Load File (a Keyer-MCU-app-....hex), then Program. Check Keyer shows whether the keyer
app or its bootloader answers. Start Keyer is greyed out unless the keyer is found
waiting in its bootloader (an update cut short); then it starts the keyer as it is. The work is done by keyer-flash.py, which must be in the same folder.
The MSCC servers must be stopped first: if any is running, it says so and closes.
Needs pyusb and tkinter (python3-tk on the Pi).
"""
import importlib.util
import os
import queue
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

FROZEN = getattr(sys, "frozen", False)     # Windows keyer-flash-gui.exe (PyInstaller)
# HERE = folder the program sits in (Windows: with the keyer hex files); the engine
# keyer-flash.py sits beside the script, or inside the .exe when built.
HERE = (os.path.dirname(sys.executable) if FROZEN
        else os.path.dirname(os.path.realpath(__file__)))  # real folder, also via a link
ENGINE_DIR = getattr(sys, "_MEIPASS", HERE) if FROZEN else HERE
APP_TITLE = "Keyer Firmware Update"


def load_engine():
    path = os.path.join(ENGINE_DIR, "keyer-flash.py")
    if not os.path.isfile(path):
        raise SystemExit("keyer-flash.py not found next to this script (%s)" % ENGINE_DIR)
    spec = importlib.util.spec_from_file_location("keyer_flash", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


kf = load_engine()


def default_dir():
    """Where the keyer hex files are: Pi /usr/share/mscc/firmware/keyer (its own folder,
    beside the radio firmware folders), else a firmware folder next to this script, else
    this script's folder."""
    for d in ("/usr/share/mscc/firmware/keyer",
              os.path.join(HERE, "firmware"),
              HERE):
        if os.path.isdir(d):
            return d
    return os.path.expanduser("~")


class App:
    def __init__(self, root):
        self.root = root
        self.q = queue.Queue()
        self.busy = False
        root.title(APP_TITLE)
        root.minsize(560, 380)

        f = ttk.Frame(root, padding=10)
        f.pack(fill="both", expand=True)
        f.columnconfigure(0, weight=1)

        ttk.Label(f, text="Keyer firmware file (Keyer-MCU-app-....hex):").grid(
            row=0, column=0, columnspan=2, sticky="w")
        self.path = tk.StringVar()
        ttk.Entry(f, textvariable=self.path).grid(row=1, column=0, sticky="ew", padx=(0, 6))
        self.b_load = ttk.Button(f, text="Load File...", command=self.pick)
        self.b_load.grid(row=1, column=1, sticky="ew")

        bar = ttk.Frame(f)
        bar.grid(row=2, column=0, columnspan=2, sticky="w", pady=(10, 6))
        self.b_prog = ttk.Button(bar, text="Program", command=self.program)
        self.b_check = ttk.Button(bar, text="Check Keyer", command=self.check)
        # Start Keyer: only when the keyer is found waiting in its bootloader
        self.b_start = ttk.Button(bar, text="Start Keyer", command=self.start_keyer,
                                  state="disabled")
        for b in (self.b_prog, self.b_check, self.b_start):
            b.pack(side="left", padx=(0, 6))
        self.stuck = False

        self.pbar = ttk.Progressbar(f, mode="determinate", maximum=100)
        self.pbar.grid(row=3, column=0, columnspan=2, sticky="ew")
        self.phase = tk.StringVar(value="")
        ttk.Label(f, textvariable=self.phase).grid(row=4, column=0, columnspan=2, sticky="w")

        lf = ttk.Frame(f)
        lf.grid(row=5, column=0, columnspan=2, sticky="nsew", pady=(6, 0))
        f.rowconfigure(5, weight=1)
        self.log = tk.Text(lf, height=12, wrap="word", state="disabled")
        sb = ttk.Scrollbar(lf, command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        self.status = tk.StringVar(value="Ready.")
        ttk.Label(f, textvariable=self.status, relief="sunken", anchor="w").grid(
            row=6, column=0, columnspan=2, sticky="ew", pady=(6, 0))

        # engine output -> queue -> window (the engine runs in a worker thread)
        kf.say = lambda text: self.q.put(("log", text))
        kf.progress = lambda done, total, phase: self.q.put(("prog", (done, total, phase)))
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(100, self.pump)
        self.write("Servers are stopped. Pick the keyer firmware file, then press Program.")

    # ---- window helpers -------------------------------------------------

    def write(self, text):
        self.log.configure(state="normal")
        self.log.insert("end", time.strftime("%H:%M:%S  ") + text + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def set_busy(self, on, what=""):
        self.busy = on
        st = "disabled" if on else "normal"
        for b in (self.b_load, self.b_prog, self.b_check):
            b.configure(state=st)
        self.b_start.configure(state="normal" if (not on and self.stuck) else "disabled")
        self.status.set(what if on else "Ready.")

    def pump(self):
        try:
            while True:
                kind, val = self.q.get_nowait()
                if kind == "log":
                    self.write(val)
                elif kind == "prog":
                    done, total, phase = val
                    # writing = first 70 %, checking = last 30 %
                    pct = 70.0 * done / total if phase == "write" else 70 + 30.0 * done / total
                    self.pbar["value"] = pct
                    self.phase.set("%s row %d of %d" % ("Writing" if phase == "write"
                                                        else "Checking", done, total))
                elif kind == "done":
                    ok, msg, stuck = val
                    if stuck is not None:
                        if stuck and not self.stuck:
                            self.write("The keyer is waiting in its bootloader. Press Program "
                                       "to finish an update, or Start Keyer to run it as it is.")
                        self.stuck = stuck
                    self.set_busy(False)
                    self.status.set(msg)
                    if ok is True:
                        messagebox.showinfo(APP_TITLE, msg)
                    elif ok is False:
                        messagebox.showerror(APP_TITLE, msg)
        except queue.Empty:
            pass
        self.root.after(100, self.pump)

    def servers_ok(self):
        """Servers running: tell the user and close the window."""
        running = kf.servers_running()
        if not running:
            return True
        messagebox.showerror(APP_TITLE, kf.servers_message(running))
        self.root.destroy()
        return False

    def run(self, what, job):
        if not self.servers_ok():
            return
        self.set_busy(True, what)
        threading.Thread(target=self.worker, args=(job,), daemon=True).start()

    def worker(self, job):
        try:
            res = job()
        except kf.Fail as e:
            res = (False, "FAILED: %s" % e)
        except SystemExit as e:             # kf.dev(): radio not found
            res = (False, str(e))
        except Exception as e:              # anything else: show it, don't die
            res = (False, "Error: %s" % e)
        # after every job: is the keyer left in its bootloader? (enables Start Keyer)
        try:
            stuck = kf.answers(kf.dev(), kf.BOOT_ADDR)
        except (kf.Fail, SystemExit, Exception):
            stuck = None
        self.q.put(("log", res[1]))
        self.q.put(("done", (res[0], res[1], stuck)))

    def radio(self):
        d = kf.dev()
        if not kf.supported(d):
            raise kf.Fail("radio firmware has no keyer update (opcode 0x9F); needs Black Pill 3.16, Proficio MKII PTT 3.244 / ATU 4.245, Geminus MKII 2.154, Ultimus MKII PTT 8.244 / ATU 7.245 or later")
        return d

    # ---- buttons --------------------------------------------------------

    def pick(self):
        cur = self.path.get().strip()
        start = os.path.dirname(cur) if cur and os.path.isdir(os.path.dirname(cur)) \
            else default_dir()
        p = filedialog.askopenfilename(
            title="Keyer firmware", initialdir=start,
            filetypes=[("Keyer app", "Keyer-MCU-app-*.hex"), ("Hex files", "*.hex"),
                       ("All files", "*")])
        if p:
            self.path.set(p)
            try:
                rows = kf.rows_of(kf.load_hex(p))
                self.write("%s: %d rows, looks right" % (os.path.basename(p), len(rows)))
            except kf.Fail as e:
                self.write("This file will not do: %s" % e)

    def check(self):
        def job():
            return (None, {"boot": "Keyer is in its bootloader.",
                           "app": "Keyer app is answering.",
                           None: "Keyer is not answering."}[kf.check(self.radio())])
        self.run("Checking the keyer...", job)

    def start_keyer(self):
        def job():
            d = self.radio()
            if not kf.answers(d, kf.BOOT_ADDR):
                return (None, "Keyer is already running.")
            if kf.leave_boot(d):
                return (True, "Keyer running.")
            return (False, "The keyer did not start: its firmware is not complete. "
                           "Press Program.")
        self.run("Starting the keyer...", job)

    def program(self):
        p = self.path.get().strip()
        if not p or not os.path.isfile(p):
            messagebox.showwarning(APP_TITLE, "Pick the keyer firmware file first (Load File).")
            return
        try:
            kf.load_hex(p)
        except kf.Fail as e:
            messagebox.showerror(APP_TITLE, str(e))
            return
        if not self.servers_ok():
            return
        if not messagebox.askokcancel(APP_TITLE, "Program the keyer with\n%s ?"
                                      % os.path.basename(p)):
            return
        self.pbar["value"] = 0
        self.phase.set("")

        def job():
            try:
                kf.flash(self.radio(), p)
                return (True, "Keyer updated and running. Start the servers again when "
                              "you are ready.")
            except kf.Fail as e:
                return (False, "FAILED: %s\nThe keyer waits in its bootloader: press "
                               "Program again." % e)
        self.run("Programming the keyer...", job)

    def close(self):
        if self.busy and not messagebox.askyesno(
                APP_TITLE, "An update is running. Closing now leaves the keyer in its "
                           "bootloader (run Program again later).\n\nClose anyway?"):
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    running = kf.servers_running()
    if running:                     # no keyer updates while the servers run
        root.withdraw()
        messagebox.showerror(APP_TITLE, kf.servers_message(running))
        root.destroy()
        return
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
