# PA / board NTC temperature

| Item | Value |
|------|--------|
| MCU pin | **PA4** (WeAct silk **A4**) |
| ADC | ADC1_IN4 |
| USB opcode | **`0x06`** `CMD_GET_POTENTIA_TEMPERATURE` (Solidus retired; reused for PA NTC) |
| Die / chip temp | **`0xBF`** `CMD_GET_TRANSCEIVER_TEMP` (unchanged) |
| Host variable | Firmware `E_pa_temp` (°C, int32, big-endian on USB like `0xBF`) |

## Default divider (firmware constants in `pa_ntc.c`)

```
3.3V -- Rfixed 10k -- ADC (PA4) -- NTC 10k@25°C, β=3950 -- GND
```

NTC return may use any solid board GND. Tune `PA_NTC_R_FIXED_OHMS` / `PA_NTC_R25_OHMS` / `PA_NTC_BETA` if Stew’s parts differ.

## Not on U2 lock

PA4 is intentionally free of the Stew U2 pin lock (`docs/BLACK-PILL-TO-U2.md`).
