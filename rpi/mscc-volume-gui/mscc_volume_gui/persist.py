"""
Persist Pulse volumes under $HOME/.local/mscc/volume-levels.conf

Line format (shell-friendly):
  key|kind|name|pct|mute
  digi-speaker|sink|VirtualA|75|0

Written whenever the GUI changes a level. Restored by:
  - GUI (on Refresh / startup, after resolve)
  - mscc-virtual-audio.sh after recreating Virtual* sinks
  - python3 -m mscc_volume_gui.restore
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional

from .config import config_dir


LEVELS_FILE = "volume-levels.conf"


@dataclass
class SavedLevel:
    key: str
    kind: str
    name: str
    pct: int
    mute: bool


def levels_path() -> Path:
    return config_dir() / LEVELS_FILE


def load_levels() -> Dict[str, SavedLevel]:
    path = levels_path()
    out: Dict[str, SavedLevel] = {}
    if not path.is_file():
        return out
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return out
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 5:
            continue
        key, kind, name, pct_s, mute_s = parts[0], parts[1], parts[2], parts[3], parts[4]
        try:
            pct = max(0, min(150, int(pct_s)))
        except ValueError:
            continue
        mute = mute_s.strip() in ("1", "true", "yes", "on")
        if kind not in ("sink", "source") or not key or not name:
            continue
        out[key] = SavedLevel(key=key, kind=kind, name=name, pct=pct, mute=mute)
    return out


def save_level(key: str, kind: str, name: str, pct: int, mute: bool) -> None:
    """Update one key and rewrite the file."""
    if not key or not name or kind not in ("sink", "source"):
        return
    levels = load_levels()
    levels[key] = SavedLevel(
        key=key,
        kind=kind,
        name=name,
        pct=max(0, min(150, int(pct))),
        mute=bool(mute),
    )
    _write_all(levels)


def _write_all(levels: Dict[str, SavedLevel]) -> None:
    d = config_dir()
    d.mkdir(parents=True, exist_ok=True)
    path = levels_path()
    lines = [
        "# MSCC Volume GUI — sticky Pulse levels (reboot-safe)",
        "# key|kind|name|pct|mute",
    ]
    for key in sorted(levels.keys()):
        lv = levels[key]
        lines.append(
            f"{lv.key}|{lv.kind}|{lv.name}|{lv.pct}|{1 if lv.mute else 0}"
        )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def get_saved(key: str) -> Optional[SavedLevel]:
    return load_levels().get(key)
