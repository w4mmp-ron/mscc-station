"""Read MSCC audio device names from $HOME/.local/mscc."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


DIGI_SPEAKER_DEFAULT = "VirtualA"
DIGI_MIC_DEFAULT = "VirtualB.monitor"


def config_dir() -> Path:
    home = os.environ.get("HOME") or str(Path.home())
    return Path(home) / ".local" / "mscc"


def _read_name(path: Path) -> Optional[str]:
    if not path.is_file():
        return None
    try:
        text = path.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return None
    if not text:
        return None
    # Same as mscc-init: strip PortAudio "(...)" suffix if present
    return text.split("(", 1)[0].strip() or text.strip()


@dataclass
class ConfiguredDevice:
    """One volume row the GUI should manage."""

    key: str
    label: str
    kind: str  # "sink" or "source"
    preferred_name: str  # from ini or fixed Virtual*
    role: str  # digi-speaker, digi-mic, operator-speaker, operator-mic


def configured_devices() -> List[ConfiguredDevice]:
    d = config_dir()
    digi_sp = _read_name(d / "digital-speaker.ini") or DIGI_SPEAKER_DEFAULT
    digi_mic = _read_name(d / "digital-microphone.ini") or DIGI_MIC_DEFAULT
    op_sp = _read_name(d / "operator-speaker.ini")
    op_mic = _read_name(d / "operator-microphone.ini")

    rows: List[ConfiguredDevice] = [
        ConfiguredDevice(
            key="digi-speaker",
            label="Digi speaker (VirtualA)",
            kind="sink",
            preferred_name=digi_sp,
            role="digi-speaker",
        ),
        ConfiguredDevice(
            key="digi-mic",
            label="Digi mic (VirtualB.monitor)",
            kind="source",
            preferred_name=digi_mic,
            role="digi-mic",
        ),
    ]
    if op_sp:
        rows.append(
            ConfiguredDevice(
                key="op-speaker",
                label="Operator speaker",
                kind="sink",
                preferred_name=op_sp,
                role="operator-speaker",
            )
        )
    if op_mic:
        rows.append(
            ConfiguredDevice(
                key="op-mic",
                label="Operator microphone",
                kind="source",
                preferred_name=op_mic,
                role="operator-mic",
            )
        )
    return rows
