/**
 * Control and status register abstraction (PSoC Control_Read/Write, Status_Read).
 */
#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

/* Status register bits (PSoC Status_Read) */
#define STATUS_KEY_0  0x01u
#define STATUS_KEY_1  0x02u
#define STATUS_BOOT   0x04u
#define STATUS_BEAT   0x08u
#define STATUS_PTT    0x10u

/* Control register bits (PSoC Control_Write) — active-low for RX/AMP */
#define CONTROL_LED      0x01u
#define CONTROL_RX       0x02u
#define CONTROL_DIN      0x04u
#define CONTROL_AMP      0x08u
#define CONTROL_DOUT     0x10u
#define CONTROL_ATU_0    0x20u
#define CONTROL_ATU_0_OE 0x40u
#define CONTROL_ATU_1    0x80u

/* Band control (BS0/BS1/BS2 encoding) */
#define CONTROL_BAND_160   0x05u
#define CONTROL_BAND_80    0x04u
#define CONTROL_BAND_40_60 0x03u
#define CONTROL_BAND_20_30 0x02u
#define CONTROL_BAND_15_17 0x01u
#define CONTROL_BAND_10_12 0x00u

void     control_init(void);
uint8_t  Control_Read(void);
void     Control_Write(uint8_t value);
uint8_t  Status_Read(void);
void     Band_Control_Write(uint8_t band_code);

/** Optional USBV+ sense (1 = present). Never required for USB enum. */
uint8_t  vbus_sense_present(void);

/* CW hold timer (ms remaining; 0 = expired). Polled by CW. */
void     cw_hold_start_ms(uint32_t ms);
void     cw_hold_poll(void);          /* call from SysTick / main */
uint8_t  cw_hold_active(void);        /* 1 while running */
void     cw_hold_force_expired(void);

#endif /* CONTROL_H */
