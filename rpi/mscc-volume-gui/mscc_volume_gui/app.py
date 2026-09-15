#!/usr/bin/env python3
"""MSCC Volume GUI — Pulse levels for digi Virtual* + operator devices."""

from __future__ import annotations

import tkinter as tk
from tkinter import messagebox, ttk
from typing import List, Optional

from . import __version__
from .config import ConfiguredDevice, config_dir, configured_devices
from . import pactl_ops
from . import persist


class DeviceRow:
    def __init__(
        self,
        parent: ttk.Frame,
        cfg: ConfiguredDevice,
        resolved_name: Optional[str],
        resolved_desc: str,
        pct: int,
        muted: bool,
        missing: bool,
        missing_hint: str = "",
    ) -> None:
        self.cfg = cfg
        self.pulse_name = resolved_name
        self.kind = cfg.kind
        self._applying = False

        self.frame = ttk.LabelFrame(parent, text=cfg.label, padding=8)
        self.frame.pack(fill=tk.X, pady=6, padx=4)

        # Slider FIRST so it is never clipped below long "not found" text
        row = ttk.Frame(self.frame)
        row.pack(fill=tk.X, pady=(0, 4))

        self.var_pct = tk.IntVar(value=int(pct))
        self.scale = ttk.Scale(
            row,
            from_=0,
            to=100,
            orient=tk.HORIZONTAL,
            command=self._on_scale,
        )
        self.scale.set(pct)
        self.scale.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 8))

        self.lbl = ttk.Label(row, text=f"{pct}%", width=5)
        self.lbl.pack(side=tk.LEFT)

        self.var_mute = tk.BooleanVar(value=bool(muted))
        self.chk = ttk.Checkbutton(
            row,
            text="Mute",
            variable=self.var_mute,
            command=self._on_mute,
        )
        self.chk.pack(side=tk.LEFT, padx=(8, 0))

        if missing or not resolved_name:
            self.scale.state(["disabled"])
            self.chk.state(["disabled"])

        if missing or not resolved_name:
            status = f"not found (wanted “{cfg.preferred_name}”)"
            if missing_hint:
                status += f"\n{missing_hint}"
            fg = "#a00"
        else:
            status = f"{resolved_name}"
            if resolved_desc and resolved_desc != resolved_name:
                status += f"  —  {resolved_desc}"
            fg = "#333"

        self.status = ttk.Label(
            self.frame,
            text=status,
            foreground=fg,
            wraplength=560,
            justify=tk.LEFT,
        )
        self.status.pack(anchor=tk.W)

        self.scale.bind("<ButtonRelease-1>", self._commit_volume)
        self.scale.bind("<KeyRelease-Left>", self._commit_volume)
        self.scale.bind("<KeyRelease-Right>", self._commit_volume)

    def _on_scale(self, _val: str) -> None:
        pct = int(float(self.scale.get()))
        self.var_pct.set(pct)
        self.lbl.configure(text=f"{pct}%")

    def _persist(self, pct: int, muted: bool) -> None:
        persist.save_level(
            self.cfg.key, self.kind, self.pulse_name or self.cfg.preferred_name, pct, muted
        )

    def _commit_volume(self, _evt=None) -> None:
        if self._applying or not self.pulse_name:
            return
        pct = int(float(self.scale.get()))
        ok, err = pactl_ops.set_volume(self.kind, self.pulse_name, pct)
        if not ok:
            messagebox.showerror("Volume", err or "Failed to set volume")
            return
        self._persist(pct, self.var_mute.get())

    def _on_mute(self) -> None:
        if self._applying or not self.pulse_name:
            return
        muted = self.var_mute.get()
        ok, err = pactl_ops.set_mute(self.kind, self.pulse_name, muted)
        if not ok:
            messagebox.showerror("Mute", err or "Failed to set mute")
            return
        self._persist(int(float(self.scale.get())), muted)


class MsccVolumeApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(f"MSCC Volume  v{__version__}")
        self.minsize(600, 520)
        self.geometry("680x640")

        self._rows: List[DeviceRow] = []

        outer = ttk.Frame(self, padding=12)
        outer.pack(fill=tk.BOTH, expand=True)

        ttk.Label(
            outer,
            text="MSCC audio levels (Pulse / PipeWire)",
            font=("TkDefaultFont", 14, "bold"),
        ).pack(anchor=tk.W)

        ttk.Label(
            outer,
            text=f"Config: {config_dir()}  ·  digi Virtual* + operator devices from ini",
            foreground="#444",
        ).pack(anchor=tk.W, pady=(0, 8))

        self.warn = ttk.Label(outer, text="", foreground="#a00", wraplength=640)
        self.warn.pack(anchor=tk.W)

        btnrow = ttk.Frame(outer)
        btnrow.pack(fill=tk.X, pady=(0, 8))
        ttk.Button(btnrow, text="Refresh", command=self.rebuild).pack(side=tk.LEFT)
        ttk.Button(btnrow, text="Close", command=self.destroy).pack(side=tk.RIGHT)

        # Scrollable body — operator rows were getting clipped
        wrap = ttk.Frame(outer)
        wrap.pack(fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(wrap, highlightthickness=0)
        self.scroll = ttk.Scrollbar(wrap, orient=tk.VERTICAL, command=self.canvas.yview)
        self.body = ttk.Frame(self.canvas)
        self.body.bind(
            "<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")),
        )
        self._body_win = self.canvas.create_window((0, 0), window=self.body, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scroll.set)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.scroll.pack(side=tk.RIGHT, fill=tk.Y)

        def _on_canvas_configure(event: tk.Event) -> None:
            self.canvas.itemconfigure(self._body_win, width=event.width)

        self.canvas.bind("<Configure>", _on_canvas_configure)

        def _on_mousewheel(event: tk.Event) -> None:
            # Linux: Button-4/5; Windows/mac: delta
            if getattr(event, "num", None) == 4:
                self.canvas.yview_scroll(-1, "units")
            elif getattr(event, "num", None) == 5:
                self.canvas.yview_scroll(1, "units")
            else:
                delta = int(getattr(event, "delta", 0))
                if delta:
                    self.canvas.yview_scroll(int(-1 * (delta / 120)), "units")

        self.canvas.bind_all("<MouseWheel>", _on_mousewheel)
        self.canvas.bind_all("<Button-4>", _on_mousewheel)
        self.canvas.bind_all("<Button-5>", _on_mousewheel)

        self.footer = ttk.Label(
            outer,
            text="Install under ~/mscc/mscc-volume-gui  ·  sticky → ~/.local/mscc/volume-levels.conf  ·  v1.0.3+",
            foreground="#666",
        )
        self.footer.pack(anchor=tk.W, pady=(8, 0))

        self.rebuild()

    def rebuild(self) -> None:
        for child in self.body.winfo_children():
            child.destroy()
        self._rows.clear()
        self.warn.configure(text="")

        if not pactl_ops.pactl_available():
            self.warn.configure(
                text="pactl not found. Install pipewire-pulse or pulseaudio-utils, "
                "and run under your desktop user (not root)."
            )
            return

        try:
            catalog = pactl_ops.index_by_kind()
        except Exception as e:
            self.warn.configure(text=f"pactl error: {e}")
            return

        cfgs = configured_devices()
        if not cfgs:
            self.warn.configure(text="No configured devices (missing ~/.local/mscc?).")
            return

        resolved_map = {}
        op_speaker_pulse = None
        for cfg in cfgs:
            if cfg.role == "operator-speaker":
                bucket = catalog.get(cfg.kind, [])
                hit = pactl_ops.resolve_device(cfg.kind, cfg.preferred_name, bucket)
                resolved_map[cfg.key] = hit
                if hit:
                    op_speaker_pulse = hit.name

        for cfg in cfgs:
            if cfg.key in resolved_map:
                resolved = resolved_map[cfg.key]
            else:
                bucket = catalog.get(cfg.kind, [])
                hint = op_speaker_pulse if cfg.role == "operator-mic" else None
                resolved = pactl_ops.resolve_device(
                    cfg.kind,
                    cfg.preferred_name,
                    bucket,
                    hint_peer_name=hint,
                )
            if resolved:
                pct = resolved.volume_pct
                muted = resolved.muted
                # Apply sticky saved level (survives Virtual* recreate / reboot)
                saved = persist.get_saved(cfg.key)
                if saved is not None:
                    pactl_ops.set_volume(cfg.kind, resolved.name, saved.pct)
                    pactl_ops.set_mute(cfg.kind, resolved.name, saved.mute)
                    pct, muted = saved.pct, saved.mute
                    # Refresh stored Pulse name if it changed after reboot
                    persist.save_level(cfg.key, cfg.kind, resolved.name, pct, muted)
                self._rows.append(
                    DeviceRow(
                        self.body,
                        cfg,
                        resolved.name,
                        resolved.description,
                        pct,
                        muted,
                        missing=False,
                    )
                )
            else:
                bucket = catalog.get(cfg.kind, [])
                hint = pactl_ops.format_catalog_hint(cfg.kind, bucket)
                self._rows.append(
                    DeviceRow(
                        self.body,
                        cfg,
                        None,
                        "",
                        0,
                        False,
                        missing=True,
                        missing_hint=f"available {cfg.kind}s: {hint}",
                    )
                )

        shown = {r.pulse_name for r in self._rows if r.pulse_name}
        extras = [
            ("sink", "VirtualB", "Digi VirtualB sink"),
            ("source", "VirtualA.monitor", "Digi VirtualA.monitor"),
        ]
        for kind, name, label in extras:
            if name in shown:
                continue
            bucket = catalog.get(kind, [])
            hit = next((d for d in bucket if d.name == name), None)
            if not hit:
                continue
            cfg = ConfiguredDevice(
                key=f"extra-{name}",
                label=label,
                kind=kind,
                preferred_name=name,
                role="extra",
            )
            self._rows.append(
                DeviceRow(
                    self.body,
                    cfg,
                    hit.name,
                    hit.description,
                    hit.volume_pct,
                    hit.muted,
                    missing=False,
                )
            )

        self.body.update_idletasks()
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))


def main() -> None:
    app = MsccVolumeApp()
    app.mainloop()


if __name__ == "__main__":
    main()
