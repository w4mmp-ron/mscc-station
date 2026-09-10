# Proficio PSoC firmware

PSoC Creator trees for shipping Proficio radios. Ron moved these here from the repo root.

| Path | Radio |
|------|--------|
| `Release-Proficio-MKII-PTT/` | MKII PTT |
| `Release-Proficio-MKII-ATU/` | MKII ATU |
| `Release-Proficio-Legacy/` | Legacy Proficio |
| `bootloader/` | Creator bootloader project |

Application firmware for USB HID upload is the **`.cyacd`** under each tree’s `Release/`.

Upload tools (not this folder):

| Host | CLI + GUI |
|------|-----------|
| Raspberry Pi | `rpi/psoc-usb-bootload-linux/` (`make` → `bootloader`; GUI `bootloader-gui.py`) |
| Ubuntu x86_64 | `linux/psoc-usb-bootload-linux/` (same two files; edit here for laptop fixes) |

Geminus trees remain at repo-root `Release-Geminus-*`.
