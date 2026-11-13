#!/usr/bin/env python3
"""
stm32-app-flash-gui.py: window for updating the Black Pill (radio) firmware on the Pi.

    stm32-app-flash.gui          (menu: Black Pill Firmware Update)

Load File (a BlackPill-25MHz-<ver>-app.bin), then Program. Check Radio shows the firmware
the radio runs, or that the pill is waiting in flash mode. The work is done by
stm32-app-flash.sh (same folder), which puts the pill into flash mode by itself, flashes
the app at 0x08008000 with dfu-util and waits for the radio to come back.
Application only: the bootloader (boot-app image) is an ST-Link job (stm32-bootloader-flash).
The MSCC servers must be stopped first: if any is running, it says so and closes.
No sudo: the udev rule 99-proficio.rules opens the radio (16c0:05dc) and flash mode (0483:df11).
Needs python3-tk, python3-usb, dfu-util.
"""
import os
import queue
import re
import subprocess
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

HERE = os.path.dirname(os.path.realpath(__file__))     # real folder, also via a link
SCRIPT = os.path.join(HERE, "stm32-app-flash.sh")
APP_TITLE = "Black Pill Firmware Update"
SERVERS = ("ms-sdr", "sdrcore-recv", "sdrcore-trans")
FW_DIRS = ("/usr/share/mscc/firmware/black-pill", HERE)
APP_SIZE_MAX = 480 * 1024          # app area 0x08008000..0x0807FFFF on the F411


def servers_running():
    found = []
    for p in os.listdir("/proc"):
        if p.isdigit():
            try:
                with open("/proc/%s/comm" % p) as f:
                    name = f.read().strip()
            except OSError:
                continue
            if name in SERVERS and name not in found:
                found.append(name)
    return found


def servers_message(names):
    return ("The MSCC servers are running (%s). Stop them (mscc stop), then run the "
            "Black Pill update again." % ", ".join(names))


def file_problem(path):
    """None if the file looks like an app image, else what is wrong."""
    name = os.path.basename(path)
    if not os.path.isfile(path):
        return "file not found"
    if name.endswith("-boot-app.bin"):
        return ("%s is a bootloader + app image (ST-Link only). Pick the -app.bin file."
                % name)
    if name.startswith("bootload"):
        return ("%s is the bootloader (ST-Link only). Pick the -app.bin file." % name)
    if not name.endswith(".bin"):
        return "not a .bin file"
    size = os.path.getsize(path)
    if size < 1024 or size > APP_SIZE_MAX:
        return "size %d bytes does not fit the app area" % size
    with open(path, "rb") as f:
        head = f.read(8)
    # vector table: initial stack pointer in RAM (0x2000xxxx), reset vector in the app area
    sp = int.from_bytes(head[0:4], "little")
    rv = int.from_bytes(head[4:8], "little")
    if (sp >> 24) != 0x20 or not (0x08008000 <= rv < 0x08080000):
        return "does not start like a Black Pill app at 0x08008000"
    return None


def usb_state():
    """('radio', '3.16') / ('dfu', None) / (None, None)."""
    import usb.core
    d = usb.core.find(idVendor=0x16C0, idProduct=0x05DC)
    if d is not None:
        try:
            v = d.ctrl_transfer(0xC0, 0x00, 0, 0, 2, 500)
            return "radio", "%d.%d" % (v[0], v[1])
        except usb.core.USBError:
            return "radio", "unknown"
    if usb.core.find(idVendor=0x0483, idProduct=0xDF11) is not None:
        return "dfu", None
    return None, None


def default_dir():
    for d in FW_DIRS:
        if os.path.isdir(d):
            return d
    return os.path.expanduser("~")


