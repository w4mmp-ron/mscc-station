"""Serial ports and PortAudio device listing."""

from __future__ import annotations

import os
import re
from contextlib import contextmanager
from dataclasses import dataclass
from typing import List, Optional, Tuple


@dataclass
class SerialChoice:
    path: str
    label: str


@dataclass
class AudioDevice:
    index: int
    name: str
    host_api: str
    channels: int
    is_input: bool
    hint: str = ""


def list_serial_ports() -> List[SerialChoice]:
    choices = [
        SerialChoice("PTY", "Kenwood CAT text only (no hardware PTT pins)"),
    ]
    try:
        names = sorted(os.listdir("/dev"))
    except OSError:
        return choices
    for n in names:
        if not (
            n.startswith("ttyUSB")
            or n.startswith("ttyACM")
            or n.startswith("ttyS")
            or n.startswith("tnt")
        ):
            continue
        path = f"/dev/{n}"
        if n.startswith("tnt"):
            hint = "tty0tty — good for CAT + RTS/CTS PTT"
        elif n.startswith("ttyUSB"):
            hint = "USB serial"
        elif n.startswith("ttyACM"):
            hint = "USB ACM serial"
        else:
            hint = "serial device"
        choices.append(SerialChoice(path, hint))
    return choices


def is_digi_only_device(name: str) -> bool:
    if not name:
        return True
    if "VirtualA" in name or "VirtualB" in name:
        return True
    if ".monitor" in name or "Monitor of" in name:
        return True
    if "MSCC_Cable" in name or "MSCCLoop" in name or "MSCC_Digi" in name:
        return True
    return False


def device_hint(name: str) -> str:
    if not name:
        return ""
    if "Multus" in name or "Proficio" in name:
        return "radio I/Q — usually NOT operator phones"
    if re.search(r"audioinjector|AudioInjector|wm8731", name, re.I):
        return "typical operator sound card"
    if "hdmi" in name.lower() or "HDMI" in name:
        return "HDMI — rarely useful for radio"
    if "Loopback" in name or "loopback" in name or "aloop" in name:
        return "ALSA loopback — usually not operator"
    return ""


@contextmanager
def _quiet_alsa_stderr():
    """Hide ALSA probe spam during PortAudio init/enumerate."""
    try:
        devnull = open(os.devnull, "w")
        old_err = os.dup(2)
        os.dup2(devnull.fileno(), 2)
        try:
            yield
        finally:
            os.dup2(old_err, 2)
            os.close(old_err)
            devnull.close()
    except Exception:
        yield


def _supports_96k(pa, index: int, is_input: bool, channels: int) -> bool:
    """True if PortAudio reports 96 kHz support (int16)."""
    import pyaudio

    ch = min(max(channels, 1), 2)
    try:
        if is_input:
            return bool(
                pa.is_format_supported(
                    rate=96000,
                    input_device=index,
                    input_channels=ch,
                    input_format=pyaudio.paInt16,
                )
            )
        return bool(
            pa.is_format_supported(
                rate=96000,
                output_device=index,
                output_channels=ch,
                output_format=pyaudio.paInt16,
            )
        )
    except ValueError:
        # PyAudio raises ValueError when not supported
        return False
    except Exception:
        return False


def list_audio_devices(
    want_input: bool,
    *,
    require_96k: bool = False,
) -> Tuple[List[AudioDevice], Optional[str]]:
    """
    List operator-oriented PortAudio devices.
    require_96k=False (default): list all usable operator devices; sdrcore
    resamples when the device is not 96 kHz.
    Returns (devices, error_message).
    """
    try:
        import pyaudio
    except ImportError:
        return [], (
            "python3-pyaudio is not installed.\n"
            "Install:  sudo apt install -y python3-pyaudio portaudio19-dev"
        )

    devices: List[AudioDevice] = []
    err: Optional[str] = None
    with _quiet_alsa_stderr():
        pa = pyaudio.PyAudio()
        try:
            n = pa.get_device_count()
            for i in range(n):
                try:
                    info = pa.get_device_info_by_index(i)
                except Exception:
                    continue
                name = str(info.get("name") or "")
                if is_digi_only_device(name):
                    continue
                max_in = int(info.get("maxInputChannels") or 0)
                max_out = int(info.get("maxOutputChannels") or 0)
                if want_input and max_in < 1:
                    continue
                if not want_input and max_out < 1:
                    continue
                ch = max_in if want_input else max_out
                if require_96k and not _supports_96k(pa, i, want_input, ch):
                    continue
                try:
                    api = pa.get_host_api_info_by_index(int(info["hostApi"]))
                    api_name = str(api.get("name") or "?")
                except Exception:
                    api_name = "?"
                devices.append(
                    AudioDevice(
                        index=i,
                        name=name,
                        host_api=api_name,
                        channels=ch,
                        is_input=want_input,
                        hint=device_hint(name),
                    )
                )
        finally:
            pa.terminate()

    return devices, err


def read_multus_serial() -> Tuple[str, str]:
    """
    Try to read Proficio/Multus USB serial (VID 16c0 PID 05dc).
    Returns (serial_or_UNKNOWN, status_message).
    """
    vid, pid = 0x16C0, 0x05DC
    try:
        import usb.core
        import usb.util
    except ImportError:
        # Fall back to lsusb -v is heavy; try sysfs
        return _serial_from_sysfs(vid, pid)

    try:
        dev = usb.core.find(idVendor=vid, idProduct=pid)
        if dev is None:
            return "UNKNOWN", "No Multus/Proficio USB control device found (OK to continue)."
        try:
            # pyusb string descriptor
            if dev.iSerialNumber:
                sn = usb.util.get_string(dev, dev.iSerialNumber)
                if sn:
                    return sn.strip(), f"Found transceiver serial: {sn.strip()}"
        except Exception as e:
            return "UNKNOWN", f"USB device found but serial read failed: {e}"
        return "UNKNOWN", "Transceiver found but no USB serial string."
    except Exception as e:
        return "UNKNOWN", f"USB probe failed: {e}"


def _serial_from_sysfs(vid: int, pid: int) -> Tuple[str, str]:
    """Best-effort serial from /sys without pyusb."""
    base = "/sys/bus/usb/devices"
    try:
        for entry in os.listdir(base):
            d = os.path.join(base, entry)
            try:
                with open(os.path.join(d, "idVendor"), encoding="utf-8") as f:
                    v = int(f.read().strip(), 16)
                with open(os.path.join(d, "idProduct"), encoding="utf-8") as f:
                    p = int(f.read().strip(), 16)
            except (OSError, ValueError):
                continue
            if v != vid or p != pid:
                continue
            try:
                with open(os.path.join(d, "serial"), encoding="utf-8") as f:
                    sn = f.read().strip()
                if sn:
                    return sn, f"Found transceiver serial: {sn}"
            except OSError:
                return "UNKNOWN", "Transceiver found (sysfs) but no serial."
    except OSError:
        pass
    return "UNKNOWN", "No Multus/Proficio USB control device found (OK to continue)."
