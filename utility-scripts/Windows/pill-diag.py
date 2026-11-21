#!/usr/bin/env python3
"""
pill-diag.py: Black Pill diagnostics for Windows. One window, runs top to bottom, saves a
zip on the Desktop to send back. Built into pill-diag.exe (build-pill-diag.cmd), so the
person running it needs nothing installed.

Steps (MSCC must be closed):
  1  Find the radio: USB IDs, driver (WinUSB / libusb-win32), pill version
  2  Pill state: last reset, power-up I2C, hang record, counters (audio-stats.py)
  3  Codec + receive link: codec setup and read-back, receive frame errors now
  4  Receive: 10 s of raw I/Q (WASAPI exclusive), sample rate against the PC clock,
     bit / level / I-Q analysis, saved as a .wav
  5  Transmit test (optional, dummy load): the pill keyed for 5 s with a clean 12 kHz
     I/Q tone from this program at 1/10 of full scale, no servers involved
  6  Setup: MSCC ini files + logs, MSCC program versions, USB driver, Windows version
  7  Zip on the Desktop with summary.txt

Pill commands used (all existing): 0x00 version, 0xC9 stats, 0x32 set LO (Hz), 0x70 mode
('U' = not CW), 0x50 PTT (1 key / 0 unkey). The pill picks the band filters from the LO
and will not transmit outside a ham band.

Run from source: python pill-diag.py  (needs pyusb, sounddevice, numpy; libusb-1.0.dll and
audio-stats.py next to it)
"""
import contextlib
import datetime
import glob
import importlib.util
import io
import os
import platform
import queue
import struct
import subprocess
import sys
import threading
import time
import traceback
import wave
import zipfile

import json  # noqa: F401  - used by audio-stats.py; imported here so PyInstaller packs it
import numpy as np
import sounddevice as sd
import usb.backend.libusb0  # noqa: F401  - as above
import usb.backend.libusb1  # noqa: F401
import usb.core  # noqa: F401
import tkinter as tk
from tkinter import messagebox, scrolledtext, ttk

TOOL_VERSION = "1.0"
RATE = 96000
FULL = 32767.0
EXPECTED_RATE = 95991.6          # pill PLLI2S: 95.99 kHz, -87 ppm
RX_SECS = 10.0
TX_SECS = 5.0
TONE_HZ = 12000                  # 8 samples per cycle at 96 kHz
TONE_LEVEL = 0.1                 # of full scale: about 1/100 of full power
SERVER_PREFIXES = ("ms-sdr", "sdrcore", "mscc-recv", "mscc-trans")
BANDS = [("160 m", 1840000), ("80 m", 3573000), ("40 m", 7074000), ("30 m", 10136000),
         ("20 m", 14074000), ("17 m", 18100000), ("15 m", 21074000), ("12 m", 24915000),
         ("10 m", 28074000)]
CMD_VERSION, CMD_SET_FREQ, CMD_MODE, CMD_PTT = 0x00, 0x32, 0x70, 0x50
NO_WINDOW = 0x08000000           # CREATE_NO_WINDOW for helper commands


def here():
    """Folder of the bundled files (PyInstaller) or of this script."""
    return getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__)))


