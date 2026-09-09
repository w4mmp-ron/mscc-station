"""
Set operator speaker/mic ALSA mixer levels to 100% after MSCC Init.

ALSA only — the hardware card always has the device. MSCC software gain
controls AF after the path is fully open.

Persistence: write operator-alsa-cards.txt; mscc.sh re-runs amixer to 100%
on every start. Do NOT use ~/.asoundrc (unreliable on Pi OS). alsactl store
alone is not reliable with USB / desktop audio either.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from pathlib import Path
from typing import List, Optional, Set, Tuple


def _run(argv: List[str], timeout: float = 10.0) -> Tuple[int, str]:
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
        return 127, "not found"
    except Exception as e:
        return 1, str(e)


def _config_dir() -> Path:
    home = os.environ.get("HOME") or str(Path.home())
    return Path(home) / ".local" / "mscc"


def _name_tokens(devname: str) -> List[str]:
    base = (devname or "").split("(", 1)[0].strip()
    base = re.sub(
        r"\b(USB|Audio|Device|Analog|Stereo|Digital|Surround|PnP|Sound)\b",
        " ",
        base,
        flags=re.I,
    )
    toks = re.findall(r"[A-Za-z0-9][A-Za-z0-9._-]{1,}", base)
    out: List[str] = []
    for t in toks:
        if t.lower() in ("card", "default", "sysdefault", "front", "surround", "hw"):
            continue
        if len(t) >= 2:
            out.append(t)
    return out


def _score_match(devname: str, target: str) -> int:
    if not target:
        return 0
    dn = (devname or "").lower()
    tg = target.lower()
    score = 0
    if dn and dn in tg:
        score += 50
    for t in _name_tokens(devname):
        tl = t.lower()
        if tl in tg:
            score += max(4, min(len(t), 14))
    for m in re.findall(r"[a-z0-9]{5,}", dn):
        if m in tg:
            score += 10
    return score


def _cards_from_list_cmd(cmd: List[str]) -> List[Tuple[int, str]]:
    code, out = _run(cmd)
    if code != 0 or not out:
        return []
    cards: List[Tuple[int, str]] = []
    for line in out.splitlines():
        m = re.match(r"card\s+(\d+):\s*(\S+)\s*\[([^\]]*)\]", line, re.I)
        if m:
            cards.append((int(m.group(1)), f"{m.group(2)} {m.group(3)}"))
    return cards


def _alsa_card_from_name(devname: str) -> Optional[int]:
    """Resolve PortAudio device name → ALSA card index."""
    if not devname:
        return None
    m = re.search(r"hw:(\d+)", devname, re.I)
    if m:
        return int(m.group(1))
    # card N in some PA strings
    m = re.search(r"\bcard\s*(\d+)\b", devname, re.I)
    if m:
        return int(m.group(1))

    candidates: List[Tuple[int, str]] = []
    if shutil.which("aplay"):
        candidates.extend(_cards_from_list_cmd(["aplay", "-l"]))
    if shutil.which("arecord"):
        for c in _cards_from_list_cmd(["arecord", "-l"]):
            if c not in candidates:
                candidates.append(c)
    # /proc/asound/cards
    try:
        text = Path("/proc/asound/cards").read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            m = re.match(r"\s*(\d+)\s+\[([^\]]+)\]:\s*(.*)", line)
            if m:
                card = int(m.group(1))
                label = f"{m.group(2)} {m.group(3)}"
                if not any(c[0] == card for c in candidates):
                    candidates.append((card, label))
    except OSError:
        pass

    best_card: Optional[int] = None
    best = 0
    for card, label in candidates:
        # skip pure HDMI for operator matching unless name says hdmi
        if "hdmi" in label.lower() and "hdmi" not in (devname or "").lower():
            continue
        s = _score_match(devname, label)
        if s > best:
            best = s
            best_card = card
    if best < 6:
        return None
    return best_card


def _list_simple_controls(card: int) -> List[str]:
    if not shutil.which("amixer"):
        return []
    code, out = _run(["amixer", "-c", str(card), "scontrols"])
    if code != 0 or not out:
        return []
    names: List[str] = []
    for line in out.splitlines():
        # Simple mixer control 'Master',0
        m = re.search(r"'([^']+)'", line)
        if m:
            names.append(m.group(1))
    return names


def _set_amixer_card(card: int) -> List[str]:
    """Open all simple mixer controls on card to 100% unmuted (playback + capture)."""
    if not shutil.which("amixer"):
        return [f"amixer not installed (need alsa-utils) for card {card}"]
    notes: List[str] = []
    controls = _list_simple_controls(card)
    if not controls:
        # fallback well-known names
        controls = [
            "Master",
            "PCM",
            "Speaker",
            "Headphone",
            "Playback",
            "Digital",
            "Capture",
            "Mic",
            "Mic Capture",
            "PGA",
            "ADC",
            "Line",
            "Boost",
        ]
    any_ok = False
    for ctl in controls:
        code, _ = _run(["amixer", "-c", str(card), "sset", ctl, "100%", "unmute"])
        if code != 0:
            code, _ = _run(["amixer", "-c", str(card), "sset", ctl, "100%"])
        if code != 0:
            # enums / switches: try unmute only
            code, _ = _run(["amixer", "-c", str(card), "sset", ctl, "unmute"])
        if code == 0:
            notes.append(f"ALSA card {card} '{ctl}' → 100%/unmute")
            any_ok = True
    if not any_ok:
        notes.append(
            f"ALSA card {card}: could not set controls "
            f"(try: alsamixer -c {card})"
        )
    return notes


def _remember_cards(cards: Set[int]) -> List[str]:
    """
    Remember operator ALSA card numbers for mscc start.
    Levels are re-applied with amixer on every start (not ~/.asoundrc,
    not alsactl — those do not stick reliably on Pi OS / USB audio).
    """
    notes: List[str] = []
    d = _config_dir()
    try:
        d.mkdir(parents=True, exist_ok=True)
        p = d / "operator-alsa-cards.txt"
        p.write_text(
            "\n".join(str(c) for c in sorted(cards)) + "\n",
            encoding="utf-8",
        )
        notes.append(
            f"Remembered ALSA card(s) → {p} "
            "(mscc start re-applies 100% via amixer each time)"
        )
    except OSError as e:
        notes.append(f"could not write operator-alsa-cards.txt: {e}")
    return notes


def set_operator_levels_100(speaker_name: str, mic_name: str) -> List[str]:
    """
    Unmute and set ALSA mixer for operator speaker + mic cards to 100%,
    remember card numbers for mscc start re-apply.
    """
    lines: List[str] = []
    cards: Set[int] = set()

    sp_card = _alsa_card_from_name(speaker_name)
    mic_card = _alsa_card_from_name(mic_name)

    if sp_card is not None:
        lines.append(f"Speaker '{speaker_name[:50]}' → ALSA card {sp_card}")
        cards.add(sp_card)
    else:
        lines.append(
            f"Speaker: no ALSA card match for '{speaker_name[:60]}' "
            "(check aplay -l vs PortAudio name)"
        )

    if mic_card is not None:
        lines.append(f"Mic '{mic_name[:50]}' → ALSA card {mic_card}")
        cards.add(mic_card)
    else:
        lines.append(
            f"Mic: no ALSA card match for '{mic_name[:60]}' "
            "(check arecord -l vs PortAudio name)"
        )

    for card in sorted(cards):
        lines.extend(_set_amixer_card(card))

    if cards:
        lines.extend(_remember_cards(cards))
    else:
        lines.append("No ALSA cards updated — open alsamixer once if quiet")

    return lines
