"""Thin wrappers around pactl for sink/source volume and mute."""

from __future__ import annotations

import re
import shutil
import subprocess
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


@dataclass
class PulseDevice:
    name: str
    description: str
    kind: str  # sink | source
    volume_pct: int
    muted: bool


def pactl_available() -> bool:
    return shutil.which("pactl") is not None


def _run(argv: List[str], timeout: float = 8.0) -> Tuple[int, str]:
    try:
        r = subprocess.run(
            argv,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        out = ((r.stdout or "") + (r.stderr or "")).strip()
        return r.returncode, out
    except FileNotFoundError:
        return 127, "pactl not found"
    except Exception as e:
        return 1, str(e)


def _parse_volume_pct(text: str) -> int:
    # "Volume: front-left: 39321 /  60% / -13.00 dB, ..."
    m = re.search(r"/\s*(\d+)%", text)
    if m:
        return max(0, min(150, int(m.group(1))))
    return 0


def _parse_mute(text: str) -> bool:
    m = re.search(r"Mute:\s*(yes|no)", text, re.I)
    if m:
        return m.group(1).lower() == "yes"
    # get-*-mute often prints only "yes"/"no" or "Mute: yes"
    t = (text or "").strip().lower()
    if t in ("yes", "no"):
        return t == "yes"
    return False


def _descriptions(kind: str) -> Dict[str, str]:
    """Map Name → Description from `pactl list {sinks|sources}`."""
    code, out = _run(["pactl", "list", f"{kind}s"], timeout=15.0)
    if code != 0 or not out:
        return {}
    mapping: Dict[str, str] = {}
    name: Optional[str] = None
    for line in out.splitlines():
        m = re.match(r"\s*Name:\s*(\S+)\s*$", line)
        if m:
            name = m.group(1)
            continue
        m = re.match(r"\s*Description:\s*(.+)\s*$", line)
        if m and name:
            mapping[name] = m.group(1).strip()
            name = None
    return mapping


def list_devices(kind: str) -> List[PulseDevice]:
    """kind = sink or source."""
    assert kind in ("sink", "source")
    code, out = _run(["pactl", "list", "short", f"{kind}s"])
    if code != 0 or not out:
        return []
    descs = _descriptions(kind)
    devices: List[PulseDevice] = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        name = parts[1]
        pct, muted = get_volume(kind, name)
        devices.append(
            PulseDevice(
                name=name,
                description=descs.get(name, name),
                kind=kind,
                volume_pct=pct,
                muted=muted,
            )
        )
    return devices


def get_volume(kind: str, name: str) -> Tuple[int, bool]:
    code, out = _run(["pactl", f"get-{kind}-volume", name])
    pct = _parse_volume_pct(out) if code == 0 else 0
    code2, out2 = _run(["pactl", f"get-{kind}-mute", name])
    muted = _parse_mute(out2) if code2 == 0 else False
    return pct, muted


def set_volume(kind: str, name: str, pct: int) -> Tuple[bool, str]:
    pct = max(0, min(150, int(pct)))
    code, out = _run(["pactl", f"set-{kind}-volume", name, f"{pct}%"])
    if code != 0:
        return False, out or f"set-{kind}-volume failed"
    return True, ""


def set_mute(kind: str, name: str, muted: bool) -> Tuple[bool, str]:
    code, out = _run(["pactl", f"set-{kind}-mute", name, "1" if muted else "0"])
    if code != 0:
        return False, out or f"set-{kind}-mute failed"
    return True, ""


def _norm(s: str) -> str:
    return re.sub(r"[^a-z0-9]+", " ", (s or "").lower()).strip()


def _looks_usb(s: str) -> bool:
    n = _norm(s)
    return "usb" in n or "advanced audio" in n


def _looks_hat(s: str) -> bool:
    n = _norm(s)
    return (
        "audioinjector" in n
        or "soc sound" in n
        or "soc_sound" in n.replace(" ", "_")
        or "wm8731" in n
        or "hifiberry" in n
    )


def _score(preferred: str, name: str, description: str) -> int:
    if not preferred:
        return 0
    p = preferred.lower().strip()
    pn = _norm(preferred)
    n = (name or "").lower()
    d = (description or "").lower()
    blob = f"{n} {d}"
    bn = _norm(blob)
    score = 0
    if p == n or p == d or pn == _norm(n) or pn == _norm(d):
        score += 100
    if p in n or p in d or pn in bn:
        score += 40

    # Keep meaningful tokens — including usb / advanced (USB mic names)
    stop = {"the", "a", "an", "of", "and", "card", "default", "sysdefault"}
    for tok in pn.split():
        if len(tok) < 2 or tok in stop:
            continue
        if tok in bn:
            # short generic words worth less
            if tok in ("audio", "device", "analog", "stereo", "input", "output"):
                score += 6
            else:
                score += max(8, min(len(tok), 20))

    if "audioinjector" in pn and (
        "audioinjector" in bn or "soc_sound" in blob or "soc sound" in bn or "wm8731" in bn
    ):
        score += 50
    if "advanced" in pn and "advanced" in bn:
        score += 40
    if _looks_usb(preferred) and ("usb" in blob or name.startswith("alsa_input.usb-") or name.startswith("alsa_output.usb-")):
        score += 35

    # Different device family → heavy penalty (HAT speaker must not steal USB mic)
    if _looks_usb(preferred) and _looks_hat(blob) and not _looks_usb(blob):
        score -= 80
    if _looks_hat(preferred) and _looks_usb(blob) and not _looks_hat(blob):
        score -= 80

    if ".monitor" in n:
        score -= 25
    return score


def counterpart_io_name(name: str, want_kind: str) -> Optional[str]:
    """Map alsa_output.* ↔ alsa_input.* (same card path)."""
    if not name:
        return None
    if want_kind == "source" and name.startswith("alsa_output."):
        return "alsa_input." + name[len("alsa_output.") :]
    if want_kind == "sink" and name.startswith("alsa_input."):
        return "alsa_output." + name[len("alsa_input.") :]
    return None


def _alsa_card_stem(name: str) -> Optional[str]:
    """
    alsa_output.platform-soc_sound.stereo-fallback → platform-soc_sound
    alsa_input.usb-Foo_Bar-00.analog-stereo → usb-Foo_Bar-00
    """
    m = re.match(r"alsa_(?:output|input)\.([^.]+)", name or "")
    return m.group(1) if m else None


def resolve_device(
    kind: str,
    preferred_name: str,
    catalog: Optional[List[PulseDevice]] = None,
    *,
    hint_peer_name: Optional[str] = None,
) -> Optional[PulseDevice]:
    """Best-match Pulse sink/source for an ini / Virtual name."""
    devices = catalog if catalog is not None else list_devices(kind)
    if not devices:
        return None

    # 1) Exact name / description
    for dev in devices:
        if dev.name == preferred_name or dev.description == preferred_name:
            return dev
        if _norm(dev.description) == _norm(preferred_name):
            return dev

    # 2) Peer hint ONLY when preferred is same device family as the peer
    #    (AudioInjector speaker must NOT force HAT mic when ini says USB mic)
    if hint_peer_name:
        peer_ok = True
        if _looks_usb(preferred_name) and _looks_hat(hint_peer_name):
            peer_ok = False
        if _looks_hat(preferred_name) and _looks_usb(hint_peer_name):
            peer_ok = False
        if peer_ok:
            alt = counterpart_io_name(hint_peer_name, kind)
            if alt:
                for dev in devices:
                    if dev.name == alt:
                        return dev
                stem = _alsa_card_stem(hint_peer_name)
                prefix = (
                    f"alsa_input.{stem}."
                    if kind == "source" and stem
                    else f"alsa_output.{stem}."
                    if kind == "sink" and stem
                    else None
                )
                if prefix:
                    for dev in devices:
                        if dev.name.startswith(prefix):
                            return dev

    # 3) Fuzzy score
    best: Optional[PulseDevice] = None
    best_score = 0
    for dev in devices:
        s = _score(preferred_name, dev.name, dev.description)
        if s > best_score:
            best_score = s
            best = dev
    if best_score < 12:
        return None
    return best


def format_catalog_hint(kind: str, catalog: List[PulseDevice], limit: int = 8) -> str:
    """Short list of Pulse names for 'not found' UI."""
    lines = []
    for dev in catalog[:limit]:
        if dev.description and dev.description != dev.name:
            lines.append(f"{dev.name} ({dev.description})")
        else:
            lines.append(dev.name)
    more = "" if len(catalog) <= limit else f" … +{len(catalog) - limit} more"
    return "; ".join(lines) + more if lines else "(none)"


def index_by_kind() -> Dict[str, List[PulseDevice]]:
    return {
        "sink": list_devices("sink"),
        "source": list_devices("source"),
    }
