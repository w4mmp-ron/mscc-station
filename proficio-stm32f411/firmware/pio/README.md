# PlatformIO — Proficio F411

## Requirements

- [PlatformIO Core](https://platformio.org/) or VS Code + PlatformIO IDE  
- ST-Link for upload/debug  
- WeAct **STM32F411CEU6** Black Pill (when available)

## Build / flash

```bash
cd firmware/pio
pio run              # build
pio run -t upload    # flash via ST-Link
pio device list
```

Last verified: **SUCCESS** (~9.7 KB flash control-path image).

## What you get (control path)

| Feature | Status |
|---------|--------|
| Clock HSE 25 MHz → 96 MHz (HSI fallback) | yes |
| PC13 LED heartbeat | yes |
| Control/Status GPIO (RX, AMP, BS0–2, KEY0/1) | yes |
| I2C1 PB8/PB9 | yes |
| USB vendor opcode dispatch | yes (no USB stack yet) |
| SI5351 soft/hard tune | yes (needs chip on bus) |
| CW Configure + MKII paddles | yes |
| Legacy DIN CW | build `-D PROFICIO_CW_MKII=0` |
| Band / TX inhibit | yes |
| USB Audio | no |

## Module map

See root `RESUME.md`. Headers under `include/`, sources under `src/`.

## USB stack (next)

`usb_vendor_setup()` / `usb_vendor_complete_out()` are ready for TinyUSB or ST USB Device class vendor handlers. Device descriptors / VID:PID still to choose (clone Multus or use temporary test IDs).

## STM32CubeIDE

See `../cubeide/Proficio_F411.ioc` + `../cubeide/README.md` (may lag PIO sources).
