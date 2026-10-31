#!/usr/bin/env python3
"""
iq-phase.py - slider for the transmit I/Q phase correction in sdrcore-trans.

Talks straight to sdrcore-trans on the Pi (UDP port 9200), not through ms-sdr or
the client. Needs the sdrcore-trans build that has the phase correction.

The slider acts on the band the rig is on (or the band of a running TX IQ
session). It is live at once but only kept after SAVE, which writes iq.ini on
the Pi. An unsaved value is lost at the next band change, like the amplitude.

Run:    python iq-phase.py --host proficio
        python iq-phase.py --host 192.168.1.50
        python iq-phase.py --host 127.0.0.1        (on the Pi itself)

Keys:   Left / Right   one step (0.05 deg)
        Up / Down      ten steps (0.5 deg)
"""

import argparse
import socket
import struct
import time
import tkinter as tk

CMD_SET_IQ_PHASE = 0x2A
CMD_GET_IQ_PHASE = 0x2B
CMD_SET_COMMIT_IQ = 0x57
STEP_DEG = 0.05
MAX_STEPS = 400
NO_BAND = 200
BANDS = ["10 m", "12 m", "15 m", "17 m", "20 m", "30 m", "40 m", "60 m", "80 m", "160 m", "630 m", "2200 m"]


def parse_args():
    p = argparse.ArgumentParser(description="Slider for the transmit I/Q phase correction.")
    p.add_argument("--host", default="proficio", help="name or address of the Pi (default proficio)")
    p.add_argument("--port", type=int, default=9200, help="sdrcore-trans UDP port (default 9200)")
    return p.parse_args()


class App:
    def __init__(self, root, host, port):
        self.root = root
        self.port = port
        self.addr = None
        self.record = None          # record sdrcore-trans last reported
        self.last_reply = 0.0
        self.last_poll = 0.0
        self.unsaved = False
        self.in_use = None          # value sdrcore-trans has now, as far as we know

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setblocking(False)

        root.title("TX I/Q phase")
        top = tk.Frame(root, padx=12, pady=8)
        top.pack(fill="x")
        tk.Label(top, text="Pi:").pack(side="left")
        self.host_var = tk.StringVar(value=host)
        entry = tk.Entry(top, textvariable=self.host_var, width=18)
        entry.pack(side="left", padx=4)
        entry.bind("<Return>", lambda e: self.connect())
        tk.Button(top, text="Connect", command=self.connect).pack(side="left")
        self.status = tk.Label(top, text="", anchor="w")
        self.status.pack(side="left", padx=12)

        self.band = tk.Label(root, text="band: ---", font=("TkDefaultFont", 14))
        self.band.pack(pady=(4, 0))
        self.value = tk.Label(root, text="", font=("TkFixedFont", 22))
        self.value.pack()

        self.scale = tk.Scale(root, from_=-MAX_STEPS, to=MAX_STEPS, orient="horizontal", length=700,
                              resolution=1, showvalue=False, tickinterval=100, command=self.on_slider)
        self.scale.pack(padx=12)
        self.scale.focus_set()
        root.bind("<Up>", lambda e: self.nudge(10))
        root.bind("<Down>", lambda e: self.nudge(-10))

        row = tk.Frame(root, pady=8)
        row.pack()
        for text, steps in (("-0.5", -10), ("-0.05", -1), ("+0.05", 1), ("+0.5", 10)):
            tk.Button(row, text=text, width=6, command=lambda s=steps: self.nudge(s)).pack(side="left", padx=3)
        tk.Button(row, text="ZERO", width=7, command=lambda: self.set_steps(0)).pack(side="left", padx=(20, 3))
        self.save_btn = tk.Button(row, text="SAVE", width=7, command=self.save)
        self.save_btn.pack(side="left", padx=3)

        self.note = tk.Label(root, text="", anchor="w", padx=12, pady=6)
        self.note.pack(fill="x")

        self.show_value()
        self.connect()
        self.tick()

    def connect(self):
        host = self.host_var.get().strip()
        try:
            self.addr = (socket.gethostbyname(host), self.port)
        except OSError as e:
            self.addr = None
            self.status.config(text=f"cannot find {host}: {e}", fg="red")
            return
        self.record = None
        self.last_reply = 0.0
        self.status.config(text=f"asking {self.addr[0]} ...", fg="black")
        self.send(CMD_GET_IQ_PHASE, 0)

    def send(self, opcode, value):
        if self.addr is None:
            return
        try:
            self.sock.sendto(struct.pack("<Bi", opcode, value), self.addr)
        except OSError as e:
            self.status.config(text=f"send failed: {e}", fg="red")

    def show_value(self):
        steps = int(self.scale.get())
        mark = "   (not saved)" if self.unsaved else ""
        self.value.config(text=f"{steps * STEP_DEG:+6.2f} deg   ({steps:+d}){mark}")

    def on_slider(self, _):
        steps = int(self.scale.get())
        # the slider also reports when it is set from a reply: nothing to send then
        if steps != self.in_use and self.record is not None and self.record < len(BANDS):
            self.in_use = steps
            self.unsaved = True
            self.send(CMD_SET_IQ_PHASE, steps)
        self.show_value()

    def set_steps(self, steps):
        self.scale.set(max(-MAX_STEPS, min(MAX_STEPS, steps)))

    def nudge(self, steps):
        self.set_steps(int(self.scale.get()) + steps)

    def save(self):
        if self.record is None or self.record >= len(BANDS):
            return
        self.send(CMD_SET_COMMIT_IQ, 0)
        self.unsaved = False
        self.show_value()
        self.note.config(text=f"SAVE sent for {BANDS[self.record]} at {time.strftime('%H:%M:%S')} "
                              "(check iq.ini on the Pi)")

    def on_reply(self, data):
        if len(data) < 10 or data[0] != CMD_GET_IQ_PHASE:
            return
        _, record, phase, offset = struct.unpack("<BBii", data[:10])
        self.last_reply = time.monotonic()
        if record >= len(BANDS):
            self.band.config(text="band: none yet (sdrcore-trans has no band)")
            self.record = record
            return
        self.band.config(text=f"band: {BANDS[record]}     amplitude value: {offset}")
        # take the server's value on first contact and on a band change only,
        # so a late reply never fights the slider while it is being moved
        if record != self.record:
            self.record = record
            self.unsaved = False
            self.in_use = phase
            self.scale.set(phase)
            self.show_value()

    def tick(self):
        while True:
            try:
                data, _ = self.sock.recvfrom(64)
            except (BlockingIOError, ConnectionResetError):
                break
            except OSError:
                break
            self.on_reply(data)
        now = time.monotonic()
        if self.addr is not None:
            if now - self.last_poll >= 1.0:
                self.last_poll = now
                self.send(CMD_GET_IQ_PHASE, 0)
            if self.last_reply and now - self.last_reply < 3.0:
                self.status.config(text=f"connected to {self.addr[0]}", fg="dark green")
            elif now - self.last_poll < 0.5 and self.last_reply == 0.0:
                pass
            else:
                self.status.config(text=f"no answer from {self.addr[0]}:{self.port}", fg="red")
        self.root.after(200, self.tick)


def main():
    args = parse_args()
    root = tk.Tk()
    App(root, args.host, args.port)
    root.mainloop()


if __name__ == "__main__":
    main()
