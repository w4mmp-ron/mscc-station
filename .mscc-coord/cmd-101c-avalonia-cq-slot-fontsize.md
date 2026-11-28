# cmd-101c — Avalonia: larger CQ memory slot font so 48 characters nearly fill the slot at 1024×600

> **STOP — read first.** Build commits only. **No push, no installer, no GitHub Release, no version bump** (Avalonia stays **1.7.08**). Avalonia only: `Avalonia/src/MSCC.Avalonia`. WPF tree: read only. **Do not touch** `rpi/`, `Solidus/`, `installers/rpi/`, **`Controls/UiChromeTheme.cs`**, or `mscc-ui/Avalonia-Migration`. No radio protocol commands; bindings and commands unchanged. Do not commit bin/obj/publish/.deb/VS files or `.mscc-coord/status/ubuntu-stew.md`. Do not pull, merge or rebase. Stew pushes. Ask Stew before any destructive git step.
>
> Do not investigate the resize crash in this command.

Follows cmd-101b (Build `1c8b2bdb`). Stew: slot size is good (monospace works; 48 M's take about two thirds of the slot at 1024×600, slot ≈ 550 px). The text is too small; the slot height has room for a larger font so 48 characters fill the slot.

Tiny command: font size only (and MinWidth from the new size). Layout, MaxWidth 560, grid `*,Auto,Auto,Auto`, MaxLength 48, font family list, R/P and left-panel fit stay as they are.

Line numbers from `1c8b2bdb`, `Views/MainWindow.axaml`. Four slot TextBoxes: 621, 639, 657, 675 (`FontSize="12"` today).

## Change

1. At 1024×600, measure the rendered slot width (`Wslot`, Stew saw ≈ 550) and the text extent of 48 × `W` at the current FontSize 12 in the resolved font (`T12`).
2. Largest integer FontSize `N` where `T12 × (N / 12) + 8 (padding) + 2 (border) + 4 (caret/rounding) ≤ Wslot`. Expected **17 or 18**. If 18 overflows by 1–3 px, use 17.
3. Set all four slots (and their placeholders — same TextBox, same font) to `FontSize="N"`.
4. Recompute `MinWidth` from a measured 48 × `W` at FontSize `N`, plus the same +8+2+4. Expected ≈ 361 × N / 12 (about 526 at 17, 541 at 18). Keep `MaxWidth="560"`. `MinWidth` must stay ≤ `Wslot` at 1024×600 so the layout does not push R/P off.
5. Slot height: leave padding and row height alone if the glyphs fit vertically with ≥ 2 px inside the box above and below the text. If they clip, raise `Padding` top/bottom by the smallest even amount that clears them (4,3 → 4,4 or 4,5) and report the new box height.
6. **Typing test** at 1024×600 and 1280×720: 48 × `W`, 48 × `M`, 48 × `0`. All 48 visible with no horizontal scroll in the box, counter `48/48`, R and P visible. At 1024, 48 W's should nearly fill the slot (a few px of spare at the right is correct; they must not clip).

## Checklist (Build)

- [ ] `git fetch` + `git status -sb`; report if origin moved. Do not pull.
- [ ] `Wslot`, `T12`, chosen `N`, new `MinWidth`, and slot height (before/after if changed) reported.
- [ ] Typing test passed at both sizes.
- [ ] `dotnet build -c Release --no-incremental`: 0 errors, no new warnings.
- [ ] One commit: `Avalonia: larger CQ slot font so 48 characters fill the box at 1024`.
- [ ] Report commit hash, files changed, measurements.

## Stew's check (printable)

- [ ] At 1024×600, 48 W's nearly fill each CQ slot; all 48 show; counter 48/48.
- [ ] R and P still visible on all four slots.
- [ ] Text is not clipped at the top or bottom of the box.
- [ ] R stores and P plays a 48-character message.
