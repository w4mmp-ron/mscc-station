# STM32CubeIDE project (from `.ioc`)

## Generate / open

1. Install **STM32CubeIDE** (includes CubeMX).
2. **File → Open Projects from File System** is not enough for `.ioc` alone.
3. Preferred:
   - **File → New → STM32 Project** from an existing `.ioc`  
   - Or open `Proficio_F411.ioc` with CubeMX / CubeIDE → **Generate Code**.
4. Target: **STM32F411CEU6**, LED on **PC13**, HSE **25 MHz**, SYSCLK **96 MHz**.

After generation, merge blink logic from `../pio/src/main.c` or our pin headers from `../include/board_pins.h`.

## Alternative (recommended for build now)

Use **PlatformIO** project in `../pio/` — builds without CubeIDE:

```bash
cd firmware/pio
pio run
pio run -t upload
```
