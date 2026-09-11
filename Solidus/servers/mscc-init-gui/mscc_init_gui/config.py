"""Write MSCC config files under $HOME/.local/mscc (same as CLI mscc-init)."""

from __future__ import annotations

import os
import socket
from pathlib import Path
from typing import List, Tuple

MSCC_DIGI_SPEAKER = "VirtualA"
MSCC_DIGI_MIC = "VirtualB.monitor"

MSCC_PORT = 8889
MS_SDR_PORT = 8888
PCB_VERSION = 10


def config_dir() -> Path:
    home = os.environ.get("HOME") or str(Path.home())
    return Path(home) / ".local" / "mscc"


def ensure_config_dir() -> Path:
    d = config_dir()
    d.mkdir(parents=True, exist_ok=True)
    return d


def _write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def write_mscc_ini(
    serial: str,
    host: str | None = None,
    *,
    proficio_mkii: bool = True,
) -> Path:
    """Write mscc.ini including PROFICIO-MKII (ms-sdr PTT sense thread gate)."""
    d = ensure_config_dir()
    if not host:
        host = socket.gethostname() or "127.0.0.1"
    serial = serial.strip() or "UNKNOWN"
    mkii = 1 if proficio_mkii else 0
    path = d / "mscc.ini"
    _write(
        path,
        (
            f"PROFICIO_SERIAL_NUMBER={serial};\n"
            f"MSCC_PORT={MSCC_PORT};\n"
            f"MSCC_IP={host};\n"
            f"PROFICIO_DLL_PORT={MS_SDR_PORT};\n"
            f"PROFICIO_DLL_IP={host};\n"
            f"PCB_VERSION={PCB_VERSION};\n"
            f"PROFICIO-MKII={mkii};\n"
        ),
    )
    return path


def write_i2c_ini(mfc: int = 0, meter: int = 0) -> Path:
    d = ensure_config_dir()
    path = d / "i2c.ini"
    _write(
        path,
        (
            "G_MASTER_CONTROLLER_attached=2;\n"
            f"G_MFC_attached={mfc};\n"
            "G_SOLIDUS_TEMP_SENSOR_attached=0;\n"
            f"G_METER_attached={meter};\n"
            "G_IQBD_attached=0;\n"
            "G_CURRENT_SENSOR_attached=0;\n"
        ),
    )
    return path


def write_cw_ini(keyer_installed: bool) -> Path:
    d = ensure_config_dir()
    path = d / "cw.ini"
    k = 1 if keyer_installed else 0
    _write(
        path,
        (
            f"CW_Keyer_Installed={k};\n"
            "CW_Keyer_Mode=0;\n"
            "CW_Iambic_Type=0;\n"
            "CW_Iambic_Calibrate=120;\n"
            "CW_Memory=0;\n"
            "CW_Spacing=0;\n"
            "CW_Paddle=0;\n"
            "CW_Weight=50;\n"
            "CW_Tx_Hold=15;\n"
            "CW_Speed=18;\n"
            "CW_Semi_Break_In=0;\n"
            "CW_Semi_Control=0;\n"
            "CW_Side_Tone_Volume=0;\n"
        ),
    )
    return path


def is_pty_name(port_name: str) -> bool:
    if not port_name or not port_name.strip():
        return True
    p = port_name.strip()
    if p == "0":
        return True
    return p.upper() == "PTY"


def write_comm_port_ini(port_name: str, pin: int) -> Path:
    d = ensure_config_dir()
    pin = max(0, min(2, int(pin)))
    if is_pty_name(port_name):
        name = "PTY"
        pin = 0
    else:
        name = port_name.strip()
    path = d / "comm-port.ini"
    _write(
        path,
        (
            f"COMM_PORT_NAME={name},COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,"
            f"PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN={pin};\n"
        ),
    )
    return path


def _device_name_for_ini(devname: str) -> str:
    """Match CLI: strip PortAudio '(...)' suffix noise."""
    name = devname.split("(", 1)[0].strip()
    return name or devname.strip()


def write_device_name(filename: str, devname: str) -> Path:
    d = ensure_config_dir()
    path = d / filename
    _write(path, _device_name_for_ini(devname))
    return path


def write_fixed_digital() -> Tuple[Path, Path]:
    sp = write_device_name("digital-speaker.ini", MSCC_DIGI_SPEAKER)
    mic = write_device_name("digital-microphone.ini", MSCC_DIGI_MIC)
    return sp, mic


def write_operator_audio(speaker: str, microphone: str) -> Tuple[Path, Path]:
    sp = write_device_name("operator-speaker.ini", speaker)
    mic = write_device_name("operator-microphone.ini", microphone)
    return sp, mic


def apply_all(
    *,
    serial: str,
    keyer_installed: bool,
    cat_port: str,
    cat_pin: int,
    operator_speaker: str,
    operator_mic: str,
    proficio_mkii: bool = True,
) -> List[str]:
    """Write full set of init files. Returns human-readable summary lines."""
    from .volume import set_operator_levels_100

    lines: List[str] = []
    ensure_config_dir()
    p = write_mscc_ini(serial, proficio_mkii=proficio_mkii)
    lines.append(
        f"mscc.ini → {p}  serial={serial or 'UNKNOWN'}  "
        f"PROFICIO-MKII={'1' if proficio_mkii else '0'}"
    )
    p = write_i2c_ini()
    lines.append(f"i2c.ini → {p}")
    p = write_cw_ini(keyer_installed)
    lines.append(f"cw.ini → {p}  keyer={'yes' if keyer_installed else 'no'}")
    p = write_comm_port_ini(cat_port, cat_pin)
    lines.append(f"comm-port.ini → {p}  CAT={cat_port} PIN={cat_pin}")
    sp, mic = write_fixed_digital()
    lines.append(f"digital-speaker.ini → {sp}  ({MSCC_DIGI_SPEAKER})")
    lines.append(f"digital-microphone.ini → {mic}  ({MSCC_DIGI_MIC})")
    osp, omic = write_operator_audio(operator_speaker, operator_mic)
    lines.append(f"operator-speaker.ini → {osp}")
    lines.append(f"operator-microphone.ini → {omic}")
    # Hardware path full open once; MSCC client controls AF gain.
    lines.append("Operator levels (100% unmute, once):")
    for vl in set_operator_levels_100(operator_speaker, operator_mic):
        lines.append(f"  {vl}")
    lines.append(f"Config directory: {config_dir()}")
    return lines
