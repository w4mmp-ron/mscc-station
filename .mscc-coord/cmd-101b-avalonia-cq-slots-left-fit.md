# cmd-101b — Avalonia: CQ memory slots show all 48 characters; left panel fits at 1024×600

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. WPF tree: read only. **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No radio protocol commands; bindings and commands unchanged. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.
>
> **Do not investigate the resize crash** (RX/TX or QRP CAL, 1024 → 1280) in this command. Stew's test result is not in yet; it gets its own command if needed.

Follows cmd-101 (Build `4b5added`). Stew: tab borders look real nice; the rest of the CW tab looks great. Two fixes.

Line numbers are from `4b5added` (`Views/MainWindow.axaml` unless noted).

## 1. CQ memory slots: all 48 characters visible

**Problem:** at `Width="360"` a slot shows only **31** characters. 360 − 8 (padding) − 2 (border) = 350 px / 31 ≈ **11.3 px per character**. A 12 pt monospace font is ~7.2 px per character, so the slot is **not** rendering in a monospace font. The font list `Consolas,Courier New,monospace` (622, 640, 658, 676) does not resolve on stew-HP: Consolas is not installed and Avalonia has no generic `monospace` alias, so it falls back to the proportional default font (wide capitals such as W and M are ~11 px).

**Fix:**
1. Four slot TextBoxes (621, 639, 657, 675): `FontFamily="Consolas, DejaVu Sans Mono, Liberation Mono, Courier New, monospace"`. Avalonia uses the first installed family: Consolas on Windows, DejaVu Sans Mono on Ubuntu and Raspberry Pi OS. Do not change the font on any other control in this command.
2. Slot width from the real metrics, not a guess: in code or in a quick debug run, measure the TextBox text extent for 48 × `W` at FontSize 12 in the resolved font. Expected ≈ 48 × 7.22 = **347 px** (DejaVu Sans Mono, 0.602 em). Slot width = measured + 8 (Padding 4,3 left/right) + 2 (border) + 4 (caret and rounding) → expected ≈ **361 px**.
3. Make the slot column flexible with a floor and a cap, so wide fonts never truncate and 1280 does not waste space:
   - CQ grid (617): `ColumnDefinitions="*,Auto,Auto,Auto"`, `HorizontalAlignment="Stretch"`.
   - Slot TextBoxes: remove `Width="360"`; set `MinWidth` = the measured value from step 2 (≈ 361) and `MaxWidth="560"`. Keep `Margin="0,2,6,2" Padding="4,3" MaxLength="48"`.
   - `HorizontalAlignment="Left"` on the TextBoxes so they stop at `MaxWidth` and R / P stay next to them.
4. Width at 1024×600: CW content ≈ **663 px** (cmd-101). R/P side = 6 + 36 (counter) + 6 + 30 (R) + 4 + 30 (P) = **112 px** → slot up to **551 px**. With MinWidth ≈ 361 and the slot at ~551, R and P are fully visible; even 48 proportional capitals (~542 px) would fit.
5. **Verify by typing** (report each, at 1024×600 and 1280×720): 48 × `W`, 48 × `M`, 48 × `0`, and a real line `CQ CQ CQ DE N8VET N8VET K` padded to 48. All 48 characters visible with no horizontal scroll inside the box, counter reads `48/48`, R and P visible.

## 2. Left panel: fits at 1024×600 with a few px spare

**Problem:** at 1024×600 the left panel content is a few px taller than its viewport, so its ScrollViewer shows a scroll bar when the pointer enters (Fluent hides the bar until pointer-over, which is why it appears on click).

**Fix:**
1. `App.axaml` 623, `StackPanel.mscc-left-rail > TextBlock.mscc-section`: `Margin="0,10,0,1"` → **`0,8,0,1`**. 5 headings × 2 px = **10 px** back.
2. Measure in a debug run at exactly 1024×600 (client area, not clamped): left ScrollViewer `Extent.Height` vs `Viewport.Height`. Target: **viewport − extent ≥ 4 px**.
3. If still under 4 px spare: `0,7,0,1`, then `0,6,0,1`. Do not change slider rows, buttons or the SERVER block.
4. Report: margin value used, extent, viewport, spare px. Confirm by moving the pointer into the left panel at 1024×600: no scroll bar appears.

## Checklist (Build)
- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] §1 slot font list; measured 48-W width reported; `MinWidth` set from it; `MaxWidth="560"`; grid `*,Auto,Auto,Auto`.
- [ ] §1 typing test (W, M, 0, real CQ line) at 1024×600 and 1280×720: 48 visible, counter 48/48, R/P visible.
- [ ] §2 left-rail heading margin; extent vs viewport with ≥ 4 px spare; no scroll bar on pointer-over at 1024×600.
- [ ] Do not investigate the resize crash.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] One commit: `Avalonia: CQ slots show 48 chars (monospace + measured MinWidth); left panel fits 1024x600`.
- [ ] Report commit hash, files changed, measurements.

## Stew's check (printable)
- [ ] Type 48 W's in each CQ slot: all 48 show; counter 48/48.
- [ ] At 1024×600: R and P visible on all four slots.
- [ ] R stores and P plays a 48-character message.
- [ ] At 1024×600: move the pointer into the left panel and click: no scroll bar.
- [ ] Left panel sections still evenly spaced.
