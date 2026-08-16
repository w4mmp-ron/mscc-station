# Reference: PSoC Proficio sources

Do **not** duplicate the full PSoC tree here. Port from:

```text
../Release-Proficio-MKII-PTT/Proficio-MKII-PTT.cydsn/
```

## Primary application files

| File | Port notes |
|------|------------|
| `main.c` | Main loop structure, globals (`E_*`) |
| `usbvend.h` / `usbvend01.c` | Vendor USB opcodes (start with this) |
| `cw.c` | Keyer I2C state machine, mem queue |
| `si5351a.c` / `si5351.c` | LO |
| `audio.c` / `pcm3060.c` | USB audio + codec (late) |
| `band.c` / `tx.c` / `power.c` | GPIO / band / TX |
| `display.c` / `bias.c` / `temperature.c` | Peripherals |
| `basic-plus.h` | Shared externs |

## Do not port

- `Generated_Source/`, `codegentemp/`  
- Keil / PSoC Creator project files  
- Verilog FracN/SyncSOF as-is — redesign on STM32