class App:
    def __init__(self, root):
        self.root = root
        self.q = queue.Queue()
        self.busy = False
        root.title(APP_TITLE)
        root.minsize(600, 400)

        f = ttk.Frame(root, padding=10)
        f.pack(fill="both", expand=True)
        f.columnconfigure(0, weight=1)

        ttk.Label(f, text="Black Pill firmware file (BlackPill-25MHz-<version>-app.bin):").grid(
            row=0, column=0, columnspan=2, sticky="w")
        self.path = tk.StringVar()
        ttk.Entry(f, textvariable=self.path).grid(row=1, column=0, sticky="ew", padx=(0, 6))
        self.b_load = ttk.Button(f, text="Load File...", command=self.pick)
        self.b_load.grid(row=1, column=1, sticky="ew")

        bar = ttk.Frame(f)
        bar.grid(row=2, column=0, columnspan=2, sticky="w", pady=(10, 6))
        self.b_prog = ttk.Button(bar, text="Program", command=self.program)
        self.b_check = ttk.Button(bar, text="Check Radio", command=self.check)
        for b in (self.b_prog, self.b_check):
            b.pack(side="left", padx=(0, 6))

        self.pbar = ttk.Progressbar(f, mode="determinate", maximum=100)
        self.pbar.grid(row=3, column=0, columnspan=2, sticky="ew")
        self.phase = tk.StringVar(value="")
        ttk.Label(f, textvariable=self.phase).grid(row=4, column=0, columnspan=2, sticky="w")

        lf = ttk.Frame(f)
        lf.grid(row=5, column=0, columnspan=2, sticky="nsew", pady=(6, 0))
        f.rowconfigure(5, weight=1)
        self.log = tk.Text(lf, height=14, wrap="word", state="disabled")
        sb = ttk.Scrollbar(lf, command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        self.status = tk.StringVar(value="Ready.")
        ttk.Label(f, textvariable=self.status, relief="sunken", anchor="w").grid(
            row=6, column=0, columnspan=2, sticky="ew", pady=(6, 0))

        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(100, self.pump)
        self.write("Servers are stopped. Pick the Black Pill firmware file, then press Program.")
        self.write("Application update only; the bootloader is not touched.")

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
        self.status.set(what if on else "Ready.")

    def pump(self):
        try:
            while True:
                kind, val = self.q.get_nowait()
                if kind == "log":
                    self.write(val)
                elif kind == "pct":
                    self.pbar["value"] = val
                    self.phase.set("Writing %d %%" % val)
                elif kind == "done":
                    ok, msg = val
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
        running = servers_running()
        if not running:
            return True
        messagebox.showerror(APP_TITLE, servers_message(running))
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
        except Exception as e:              # show it, don't die
            res = (False, "Error: %s" % e)
        self.q.put(("log", res[1]))
        self.q.put(("done", res))

    # ---- buttons --------------------------------------------------------

    def pick(self):
        cur = self.path.get().strip()
        start = os.path.dirname(cur) if cur and os.path.isdir(os.path.dirname(cur)) \
            else default_dir()
        p = filedialog.askopenfilename(
            title="Black Pill firmware", initialdir=start,
            # [0-9]: only <version>-app.bin; hides -boot-app.bin and bootload-PA8.bin (ST-Link)
            filetypes=[("Black Pill app", "BlackPill-*[0-9]-app.bin"), ("All files", "*")])
        if p:
            self.path.set(p)
            why = file_problem(p)
            if why:
                self.write("This file will not do: %s" % why)
            else:
                self.write("%s: %d bytes, looks right" % (os.path.basename(p),
                                                         os.path.getsize(p)))

    def check(self):
        def job():
            kind, ver = usb_state()
            if kind == "radio":
                return (None, "Radio running Black Pill firmware %s." % ver)
            if kind == "dfu":
                return (None, "The pill is waiting in flash mode (LED B). Press Program "
                              "to finish an update.")
            return (None, "No radio on USB. Is the rig powered and the USB cable in?")
        self.run("Checking the radio...", job)

    def program(self):
        p = self.path.get().strip()
        if not p:
            messagebox.showwarning(APP_TITLE, "Pick the Black Pill firmware file first "
                                              "(Load File).")
            return
        why = file_problem(p)
        if why:
            messagebox.showerror(APP_TITLE, why)
            return
        if not self.servers_ok():
            return
        if not messagebox.askokcancel(APP_TITLE, "Program the radio with\n%s ?"
                                      % os.path.basename(p)):
            return
        self.pbar["value"] = 0
        self.phase.set("")
        self.run("Programming the radio...", lambda: self.flash(p))

    def flash(self, path):
        """Run stm32-app-flash.sh; its output goes to the log, dfu-util's % to the bar."""
        if not os.path.isfile(SCRIPT):
            return (False, "stm32-app-flash.sh not found next to this program (%s)" % HERE)
        proc = subprocess.Popen(["bash", SCRIPT, path], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        lines, buf, last_pct = [], b"", -1
        while True:
            chunk = proc.stdout.read1(256) if hasattr(proc.stdout, "read1") \
                else proc.stdout.read(1)
            if not chunk:
                break
            buf += chunk
            # dfu-util redraws its progress line with \r: split on both
            parts = re.split(rb"[\r\n]", buf)
            buf = parts.pop()
            for raw in parts:
                line = raw.decode("utf-8", "replace").rstrip()
                if not line:
                    continue
                m = re.search(r"Download\s*\[.*\]\s*(\d+)%", line)
                if m:
                    pct = int(m.group(1))
                    if pct != last_pct:
                        last_pct = pct
                        self.q.put(("pct", pct))
                    continue
                lines.append(line)
                self.q.put(("log", line))
        rc = proc.wait()
        text = "\n".join(lines)
        if rc == 0 and "Done: radio running firmware" in text:
            ver = re.search(r"Done: radio running firmware (\S+?)\.?$", text, re.M)
            return (True, "Radio updated and running firmware %s. Start the servers again "
                          "when you are ready." % (ver.group(1) if ver else "?"))
        if rc == 0 and "still in the bootloader" in text:
            return (False, "Flashed, but the pill stays in its bootloader: remove the BOOT "
                           "jumper (PA8) and press NRST.")
        if rc == 0:
            return (False, "Flashed, but the radio did not come back on USB. Check the LED "
                           "letter.")
        return (False, "FAILED (see the messages above). If the pill waits in flash mode, "
                       "press Program again.")

    def close(self):
        if self.busy and not messagebox.askyesno(
                APP_TITLE, "An update is running. Closing now can leave the pill in flash "
                           "mode (run Program again later).\n\nClose anyway?"):
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    running = servers_running()
    if running:                     # no updates while the servers run
        root.withdraw()
        messagebox.showerror(APP_TITLE, servers_message(running))
        root.destroy()
        return
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