def load_audio_stats():
    spec = importlib.util.spec_from_file_location("audio_stats", os.path.join(here(), "audio-stats.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


AS = load_audio_stats()


def run_cmd(args, timeout=30):
    try:
        r = subprocess.run(args, capture_output=True, text=True, timeout=timeout,
                           creationflags=NO_WINDOW, errors="replace")
        return (r.stdout or "") + (r.stderr or "")
    except Exception as e:  # noqa: BLE001 - report anything, never stop the run
        return "(%s failed: %s)" % (args[0], e)


def com_init():
    """Windows audio (PortAudio WASAPI) needs COM set up in the thread that opens a stream;
    Python threads don't. Without it the open fails with 'Invalid device' (-9996) and the
    device then stays unusable until this program exits (found 2026-11-21)."""
    if sys.platform == "win32":
        import ctypes
        ctypes.windll.ole32.CoInitializeEx(None, 0)   # COINIT_MULTITHREADED


def capture_print(fn, *a):
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        try:
            fn(*a)
        except SystemExit as e:
            print("stopped: %s" % e)
    return buf.getvalue()


# ---------------------------------------------------------------- receive analysis

def db(x):
    return 20.0 * np.log10(max(float(x), 1e-9))


def analyse_rx(data):
    """data: int16 frames x 2. Returns (text lines, verdict, status)."""
    out = []
    chans = []
    for name, ch in (("L", data[:, 0]), ("R", data[:, 1])):
        x = ch.astype(np.float64)
        rms = float(np.sqrt(np.mean(x * x)))
        bits = int(np.bitwise_or.reduce(ch.view(np.uint16)))
        neg = float(np.mean(ch < 0) * 100.0)
        full = float(np.mean(np.abs(x) > 0.9 * FULL) * 100.0)
        uniq = len(np.unique(ch))
        chans.append(dict(rms=rms, bits=bits, neg=neg, full=full, uniq=uniq))
        out.append("%s  RMS %7.1f counts (%6.1f dBFS)  DC %7.1f  peak %6d  negative %5.1f %%  "
                   "near full scale %5.1f %%  different values %d  bits used 0x%04X"
                   % (name, rms, db(rms / FULL), float(np.mean(x)), int(np.max(np.abs(x))),
                      neg, full, uniq, bits))
    b = data.tobytes()[1:]
    b = b[: len(b) // 4 * 4]
    s = np.frombuffer(b, dtype="<i2").astype(np.float64)
    shift_db = db(np.sqrt(np.mean(s * s)) / FULL)
    rms_db = db(max(c["rms"] for c in chans) / FULL)
    out.append("Same bytes read one byte later: RMS %.1f dBFS (normal: about the same or louder)" % shift_db)

    l = data[:, 0].astype(np.float64)
    r = data[:, 1].astype(np.float64)
    corr = float(np.corrcoef(l, r)[0, 1]) if np.std(l) > 0 and np.std(r) > 0 else float("nan")
    ratio_db = db(chans[0]["rms"]) - db(chans[1]["rms"])
    out.append("I/Q: level L vs R %+.1f dB, correlation %.3f (normal: within a few dB, near 0)"
               % (ratio_db, corr))

    iq = l + 1j * r
    n = min(len(iq), 65536)
    spec = np.abs(np.fft.fftshift(np.fft.fft(iq[:n] * np.hanning(n)))) / n
    spec_db = 20 * np.log10(spec / FULL + 1e-12)
    floor = float(np.median(spec_db))
    pk = int(np.argmax(spec_db))
    out.append("Spectrum: floor %.1f dBFS, strongest line %.1f dBFS at %+.0f Hz"
               % (floor, spec_db[pk], (pk - n / 2) * RATE / n))
    # lines standing well above the floor (comb check)
    order = np.argsort(spec_db)[::-1]
    lines = []
    for i in order:
        if spec_db[i] < floor + 30 or len(lines) >= 8:
            break
        if all(abs(i - j) > 8 for j in lines):
            lines.append(int(i))
    if lines:
        out.append("Lines 30 dB+ over the floor: " + ", ".join(
            "%+.0f Hz (%.0f dB)" % ((i - n / 2) * RATE / n, spec_db[i] - floor) for i in sorted(lines)))

    uniq = min(c["uniq"] for c in chans)
    if max(c["rms"] for c in chans) == 0:
        return out, "all zero: no receive data at all (codec / I2S not delivering)", "FAIL"
    if min(c["rms"] for c in chans) == 0:
        return out, "one channel dead (I or Q is all zero)", "FAIL"
    if uniq <= 4:
        return out, "stuck data: a few repeating values (codec / I2S not delivering)", "FAIL"
    if all(c["neg"] < 1.0 for c in chans):
        return out, "no negative samples: receive one bit out of step (slip)", "FAIL"
    # normal data (noise around 0, half negative) sets every bit; the 2026-11-17 misframe
    # only ever set bits 15-14 and 5-0 (OR 0xC03F)
    if any(c["bits"] != 0xFFFF for c in chans):
        return out, "some bits never set (L 0x%04X, R 0x%04X): receive out of step (misframe)" % (
            chans[0]["bits"], chans[1]["bits"]), "FAIL"
    if shift_db < rms_db - 20:
        return out, "byte slip: samples read one byte off", "FAIL"
    if abs(ratio_db) > 6:
        return out, "I and Q levels differ by %.0f dB (one side weak)" % abs(ratio_db), "WARNING"
    if not np.isnan(corr) and abs(corr) > 0.9:
        return out, "I and Q nearly the same signal (correlation %.2f): no real I/Q" % corr, "WARNING"
    if max(c["full"] for c in chans) > 5 or rms_db > -6:
        return out, "huge input, clipping (overload)", "WARNING"
    return out, "looks like normal receive data", "OK"


# ---------------------------------------------------------------- the window

class App:
    def __init__(self, root):
        self.root = root
        root.title("Black Pill Diagnostics %s" % TOOL_VERSION)
        root.geometry("900x680")
        self.q = queue.Queue()
        self.summary = []          # (status, step, text)
        self.details = []          # free text for details.txt
        self.files = {}            # name in zip -> bytes
        self.dev = None
        self.keyed = False
        self.tx_stop = threading.Event()
        self.tx_stream = None
        self.busy = False
        self.stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")

        top = ttk.Frame(root, padding=8)
        top.pack(fill="x")
        ttk.Label(top, text="Close MSCC first. Then press Run checks. At the end a zip is saved "
                            "on your Desktop: send it back.", wraplength=860).pack(anchor="w")
        bar = ttk.Frame(root, padding=(8, 0))
        bar.pack(fill="x")
        self.run_btn = ttk.Button(bar, text="Run checks", command=self.start_checks)
        self.run_btn.pack(side="left")

        txf = ttk.LabelFrame(root, text="Transmit test (optional): dummy load only", padding=8)
        txf.pack(fill="x", padx=8, pady=6)
        ttk.Label(txf, text="Band").pack(side="left")
        self.band = ttk.Combobox(txf, values=[b[0] for b in BANDS], width=7, state="readonly")
        self.band.set("20 m")
        self.band.pack(side="left", padx=4)
        self.key_btn = ttk.Button(txf, text="Key now (5 s)", command=self.start_tx, state="disabled")
        self.key_btn.pack(side="left", padx=4)
        self.stop_btn = ttk.Button(txf, text="STOP", command=self.stop_tx, state="disabled")
        self.stop_btn.pack(side="left", padx=4)
        self.skip_btn = ttk.Button(txf, text="Skip / done: save the zip", command=self.start_finish,
                                   state="disabled")
        self.skip_btn.pack(side="left", padx=12)

        self.text = scrolledtext.ScrolledText(root, font=("Consolas", 9), wrap="word")
        self.text.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        for tag, col in (("OK", "#1a7f37"), ("WARNING", "#b35900"), ("FAIL", "#c00000"),
                         ("HEAD", "#000080")):
            self.text.tag_config(tag, foreground=col)
        self.text.tag_config("HEAD", font=("Consolas", 10, "bold"))
        root.protocol("WM_DELETE_WINDOW", self.on_close)
        root.after(100, self.pump)

    # ----- output
    def put(self, text, tag=None):
        self.q.put((text, tag))
        self.details.append(text)

    def head(self, text):
        self.put("\n== " + text, "HEAD")

    def result(self, status, step, text):
        self.summary.append((status, step, text))
        self.put("%-8s %s" % (status, text), status)

    def pump(self):
        try:
            while True:
                text, tag = self.q.get_nowait()
                self.text.insert("end", text + "\n", tag or ())
                self.text.see("end")
        except queue.Empty:
            pass
        self.root.after(100, self.pump)

    def ui(self, fn):
        self.root.after(0, fn)

    # ----- checks
    def start_checks(self):
        if self.busy:
            return
        self.busy = True
        self.run_btn.config(state="disabled")
        threading.Thread(target=self.checks, daemon=True).start()

    def checks(self):
        com_init()
        try:
            self.put("Black Pill Diagnostics %s, %s" % (TOOL_VERSION, time.strftime("%Y-%m-%d %H:%M:%S")))
            if not self.step_servers():
                self.ui(lambda: self.run_btn.config(state="normal"))
                return
            if self.step_find():
                self.step_state()
                self.step_codec()
            self.step_receive()
            self.head("5  Transmit test (optional)")
            self.put("Dummy load on the antenna socket, amplifier off or unplugged (the pill keys the "
                     "amplifier line on every transmit). Pick a band, press Key now, and watch the "
                     "scope / spectrum analyser. Expected: ONE carrier 12 kHz above or below the LO "
                     "(LO = the band's FT8 frequency), plus small leftovers at the LO and 12 kHz the "
                     "other side. A comb = the problem is in the pill / board, not MSCC. "
                     "Or press Skip to save the zip.")
            self.ui(self.tx_ready)
        except (Exception, SystemExit):  # noqa: BLE001
            self.put(traceback.format_exc(), "FAIL")
            self.ui(self.tx_ready)
        finally:
            self.busy = False

    def tx_ready(self):
        self.key_btn.config(state="normal" if self.dev is not None else "disabled")
        self.skip_btn.config(state="normal")

    def step_servers(self):
        self.head("0  MSCC closed?")
        out = run_cmd(["tasklist", "/FO", "CSV", "/NH"])
        running = sorted({line.split('","')[0].strip('"') for line in out.splitlines()
                          if line.strip('"').lower().startswith(SERVER_PREFIXES)})
        if running:
            self.result("FAIL", "0", "MSCC is running (%s). Close MSCC and the servers, then press "
                                     "Run checks again." % ", ".join(running))
            return False
        self.put("OK       no MSCC servers running", "OK")
        return True

    def step_find(self):
        self.head("1  Find the radio")
        try:
            d = AS.dev()
        except SystemExit as e:
            self.result("FAIL", "1", "Radio not found on USB: %s" % e)
            return False
        self.dev = d
        backend = type(d._ctx.backend).__module__
        driver = ("libusb-win32 (Zadig)" if "libusb0" in backend else
                  "WinUSB / libusb-1.0" if "libusb1" in backend else backend)
        try:
            product = d.product
        except Exception:  # noqa: BLE001
            product = "?"
        try:
            v = bytes(d.ctrl_transfer(0xC0, CMD_VERSION, 0xA55A, 0, 2, 500))
            minor, major = v[1], v[0]
            ver = "%d.%d" % (major, minor)
        except Exception as e:  # noqa: BLE001
            ver = "? (%s)" % e
            minor = 0
        self.put("USB 16c0:05dc  product \"%s\"  bcdDevice %04X  driver %s"
                 % (product, d.bcdDevice, driver))
        self.result("OK", "1", "Radio found: Black Pill firmware %s, driver %s" % (ver, driver))
        if 0 < minor < 200 and minor < 22:
            self.result("WARNING", "1", "Firmware %s is older than 3.22 (current release 3.24)" % ver)
        elif minor >= 200:
            self.result("WARNING", "1", "Version %s looks like a PSoC radio, not a Black Pill" % ver)
        self.fw_minor = minor
        return True

    def step_state(self):
        self.head("2  Pill state")
        d = self.dev
        self.put(capture_print(AS.once, d).rstrip())
        try:
            s = AS.read(d, False)
        except SystemExit as e:
            self.result("FAIL", "2", "Could not read the pill counters: %s" % e)
            return
        self.files["stats-start.txt"] = repr(s).encode()
        self.stats0 = s
        self.result("OK", "2", "Pill up %.1f min" % (s["ms"] / 60000.0))
        if "reset" in s:
            f = s["reset"] & 0xFF
            i2c = s["reset"] >> 16
            self.result("WARNING" if f & 0x60 else "OK", "2", "Last reset: %s" % AS.reset_text(f))
            failed = i2c & 0x300
            fixed = i2c & 0xC00
            if failed and (failed >> 2) != fixed:
                self.result("FAIL", "2", "Power-up: %s" % AS.i2c_retry_text(i2c))
            elif failed:
                self.result("WARNING", "2", "Power-up: %s" % AS.i2c_retry_text(i2c))
            else:
                self.result("OK", "2", "Power-up: codec + LO answered on I2C")
        h = AS.read_hang(d)
        if h is not None and h["flags"] & 1:
            self.result("WARNING", "2", "Hang record present (the pill reset itself after a hang)")
        if s.get("ep0_cut", 0):
            self.result("WARNING", "2", "ep0_cut %d (USB replies cut short)" % s["ep0_cut"])

    def step_codec(self):
        self.head("3  Codec + receive link")
        d = self.dev
        r0 = AS.read_resync(d)
        if r0 is None:
            self.result("WARNING", "3", "Pill older than 3.18: no codec / frame error counters")
            return
        st, rb = r0["codec"] & 0xFF, (r0["codec"] >> 8) & 0xFF
        st_t = {0: "not started", 1: "set up OK", 2: "I2C FAILED"}.get(st, "code %d" % st)
        rb_t = {0: "not read", 1: "matches", 2: "DIFFERENT from what was written",
                3: "could not be read"}.get(rb, "code %d" % rb)
        self.result("OK" if st == 1 else "FAIL", "3", "Codec setup: %s" % st_t)
        self.result("OK" if rb == 1 else "FAIL", "3", "Codec registers read back: %s" % rb_t)
        time.sleep(2.0)
        r1 = AS.read_resync(d)
        dfre = (r1["fre"] - r0["fre"]) & 0xFFFFFFFF
        self.put(capture_print(AS.print_resync, r1).rstrip())
        if dfre:
            self.result("FAIL", "3", "Receive frame errors coming now: %d in 2 s (I2S out of step)" % dfre)
        else:
            self.result("OK", "3", "No receive frame errors in 2 s")
        if r1["count"]:
            self.result("WARNING", "3", "Receive slip fixed by the pill %d time(s) since power-up" % r1["count"])

    def find_audio(self, want_input):
        try:
            apis = {h["name"]: i for i, h in enumerate(sd.query_hostapis())}
        except Exception:  # noqa: BLE001
            return None
        wasapi = apis.get("Windows WASAPI")
        for i, dv in enumerate(sd.query_devices()):
            if dv["hostapi"] != wasapi or "Multus" not in dv["name"]:
                continue
            if want_input and dv["max_input_channels"] >= 2:
                return i, dv["name"]
            if not want_input and dv["max_output_channels"] >= 2:
                return i, dv["name"]
        return None

    def step_receive(self):
        self.head("4  Receive: %.0f s of raw I/Q" % RX_SECS)
        found = self.find_audio(True)
        if not found:
            self.result("FAIL", "4", "Radio receive device (Multus ... Sound, WASAPI) not found in Windows")
            return
        idx, name = found
        self.put("Device %d: %s" % (idx, name))
        chunks, times = [], []

        def cb(indata, frames, t, status):
            times.append((time.perf_counter(), frames))
            chunks.append(indata.copy())

        mode = "exclusive"
        try:
            st = sd.InputStream(device=idx, samplerate=RATE, channels=2, dtype="int16", callback=cb,
                                extra_settings=sd.WasapiSettings(exclusive=True))
        except Exception as e:  # noqa: BLE001
            self.put("Exclusive mode refused (%s), using shared mode" % e)
            mode = "shared"
            try:
                st = sd.InputStream(device=idx, samplerate=RATE, channels=2, dtype="int16", callback=cb)
            except Exception as e2:  # noqa: BLE001
                self.result("FAIL", "4", "Could not open the receive device: %s" % e2)
                return
        with st:
            time.sleep(RX_SECS)
        if not chunks:
            self.result("FAIL", "4", "No receive audio arrived in %.0f s" % RX_SECS)
            return
        data = np.concatenate(chunks)
        # sample rate against the PC clock: line fit of frames received vs callback time,
        # skipping the first second (start-up), so callback jitter mostly cancels out
        tt = np.array([t for t, _ in times])
        ff = np.cumsum([f for _, f in times])
        keep = tt - tt[0] >= 1.0
        if keep.sum() > 50 and tt[-1] - tt[keep][0] > 5:
            rate = float(np.polyfit(tt[keep], ff[keep], 1)[0])
            ppm = (rate - EXPECTED_RATE) / EXPECTED_RATE * 1e6
            txt = "Receive sample rate %.0f per s (pill expected %.0f, %+.0f ppm, %s mode)" % (
                rate, EXPECTED_RATE, ppm, mode)
            if mode == "shared":
                self.result("WARNING", "4", txt + ": shared mode, Windows may resample, not exact")
            elif abs(ppm) > 10000:
                self.result("FAIL", "4", txt + ": far off, I2S / codec clock wrong")
            elif abs(ppm) > 2000:
                self.result("WARNING", "4", txt)
            else:
                self.result("OK", "4", txt)
        buf = io.BytesIO()
        with wave.open(buf, "wb") as w:
            w.setnchannels(2)
            w.setsampwidth(2)
            w.setframerate(RATE)
            w.writeframes(data.tobytes())
        self.files["rx-capture.wav"] = buf.getvalue()
        lines, verdict, status = analyse_rx(data[RATE // 2:] if len(data) > RATE else data)
        for ln in lines:
            self.put(ln)
        self.result(status, "4", "Receive data: %s" % verdict)

    # ----- transmit test
    def start_tx(self):
        if self.dev is None or self.keyed:
            return
        band = self.band.get()
        lo = dict(BANDS)[band]
        if not messagebox.askyesno("Transmit test", "Dummy load connected to the antenna socket?\n\n"
                                   "Amplifier OFF or unplugged? (the amplifier line is keyed too)\n\n"
                                   "The radio will transmit for %.0f s on %s (LO %.3f MHz) at low drive."
                                   % (TX_SECS, band, lo / 1e6), icon="warning"):
            return
        self.key_btn.config(state="disabled")
        self.skip_btn.config(state="disabled")
        self.stop_btn.config(state="normal")
        self.tx_stop.clear()
        threading.Thread(target=self.tx_run, args=(band, lo), daemon=True).start()

    def stop_tx(self):
        self.tx_stop.set()

    def unkey(self):
        """Unkey the pill and stop the tone. Safe to call any time, any number of times."""
        if self.dev is not None:
            for _ in range(3):
                try:
                    self.dev.ctrl_transfer(0xC0, CMD_PTT, 0, 0, 1, 500)
                    break
                except Exception:  # noqa: BLE001
                    time.sleep(0.05)
        self.keyed = False
        time.sleep(0.1)  # pill ramps down 5 ms, PA off after
        st, self.tx_stream = self.tx_stream, None
        if st is not None:
            try:
                st.stop()
                st.close()
            except Exception:  # noqa: BLE001
                pass

    def tx_run(self, band, lo):
        com_init()
        self.head("5  Transmit test %s, LO %.3f MHz" % (band, lo / 1e6))
        d = self.dev
        try:
            found = self.find_audio(False)
            if not found:
                self.result("FAIL", "5", "Radio transmit device (Multus ... Sound, WASAPI) not found")
                return
            idx, name = found
            self.put("Device %d: %s" % (idx, name))
            d.ctrl_transfer(0x40, CMD_MODE, 0, 0, b"U", 500)
            d.ctrl_transfer(0x40, CMD_SET_FREQ, 0x0700, 0, struct.pack("<I", lo), 500)
            time.sleep(0.3)   # band relays
            n = np.arange(8)
            amp = TONE_LEVEL * FULL
            block = np.stack([amp * np.cos(2 * np.pi * n / 8), amp * np.sin(2 * np.pi * n / 8)],
                             axis=1).round().astype(np.int16)
            pos = [0]
            underruns = [0]

            def cb(outdata, frames, t, status):
                if status.output_underflow:
                    underruns[0] += 1
                k = (np.arange(frames) + pos[0]) % 8
                outdata[:] = block[k]
                pos[0] = (pos[0] + frames) % 8

            mode = "exclusive"
            try:
                self.tx_stream = sd.OutputStream(device=idx, samplerate=RATE, channels=2, dtype="int16",
                                                 callback=cb, extra_settings=sd.WasapiSettings(exclusive=True))
            except Exception as e:  # noqa: BLE001
                self.put("Exclusive mode refused (%s), using shared mode (Windows volume applies)" % e)
                mode = "shared"
                self.tx_stream = sd.OutputStream(device=idx, samplerate=RATE, channels=2, dtype="int16",
                                                 callback=cb)
            self.tx_stream.start()
            time.sleep(0.3)   # tone flowing before the key
            r = bytes(d.ctrl_transfer(0xC0, CMD_PTT, 1, 0, 1, 500))
            self.keyed = True
            self.put("KEYED (%s mode, tone %d Hz at %.0f %% of full scale). Watch the analyser..."
                     % (mode, TONE_HZ, TONE_LEVEL * 100), "WARNING")
            s0 = AS.read(d, False)
            t_end = time.time() + TX_SECS
            while time.time() < t_end and not self.tx_stop.is_set():
                time.sleep(0.05)
            s1 = AS.read(d, False)
            self.unkey()
            stopped = " (stopped early)" if self.tx_stop.is_set() else ""
            under = (s1["out_under"] - s0["out_under"]) & 0xFFFFFFFF
            drop = (s1["out_drop"] - s0["out_drop"]) & 0xFFFFFFFF
            self.put("Unkeyed%s. Pill transmit underruns %d, drops %d while keyed; PC underruns %d; "
                     "key reply %s" % (stopped, under, drop, underruns[0], r.hex()))
            self.result("WARNING" if under > 2 or drop else "OK", "5",
                        "Transmit test %s done%s: pill underruns %d, drops %d (what the analyser "
                        "showed: please write it in the email)" % (band, stopped, under, drop))
        except (Exception, SystemExit) as e:  # noqa: BLE001
            self.result("FAIL", "5", "Transmit test failed: %s" % e)
            self.put(traceback.format_exc())
        finally:
            self.unkey()
            self.ui(self.tx_done)

    def tx_done(self):
        self.stop_btn.config(state="disabled")
        self.key_btn.config(state="normal")
        self.skip_btn.config(state="normal")

    # ----- setup + zip
    def start_finish(self):
        self.key_btn.config(state="disabled")
        self.skip_btn.config(state="disabled")
        threading.Thread(target=self.finish, daemon=True).start()

    def step_setup(self):
        self.head("6  Setup")
        base = os.path.join(os.environ.get("LOCALAPPDATA", ""), "MSCC-NET9")
        n = 0
        if os.path.isdir(base):
            for p in glob.glob(os.path.join(base, "*.ini")) + glob.glob(os.path.join(base, "cal", "*")):
                if os.path.isfile(p):
                    with open(p, "rb") as f:
                        self.files["mscc/" + os.path.relpath(p, base).replace("\\", "/")] = f.read()
                    n += 1
            logs = [p for p in glob.glob(os.path.join(base, "logs", "*")) if os.path.isfile(p)]
            for p in sorted(logs, key=os.path.getmtime)[-20:]:
                with open(p, "rb") as f:
                    f.seek(max(0, os.path.getsize(p) - 2_000_000))
                    self.files["mscc/logs/" + os.path.basename(p)] = f.read()
                n += 1
            self.result("OK", "6", "MSCC settings + logs collected: %d files from %s" % (n, base))
        else:
            self.result("WARNING", "6", "No MSCC settings folder %s" % base)
        vers = []
        for folder in (r"C:\mscc-net9", os.path.join(os.environ.get("ProgramFiles", ""), "MSCC")):
            for p in sorted(glob.glob(os.path.join(folder, "*.exe"))):
                ps = "(Get-Item -LiteralPath '%s').VersionInfo | ForEach-Object { $_.FileVersion + ' | ' + $_.ProductVersion }" % p
                v = run_cmd(["powershell", "-NoProfile", "-Command", ps], 15).strip()
                st = os.stat(p)
                vers.append("%-40s %-30s %10d  %s" % (os.path.basename(p), v, st.st_size,
                                                      time.strftime("%Y-%m-%d %H:%M", time.localtime(st.st_mtime))))
        self.files["mscc-programs.txt"] = ("\n".join(vers) or "no MSCC folder found").encode()
        self.put("MSCC programs: %d .exe files listed" % len(vers))
        pnp = run_cmd(["pnputil", "/enum-devices", "/connected", "/drivers"], 60)
        blocks = [b for b in pnp.split("\n\n") if "VID_16C0" in b.upper() or "MULTUS" in b.upper()]
        self.files["usb-driver.txt"] = ("\n\n".join(blocks) or pnp[-20000:]).encode(errors="replace")
        self.put("USB driver entries for the radio: %d" % len(blocks))
        try:
            devs = str(sd.query_devices())
        except Exception as e:  # noqa: BLE001
            devs = str(e)
        self.files["audio-devices.txt"] = devs.encode(errors="replace")
        self.files["windows.txt"] = ("%s\n%s\n" % (platform.platform(), platform.version())).encode()
        self.put("Windows: %s" % platform.platform())

    def finish(self):
        try:
            self.step_setup()
            if self.dev is not None:
                self.files["stats-end.txt"] = capture_print(AS.once, self.dev).encode()
            self.head("7  Save")
            desk = os.path.join(os.environ.get("USERPROFILE", "."), "Desktop")
            if not os.path.isdir(desk):
                desk = os.environ.get("USERPROFILE", ".")
            path = os.path.join(desk, "pill-diag-%s.zip" % self.stamp)
            summ = ["Black Pill Diagnostics %s  %s" % (TOOL_VERSION, time.strftime("%Y-%m-%d %H:%M:%S")),
                    "", "FAIL / WARNING first:", ""]
            for want in ("FAIL", "WARNING", "OK"):
                for st, step, txt in self.summary:
                    if st == want:
                        summ.append("%-8s step %s  %s" % (st, step, txt))
            self.files["summary.txt"] = ("\r\n".join(summ) + "\r\n").encode()
            self.files["details.txt"] = ("\r\n".join(self.details) + "\r\n").encode(errors="replace")
            with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
                for name in ["summary.txt"] + sorted(k for k in self.files if k != "summary.txt"):
                    z.writestr(name, self.files[name])
            self.put("Saved %s  (%d KB). Please send this file back." % (path, os.path.getsize(path) // 1024), "OK")
            self.ui(lambda: messagebox.showinfo("Done", "Saved on your Desktop:\n%s\n\nPlease send it back." % path))
        except Exception:  # noqa: BLE001
            self.put(traceback.format_exc(), "FAIL")
            self.ui(lambda: self.skip_btn.config(state="normal"))

    def on_close(self):
        self.tx_stop.set()
        self.unkey()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = App(root)
    try:
        root.mainloop()
    finally:
        app.unkey()


if __name__ == "__main__":
    main()
