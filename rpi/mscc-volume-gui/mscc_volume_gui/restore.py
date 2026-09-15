"""Restore saved Pulse levels from volume-levels.conf (CLI + library)."""

from __future__ import annotations

import sys

from . import pactl_ops
from .persist import load_levels


def restore_all() -> int:
    """Apply all saved levels. Returns number of devices updated."""
    if not pactl_ops.pactl_available():
        print("mscc-volume-restore: pactl not available", file=sys.stderr)
        return 0
    levels = load_levels()
    if not levels:
        print("mscc-volume-restore: no saved levels")
        return 0
    catalog = pactl_ops.index_by_kind()
    n = 0
    for key, lv in levels.items():
        bucket = catalog.get(lv.kind, [])
        # Prefer exact saved Pulse name; fall back to fuzzy on that name
        hit = None
        for dev in bucket:
            if dev.name == lv.name:
                hit = dev
                break
        if hit is None:
            hit = pactl_ops.resolve_device(lv.kind, lv.name, bucket)
        if hit is None:
            print(f"mscc-volume-restore: skip {key} ({lv.name}) — not present")
            continue
        ok, err = pactl_ops.set_volume(lv.kind, hit.name, lv.pct)
        if not ok:
            print(f"mscc-volume-restore: volume fail {key}: {err}", file=sys.stderr)
            continue
        ok, err = pactl_ops.set_mute(lv.kind, hit.name, lv.mute)
        if not ok:
            print(f"mscc-volume-restore: mute fail {key}: {err}", file=sys.stderr)
            continue
        print(f"mscc-volume-restore: {key} → {hit.name} {lv.pct}% mute={int(lv.mute)}")
        n += 1
    return n


def main() -> None:
    restore_all()


if __name__ == "__main__":
    main()
