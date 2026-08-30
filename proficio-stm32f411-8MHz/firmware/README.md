# Firmware (STM32F411)

Skeleton only. Full Cube/HAL tree not generated yet.

## Planned build

```bash
# After CubeMX/CMake export is added:
mkdir build && cd build
cmake .. -G Ninja
ninja
# flash via openocd / probe-rs / STM32CubeProgrammer
```

## Layout

| Path | Role |
|------|------|
| `include/proficio_config.h` | Board/app config |
| `src/main.c` | Entry / loop stub |
| `src/board/` | Black Pill init |
| `src/app/` | Radio application (ported logic) |
| `platform/` | Reserved for HAL/CMSIS (generated later) |

## Phase 0 target

Blink PC13; optional UART log on a free USART.
