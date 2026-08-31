#!/usr/bin/env python3
"""
Proficio STM32F411 — Windows USB vendor test jig (bare Black Pill OK).

Talks to VID 0x16C0 / PID 0x05DC the same way ms-sdr does (vendor device
control transfers). Does not require PCM3060 / SI5351 / daughter board.

Requires: pip install -r requirements.txt
          WinUSB or libusb-win32 on the Proficio interface (Zadig).
"""

from __future__ import annotations

import struct
import sys
import tkinter as tk
from datetime import datetime
from tkinter import messagebox, ttk
from typing import Optional

try:
    import usb.core
    import usb.util
except ImportError:
    print("Install deps:  pip install -r requirements.txt", file=sys.stderr)
    raise SystemExit(1)

VID = 0x16C0
PID = 0x05DC

# Match ms-sdr conventions
WVALUE_IN = 0xA55A
WVALUE_OUT = 0x071B  # 0x0700 + 27

CMD_GET_VERSION = 0x00
CMD_GET_PIN = 0x02
CMD_GET_FREQ = 0x3A
CMD_GET_STARTUP = 0x3C
CMD_GET_KEY_DOWN = 0xA4
CMD_GET_PTT = 0xA5
CMD_GET_TRANSCEIVER_TEMP = 0xBF
CMD_GET_PPM_INT = 0x94
CMD_GET_PPM_DEC = 0x95
CMD_SET_FREQ = 0x32
CMD_SET_USRP1 = 0x50
CMD_ENTER_BOOTLOADER = 0xFE

TIMEOUT_MS = 1000


def _ts() -> str:
    return datetime.now().strftime("%H:%M:%S")


class ProficioUsb:
    def __init__(self) -> None:
        self.dev: Optional[usb.core.Device] = None

    def open(self) -> bool:
        self.close()
        self.dev = usb.core.find(idVendor=VID, idProduct=PID)
        if self.dev is None:
            return False
        try:
            if self.dev.is_kernel_driver_active(0):
                self.dev.detach_kernel_driver(0)
        except (NotImplementedError, usb.core.USBError):
            pass
        try:
            self.dev.set_configuration()
        except usb.core.USBError:
            pass
        return True

    def close(self) -> None:
        if self.dev is not None:
            try:
                usb.util.dispose_resources(self.dev)
            except Exception:
                pass
        self.dev = None

    def present(self) -> bool:
        return usb.core.find(idVendor=VID, idProduct=PID) is not None

    def ctrl_in(self, cmd: int, length: int, wvalue: int = WVALUE_IN, windex: int = 0) -> bytes:
        if self.dev is None and not self.open():
            raise RuntimeError("No device")
        assert self.dev is not None
        data = self.dev.ctrl_transfer(
            0xC0,  # IN | VENDOR | DEVICE
            cmd,
            wvalue,
            windex,
            length,
            timeout=TIMEOUT_MS,
        )
        return bytes(data)

    def ctrl_out(self, cmd: int, payload: bytes = b"", wvalue: int = WVALUE_OUT, windex: int = 0) -> int:
        if self.dev is None and not self.open():
            raise RuntimeError("No device")
        assert self.dev is not None
        n = self.dev.ctrl_transfer(
            0x40,  # OUT | VENDOR | DEVICE
            cmd,
            wvalue,
            windex,
            payload if payload else None,
            timeout=TIMEOUT_MS,
        )
        return int(n)


class TestJigApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Proficio STM32 — USB Test Jig")
        self.minsize(560, 480)
        self.geometry("620x520")
        self.usb = ProficioUsb()
        self._poll_id: Optional[str] = None
        self._build()
        self._poll()
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build(self) -> None:
        top = ttk.Frame(self, padding=8)
        top.pack(fill=tk.X)
        self.status = tk.StringVar(value="Disconnected")
        ttk.Label(top, textvariable=self.status, width=18).pack(side=tk.LEFT)
        ttk.Label(top, text=f"VID {VID:04X}  PID {PID:04X}").pack(side=tk.LEFT, padx=12)
        ttk.Button(top, text="Refresh", command=self._refresh).pack(side=tk.RIGHT)

        body = ttk.LabelFrame(self, text="Bare-board tests (no daughter required)", padding=8)
        body.pack(fill=tk.X, padx=8, pady=4)

        row1 = ttk.Frame(body)
        row1.pack(fill=tk.X, pady=2)
        ttk.Button(row1, text="Get Version", command=self._get_version).pack(side=tk.LEFT, padx=2)
        ttk.Button(row1, text="Get Temp (0xBF)", command=self._get_temp).pack(side=tk.LEFT, padx=2)
        ttk.Button(row1, text="Get Freq", command=self._get_freq).pack(side=tk.LEFT, padx=2)
        ttk.Button(row1, text="Get Startup", command=self._get_startup).pack(side=tk.LEFT, padx=2)

        row2 = ttk.Frame(body)
        row2.pack(fill=tk.X, pady=2)
        ttk.Button(row2, text="Get Pin / Key reg", command=self._get_pin).pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="Get PTT", command=self._get_ptt).pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="Get Key Down", command=self._get_key).pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="Get PPM int/dec", command=self._get_ppm).pack(side=tk.LEFT, padx=2)

        row3 = ttk.Frame(body)
        row3.pack(fill=tk.X, pady=2)
        ttk.Label(row3, text="Set Freq (Hz)").pack(side=tk.LEFT)
        self.freq_var = tk.StringVar(value="14074000")
        ttk.Entry(row3, textvariable=self.freq_var, width=12).pack(side=tk.LEFT, padx=4)
        ttk.Button(row3, text="Send SET_FREQ", command=self._set_freq).pack(side=tk.LEFT, padx=2)
        ttk.Label(row3, text="(accepted even if SI5351 absent)", foreground="#666").pack(side=tk.LEFT)

        row4 = ttk.Frame(body)
        row4.pack(fill=tk.X, pady=2)
        ttk.Button(row4, text="TX Request ON", command=lambda: self._usrp(True)).pack(side=tk.LEFT, padx=2)
        ttk.Button(row4, text="TX Request OFF", command=lambda: self._usrp(False)).pack(side=tk.LEFT, padx=2)
        ttk.Button(row4, text="Enter ROM Bootloader (0xFE)", command=self._enter_dfu).pack(
            side=tk.RIGHT, padx=2
        )

        logf = ttk.LabelFrame(self, text="Log", padding=4)
        logf.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
        self.log = tk.Text(logf, height=18, wrap=tk.WORD, font=("Consolas", 10), state=tk.DISABLED)
        sb = ttk.Scrollbar(logf, command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb.pack(side=tk.RIGHT, fill=tk.Y)

        ttk.Label(
            self,
            text="Tip: If device not found, use Zadig → WinUSB on Proficio (16c0:05dc). Close ms-sdr first.",
            foreground="#333",
        ).pack(anchor=tk.W, padx=10, pady=(0, 6))

        self._log("Ready. Flash Black Pill firmware, plug USB, click Refresh.")

    def _log(self, msg: str) -> None:
        self.log.configure(state=tk.NORMAL)
        self.log.insert(tk.END, f"{_ts()}  {msg}\n")
        self.log.see(tk.END)
        self.log.configure(state=tk.DISABLED)

    def _refresh(self) -> None:
        ok = self.usb.open()
        self.status.set("Connected" if ok else "Disconnected")
        if ok:
            self._log("Opened 16c0:05dc")
        else:
            self._log("No Proficio USB device (16c0:05dc)")

    def _poll(self) -> None:
        connected = self.usb.present()
        self.status.set("Connected" if connected else "Disconnected")
        if not connected:
            self.usb.close()
        self._poll_id = self.after(1500, self._poll)

    def _on_close(self) -> None:
        if self._poll_id:
            try:
                self.after_cancel(self._poll_id)
            except Exception:
                pass
        self.usb.close()
        self.destroy()

    def _run(self, title: str, fn) -> None:
        try:
            fn()
        except usb.core.USBError as e:
            self._log(f"{title}: USB error {e}")
            self.usb.close()
        except Exception as e:
            self._log(f"{title}: {e}")

    def _get_version(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_VERSION, 2)
            if len(raw) >= 2:
                ver = struct.unpack_from("<H", raw)[0]
                major = ver & 0xFF
                minor = (ver >> 8) & 0xFF
                self._log(f"GET_VERSION: 0x{ver:04X}  → major={major} minor={minor}  raw={raw.hex()}")
            else:
                self._log(f"GET_VERSION: short reply {raw.hex()}")

        self._run("GET_VERSION", go)

    def _get_temp(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_TRANSCEIVER_TEMP, 4)
            if len(raw) >= 4:
                # Firmware sends swap32 of int32 °C-ish value
                be = struct.unpack_from(">i", raw)[0]
                le = struct.unpack_from("<i", raw)[0]
                self._log(f"GET_TEMP: raw={raw.hex()}  as_be_i32={be}  as_le_i32={le}")
            else:
                self._log(f"GET_TEMP: short {raw.hex()}")

        self._run("GET_TEMP", go)

    def _get_freq(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_FREQ, 4, wvalue=0)
            if len(raw) >= 4:
                hz = struct.unpack_from("<I", raw)[0]
                self._log(f"GET_FREQ: {hz} Hz  raw={raw.hex()}")
            else:
                self._log(f"GET_FREQ: {raw.hex()}")

        self._run("GET_FREQ", go)

    def _get_startup(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_STARTUP, 4, wvalue=0)
            self._log(f"GET_STARTUP: raw={raw.hex()}")

        self._run("GET_STARTUP", go)

    def _get_pin(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_PIN, 1)
            val = raw[0] if raw else -1
            self._log(f"GET_PIN / key reg: 0x{val:02X}  raw={raw.hex()}")

        self._run("GET_PIN", go)

    def _get_ptt(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_PTT, 1, wvalue=0)
            self._log(f"GET_PTT: {raw[0] if raw else '?'}  raw={raw.hex()}")

        self._run("GET_PTT", go)

    def _get_key(self) -> None:
        def go() -> None:
            raw = self.usb.ctrl_in(CMD_GET_KEY_DOWN, 1, wvalue=0)
            self._log(f"GET_KEY_DOWN: {raw[0] if raw else '?'}  raw={raw.hex()}")

        self._run("GET_KEY", go)

    def _get_ppm(self) -> None:
        def go() -> None:
            i = self.usb.ctrl_in(CMD_GET_PPM_INT, 2)
            d = self.usb.ctrl_in(CMD_GET_PPM_DEC, 2)
            self._log(f"PPM_INT raw={i.hex()}  PPM_DEC raw={d.hex()}")

        self._run("GET_PPM", go)

    def _set_freq(self) -> None:
        def go() -> None:
            hz = int(self.freq_var.get().strip())
            payload = struct.pack("<I", hz)
            n = self.usb.ctrl_out(CMD_SET_FREQ, payload)
            self._log(f"SET_FREQ {hz} Hz → wrote {n} bytes (SI5351 may be absent — OK on bare board)")
            # Read back host-side state in MCU
            raw = self.usb.ctrl_in(CMD_GET_FREQ, 4, wvalue=0)
            if len(raw) >= 4:
                back = struct.unpack_from("<I", raw)[0]
                self._log(f"  readback GET_FREQ: {back} Hz")

        self._run("SET_FREQ", go)

    def _usrp(self, on: bool) -> None:
        def go() -> None:
            # SET_USRP1 is handled as D2H with wValue bit0 in firmware
            raw = self.usb.ctrl_in(CMD_SET_USRP1, 1, wvalue=1 if on else 0)
            self._log(f"SET_USRP1 tx={'ON' if on else 'OFF'}  reply={raw.hex()}")

        self._run("USRP1", go)

    def _enter_dfu(self) -> None:
        if not messagebox.askyesno(
            "Enter ROM Bootloader",
            "Send CMD 0xFE — board will leave app mode for STM32CubeProgrammer / DFU.\nContinue?",
        ):
            return

        def go() -> None:
            try:
                self.usb.ctrl_out(CMD_ENTER_BOOTLOADER, b"\x00\x00\x00\x00")
                self._log("ENTER_BOOTLOADER (0xFE) sent — device should re-enum as STM BOOTLOADER")
            except usb.core.USBError as e:
                # Pipe/disconnect mid-transfer is often normal
                self._log(f"ENTER_BOOTLOADER: transfer ended ({e}) — often OK if device reset")
            self.usb.close()
            self.status.set("Disconnected")

        self._run("DFU", go)


def main() -> int:
    app = TestJigApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
