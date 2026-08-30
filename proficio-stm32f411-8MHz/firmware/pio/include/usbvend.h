/**
 * USB vendor opcodes — keep in lockstep with ms-sdr / PSoC usbvend.h
 */
#ifndef USBVEND_H
#define USBVEND_H

/* Host ← device (D2H) */
#define CMD_GET_VERSION     0x00
#define CMD_GET_PIN         0x02
#define CMD_GET_FREQ        0x3A
#define CMD_GET_STARTUP     0x3C
#define CMD_GET_XTAL        0x3D
#define CMD_GET_SI570       0x3F
#define CMD_SET_USRP1       0x50
#define CMD_GET_CW_KEY      0x51
#define CMD_SET_SI570       0x20

/* Host → device (H2D) */
#define CMD_SET_FREQ_REG    0x30
#define CMD_SET_FREQ        0x32
#define CMD_SET_XTAL_INT    0x33
#define CMD_SET_PPM         0x35
#define CMD_SET_XTAL_DEC    0x3A
#define CMD_GET_PPM_INT     0x94
#define CMD_GET_PPM_DEC     0x95

#define CMD_GET_POTENTIA_POWER       0x05
#define CMD_GET_POTENTIA_TEMPERATURE 0x06
#define CMD_SET_SET_WIPER            0x09

#define SET_CW_MODE         0x70
#define SET_KEYER_MODE      0x71
#define SET_QSK             0x72
#define SET_CW_PADDLE       0x73
#define SET_IAMBIC_TYPE     0x74
#define SET_SPACING         0x75
#define SET_MEM_TEXT_WPM    0x76
#define SET_MEMORY_TYPE     SET_MEM_TEXT_WPM
#define SET_WEIGHT          0x77
#define SET_SEMI_BREAKIN    0x78
#define SET_SEMI_CONTROL    0x79
#define SET_TX_HOLD         0x7A
#define SET_WPM             0x7B
#define SET_IAMBIC_TUNING   0x7C
#define SET_KEYER_INSTALLED 0x7D
#define SET_CW_INTERFACE_METHOD 0x7E
#define SET_SIDE_TONE       0x7F

#define SET_CW_RECORD_MESSAGE 0x80
#define SET_CW_PLAY_MSG       0x81
#define SET_CW_STOP_MSG       0x82

#define CMD_SET_KEYER_MEMORY  0x9C

#define CMD_SET_TRANSCEIVER_CW_PITCH 0x90
#define DLL_VERSION           0xA0
#define CMD_SET_BAND_VOLUME_BAND   0xA1
#define CMD_SET_BAND_VOLUME_VOLUME 0xA2
#define CMD_GET_KEY_DOWN      0xA4
#define CMD_GET_PTT           0xA5
#define CMD_SET_RIG_TUNE      0xA6
#define CMD_SET_CALIBRATE     0xA7
#define CMD_SET_POWER_BAND    0xA8
#define CMD_SET_TRANSVERTER   0xA9
#define CMD_SET_BAND_VOLUME_DEFAULTS 0xAA
#define CMD_GET_BAND_VOLUME   0xB4
#define CMD_SET_TRANSCEIVER_DISPLAY 0xCC
#define CMD_SET_STAR          0xCD
#define CMD_SET_STEP_VALUE    0xCE
#define CMD_SET_S_METER       0xC0
#define CMD_GET_MIA_STATUS    0xBE
#define CMD_GET_TRANSCEIVER_TEMP 0xBF

#define CMD_SET_HDSDR_STATUS  0xF0
#define CMD_GET_HDSDR_STATUS  0xF1
#define CMD_SET_PCB_VERSION   0xF3
#define CMD_STOP_GUI          0xFF
#define CMD_SET_PA_BYPASS     0xF7
/* Host OUT (any length / zero): enter STM32 ROM bootloader for CubeProgrammer */
#define CMD_ENTER_BOOTLOADER  0xFE

#define SI570_DLL  0
#define SI5351_DLL 1

#define CMD_SET_LEFT_VOLUME   0xE0
#define CMD_SET_RIGHT_VOLUME  0xE1

#endif /* USBVEND_H */
