// Per-band calibration stored in the radio (RX IQ, TX IQ, power). 3.235, 2026-10-21;
// 3.236 adds clear (type 6, in usbvend01.c).
// Same USB protocol as the Black Pill 3.7 (CMD_CALIBRATION 0xC8, see basic-plus.h).
// The radio only stores the values; the server applies them.
//
// The record lives in RAM (E_cal). usbvend01.c (USB ISR) reads and writes it directly:
// a set lands straight in the record and marks the band valid, so no main loop step is
// needed. Values are raw little-endian bytes as the host sent them (the 8051 is big
// endian, so the firmware never treats them as numbers).
//
// Save: the ISR only sets E_cal_save_req. Cal_Main() (main loop) writes the record to
// EEPROM rows CAL_ROW_FIRST.. one row per pass (EEPROM_StartWrite / EEPROM_Query), so
// nothing blocks and audio keeps running. Refused while transmitting.
// Boot: Cal_Init() copies the EEPROM rows to RAM; bad magic / version / CRC = defaults
// (all 0, no band valid). KISS, one copy (Ron): power lost mid-save = recalibrate.
//
// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define

#include <basic-plus.h>
#include <usbvend.h>
#include <STRING.H>

#define CAL_ROW_FIRST   8u      // EEPROM row 0 = crystal / PPM / IQ channels (settings01.c)
#define CAL_ROWS        (sizeof(cal_record_t) / CYDEV_EEPROM_ROW_SIZE)
#define CAL_MAGIC_0     'P'
#define CAL_MAGIC_1     'C'
#define CAL_MAGIC_2     'A'
#define CAL_MAGIC_3     'L'
#define CAL_VERSION     1u
#define CAL_CRC_LEN     (sizeof(cal_record_t) - 2u)   // CRC covers everything before it
#define CAL_LOCK_RETRY  250u    // passes to wait for a busy SPC before giving up

#define CAL_ST_IDLE     0u
#define CAL_ST_WRITE    1u
#define CAL_ST_WAIT     2u

cal_record_t E_cal;
uint8 E_cal_status[2] = {CAL_SAVE_IDLE, 0u};   // [save status, 1 = loaded from EEPROM]
volatile uint8 E_cal_save_req = 0;

static uint8 l_state = CAL_ST_IDLE;
static uint8 l_row = 0;
static uint8 l_retry = 0;

// CRC-16-CCITT (0x1021, init 0xFFFF): 16-bit math is cheap on the 8051
static uint16 Cal_CRC(const uint8 *p, uint16 n) {
    uint16 crc = 0xFFFFu;
    uint8 b;

    while (n--) {
        crc ^= ((uint16)*p++) << 8;
        for (b = 0; b < 8u; b++) {
            if (crc & 0x8000u) crc = (crc << 1) ^ 0x1021u; else crc <<= 1;
        }
    }
    return crc;
}

static void Cal_Seal(void) {
    uint16 crc;

    E_cal.magic[0] = CAL_MAGIC_0;
    E_cal.magic[1] = CAL_MAGIC_1;
    E_cal.magic[2] = CAL_MAGIC_2;
    E_cal.magic[3] = CAL_MAGIC_3;
    E_cal.version = CAL_VERSION;
    crc = Cal_CRC((const uint8 *)&E_cal, CAL_CRC_LEN);
    E_cal.crc[0] = (uint8)crc;          // little endian, like the payloads
    E_cal.crc[1] = (uint8)(crc >> 8);
}

static uint8 Cal_EEPROM_Matches(void) {
    reg8 *l_ee = (reg8 *)(CYDEV_EE_BASE + (CAL_ROW_FIRST * CYDEV_EEPROM_ROW_SIZE));
    const uint8 *l_ram = (const uint8 *)&E_cal;
    uint16 i;

    for (i = 0; i < sizeof(cal_record_t); i++) {
        if (l_ee[i] != l_ram[i]) return FALSE;
    }
    return TRUE;
}

// Call after Settings_Init() (EEPROM_Start)
void Cal_Init(void) {
    reg8 *l_ee = (reg8 *)(CYDEV_EE_BASE + (CAL_ROW_FIRST * CYDEV_EEPROM_ROW_SIZE));
    uint8 *l_ram = (uint8 *)&E_cal;
    uint16 i, crc;

    for (i = 0; i < sizeof(cal_record_t); i++) l_ram[i] = l_ee[i];
    crc = Cal_CRC((const uint8 *)&E_cal, CAL_CRC_LEN);
    if (E_cal.magic[0] == CAL_MAGIC_0 && E_cal.magic[1] == CAL_MAGIC_1 &&
        E_cal.magic[2] == CAL_MAGIC_2 && E_cal.magic[3] == CAL_MAGIC_3 &&
        E_cal.version == CAL_VERSION &&
        E_cal.crc[0] == (uint8)crc && E_cal.crc[1] == (uint8)(crc >> 8)) {
        E_cal_status[1] = 1u;
    } else {
        memset(&E_cal, 0, sizeof(E_cal));
        E_cal_status[1] = 0u;
    }
}

// Main loop: runs a save the host asked for, one EEPROM row per call
void Cal_Main(void) {
    cystatus l_status;

    switch (l_state) {
    case CAL_ST_IDLE:
        if (!E_cal_save_req) break;
        E_cal_save_req = 0;
        if (TX_Request || E_PTT || E_key_down) {
            E_cal_status[0] = CAL_SAVE_REFUSED;
            break;
        }
        Cal_Seal();
        EEPROM_UpdateTemperature();
        l_row = 0;
        l_retry = 0;
        l_state = CAL_ST_WRITE;
        break;

    case CAL_ST_WRITE:
        l_status = EEPROM_StartWrite((const uint8 *)&E_cal + (l_row * CYDEV_EEPROM_ROW_SIZE),
                                     (uint8)(CAL_ROW_FIRST + l_row));
        if (l_status == CYRET_STARTED) {
            l_state = CAL_ST_WAIT;
        } else if (l_status != CYRET_LOCKED || ++l_retry >= CAL_LOCK_RETRY) {
            E_cal_status[0] = CAL_SAVE_FAILED;  // SPC error, or busy too long
            l_state = CAL_ST_IDLE;
        }                                       // locked: try again next pass
        break;

    case CAL_ST_WAIT:
        l_status = EEPROM_Query();
        if (l_status == CYRET_STARTED) break;   // row still being written
        if (l_status != CYRET_SUCCESS) {
            E_cal_status[0] = CAL_SAVE_FAILED;
            l_state = CAL_ST_IDLE;
            break;
        }
        if (++l_row < CAL_ROWS) {
            l_retry = 0;
            l_state = CAL_ST_WRITE;
            break;
        }
        if (Cal_EEPROM_Matches()) {
            E_cal_status[0] = CAL_SAVE_OK;
            E_cal_status[1] = 1u;
        } else {
            E_cal_status[0] = CAL_SAVE_FAILED;
        }
        l_state = CAL_ST_IDLE;
        break;

    default:
        l_state = CAL_ST_IDLE;
        break;
    }
}
