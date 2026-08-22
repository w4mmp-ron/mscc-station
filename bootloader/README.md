# Omnia SDR Proficio — device bootloader

This project provides the **on-chip bootloader** required by other Proficio projects.
Other firmware projects obtain the hex/elf from this build.

Built with PSoC Creator 4.x — http://www.cypress.com/

## Launch policy (2026-08-22)

| How you enter | Behavior |
|---------------|----------|
| **BOOT jumper** (pin low) | Stay in bootloader, wait for host (`04b4:b71d`) |
| **USB `CMD_ENTER_BOOTLOADER` (0x0E)** via app | Stay in bootloader (was broken: jumped straight back to app) |
| Normal power-up, no jumper, valid app | Jump to application |

After changing `main.c`, rebuild this bootloader project and flash it **once** with the BOOT jumper (Windows `bootloader.exe` or Programmer). Then app-side `--enter-bootloader` can work.

Linux host tool: `worktrees/psoc-usb-bootload-linux` → binary `./bootloader`.

It is normal to get errors about missing files until you have built
the project for the first time (generated files are not in git). 




