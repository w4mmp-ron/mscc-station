#pragma once
//************************************************************************
//**
//** Project......: Firmware USB AVR Si570 controler.
//**
//** Platform.....: ATtiny[48]5 @ 16.5 MHz
//**
//** Licence......: This software is freely available for non-commercial 
//**                use - i.e. for research and experimentation only!
//**
//** Programmer...: F.W. Krom, PE0FKO
//** 
//** Description..: USB commando codes
//**
//** History......: V15.12:	First version
//**
//**************************************************************************
//



#define CMD_SET_SPECTRUM_WATERFALL_FREQ 0x91
#define CMD_SET_SPECTRUM_WATERFALL_MODE 0x92
#define CMD_GET_PPM_INT 0x94
#define CMD_GET_PPM_DEC 0x95


// 
//Unused

//#define GET_IAMBIC_TUNING 0x9C  /* retired — see CMD_SET_KEYER_MEMORY */
//#define GET_CW_DEFAULTS 0x9D
//#define GET_CW_INTERFACE_METHOD 0x9E
//#define GET_SIDE_TONE 0x9F
//#define SET_IAMBIC_TYPE 0x74
//#define SET_MEMORY_TYPE 0x76  /* retired — see SET_MEM_TEXT_WPM */
//#define SET_SEMI_BREAKIN 0x78
//#define SET_SEMI_CONTROL 0x79
//#define SET_IAMBIC_TUNING 0x7C
//#define SET_CW_INTERFACE_METHOD 0x7E


//#define CMD_ECHO_WORD			0x00	// V1.4: Function changed to get version.
#define CMD_GET_VERSION   0x00 // V15.7: 
#define CMD_SET_DDR    0x01 // V1.4: 
#define CMD_GET_PIN    0x02 // V1.4: 
#define CMD_GET_PORT   0x03 // V1.4: 
#define CMD_SET_PORT   0x04 // V1.4: 

#define CMD_REBOOT    0x0F // V1.4: 
//								0x10	// V1.4: 
#define CMD_READ_EEPROM   0x11 // V1.4: Not implemented

#define CMD_SET_IO    0x15
#define CMD_GET_IO    0x16
#define CMD_SET_FILTER   0x17
#define CMD_SET_RX_BAND_FILTER 0x18 // V15.12
#define CMD_GET_RX_BAND_FILTER 0x19 // V15.12
#define CMD_SET_TX_BAND_FILTER 0x1a // MOBO Only
#define CMD_GET_TX_BAND_FILTER 0x1b // MOBO Only

#define CMD_SET_SI570   0x20 // Write byte to Si570 register

//Stop States
#define STOP_NORMAL 0
#define STOP_PROFICIO 1
#define STOP_MFC 2
#define STOP_SHUTDOWN 3
#define STOP_PROFICIO_COMMS 4
#define STOP_LOGFILE_FAILED 5
#define STOP_PROFICIO_SERIAL_NUMBER 6
#define STOP_MASTER_CONTROLLER 7
#define STOP_PROFICIO_VERSION 8
#define STOP_THREADS_START_FAILED 9
#define STOP_NETWORK_FAILED 10

//System Functions
#define DIGITAL_AUDIO 0
#define OPERATOR_AUDIO 1
#define REMOTE_AUDIO 2   /* Phones + REMOTE AUDIO checkbox → MSA1 mic */
#define KEYBOARD_DISPLAY 2
#define KEYBOARD_STOP 3
#define KEYBOARD_DISPLAY_NUMPAD 4
#define AUTO_START_ON 5
#define AUTO_START_OFF 6

#define IQ_B160  9
#define IQ_B80 8 
#define IQ_B60 7
#define IQ_B40 6
#define IQ_B30 5
#define IQ_B20 4
#define IQ_B17 3
#define IQ_B15 2
#define IQ_B12 1
#define IQ_B10 0


#define MAX_PCB_VERSION 5
#define TX_ON 1
#define TX_OFF 0
#define SI570_DLL 0
#define SI5351_DLL 1
#define FREQ_OUT_OF_RANGE 10000
#define GENERAL_BAND 200

#define MODE_AM 0
#define MODE_LSB 1
#define MODE_USB 2
#define MODE_CW 3
#define MODE_TUNE 4
#define MODE_AM_LETTER ('A')
#define MODE_LSB_LETTER ('L')
#define MODE_USB_LETTER ('U')
#define MODE_CW_LETTER ('C')
#define MODE_TUNE_LETTER ('T')

#define HDSDR_STATUS_TRANSMIT_ON 1
#define HDSDR_STATUS_STOP_MODE 2
#define HDSDR_STATUS_START_MODE 3
#define HDSDR_STATUS_WRONG_MODULATION_MODE 4
#define HDSDR_STATUS_TX_MODE 5
#define HDSDR_STATUS_RX_MODE 6
#define HDSDR_STATUS_NO_BAND 7
#define HDSDR_STATUS_NO_POWER 8



#define CMD_GET_POTENTIA_POWER 0x05
#define CMD_GET_POTENTIA_TEMPERATURE 0x06
#define CMD_GET_POTENTIA_BIAS 0x07
#define CMD_SET_POTENTIA_CALIBRATION 0x08
#define CMD_SET_SDRCORE_TRANS_INITIALIZE 0x09
//#define CMD_SET_BIAS_STATUS 0x0A
#define CMD_SET_TRANS_SMETER 0x0C
#define CMD_SET_MONITOR 0x0D
#define CMD_SET_VOLUME_BY_SERVER 0x0E
#define CMD_SET_AMPLIFIER_CALIBRATION_RESET 0x10
#define CMD_SET_METER_HILO 0x0A

#define CMD_SET_SPECTRUM_VIEW_GRID 0x21	
#define CMD_SET_SPECTRUM_SHARP	0x22	
#define CMD_SET_ALC_MULTIPLIER 0x23
#define CMD_SET_CONFIGURATION_COMMAND 0x24

#define CMD_SET_FREQ_REG  0x30
#define CMD_SET_SPLIT   0x31
#define CMD_SET_FREQ   0x32
#define CMD_SET_XTAL_INT  0x33
#define CMD_SET_SPLIT_RX_FREQ   0x34
#define CMD_SET_BIAS 0x35
#define CMD_SET_ANTENNA_SENSE 0x36
#define CMD_SET_SMETER_AVERAGE 0x37
#define CMD_SET_FOWARD_POWER_VALUE 0x38
#define CMD_SET_SPLIT_TX_FREQ   0x39
#define CMD_GET_FREQ   0x3a
#define CMD_SET_XTAL_DEC  0x3a
#define CMD_SET_X_POSITION    0x3b
#define CMD_GET_STARTUP   0x3c
#define CMD_GET_XTAL   0x3d
#define CMD_SET_Y_POSITION 0x3E //For testing
#define CMD_GET_SI570   0x3f

//For async port
#define ID0 0
#define FA 1
#define AI 2
#define MD 3
#define MD2 4
#define IF 5
#define TX 6
#define RX 7
#define AI0 8
#define FB 9
#define PS 10
#define FW 11
#define KS 12
#define RM 13
#define NO_COMMAND_FOUND 100

#define CMD_GET_SET_COMM_PORT 0x40
#define CMD_GET_SET_COMM_BAUD_RATE 0x41 
#define CMD_GET_SET_COMM_PARITY 0x42
#define CMD_GET_SET_COMM_DATA_BITS 0x43
#define CMD_GET_SET_COMM_STOP_BITS 0x44
#define CMD_GET_SET_COMM_START 0x45
#define CMD_GET_SET_COMM_NAME_INDEX 0x46
#define CMD_DELETE_COMM_PORT_INIT 0x47
#define CMD_GET_SET_COMM_PORT_PINS 0x48

//For HR50 async port
#define CMD_GET_SET_HR50_COMM_NAME_INDEX 0x49
#define CMD_GET_SET_HR50_COMM_PORT 0x4A
#define CMD_GET_SET_HR50_COMM_SEND_BAND_INFO 0x4B
#define CMD_GET_SET_HR50_COMM_START 0x4C
#define CMD_GET_SET_HR50_COMM_SEND_FREQ_INFO 0x4D
#define CMD_GET_SET_HR50_COMM_PASS_THRU 0x4E
#define CMD_SET_ALC 0x4F

#define CMD_SET_PTT    0x50
#define CMD_GET_CW_KEY   0x51
//IQ Balance Commands
#define CMD_SET_IQ_OFFSET 0x52
#define CMD_SET_SDR_CORE_BAND 0x53
#define IQ_CALIBRATION_TUNE 0x54
#define IQ_CALIBRATION_RX_TX 0x55
#define IQ_OPERATION_COMPLETE 0x56
#define CMD_SET_COMMIT_IQ 0x57
#define CMD_SET_IQ_BAND  0x58

//Panadapter Control
#define CMD_GET_SET_PANADAPTER_FILL 0x59
#define CMD_GET_SET_PANADAPTER_LINE 0x5A
#define CMD_GET_SET_PANADAPTER_MARKER 0x5B
#define CMD_GET_SET_PANADAPTER_AVERAGE 0x5C
#define CMD_GET_SET_PANADAPTER_BACKGROUND 0x5D
#define CMD_GET_SET_PANADAPTER_CURSOR 0x5E
#define CMD_GET_SET_PANADAPTER_REFRESH 0x5F

#define N_DECIMAL_POINTS_PRECISION (100)

#define CALIBRATION_INITIALIZED 4
#define CALIBRATION_RUNNING 2
#define CALIBRATION_COMPLETE 3
#define CALIBRATION_SUCCESS 1
#define CALIBRAITON_FAIL 0
#define MAX_CALIBRATION_ELEMENT 1200
#define CW_SNAP_CYCLE_COUNT 65536


//Calibration

#define CMD_SET_CALIBRATION_DATA 0x61
#define CMD_SET_CALIBRATION_FINISHED 0x62
#define CMD_SET_CAL_LOOSE 0x63
#define CMD_SET_CAL_DATA_PROCESSED 0x66
#define CMD_SET_CAL_SET_COARSE 0xC3
#define CMD_SET_CAL_SET_FINE 0xAE

#define CMD_SET_CAL_RESET 0x68
#define CMD_SET_CAL_MODE 0x69
#define CMD_SET_CALIBRATIION_PROGRESS 0x6A
#define CMD_GET_SET_CAL_FREQ_DELTA 0x6B
#define CDM_SET_CALIBRATE_CYCLE_COUNT 0x6F
#define CMD_SET_FORCE_CALIBRATION 0xC5

//CW Snap
#define CMD_SET_CALIBRATION_START 0x60
#define CMD_CW_SNAP_DATA_PROCESSED 0x64
#define CMD_CW_SNAP_SET_DELTA 0x65
#define CMD_SET_CW_SNAP_FREQ 0x67
#define CMD_CW_SNAP_START 0x6C
#define CMD_CW_SNAP_FINISHED 0x6D
#define CMD_CW_SNAP_SET_CALIBRATION_DATA 0x6E

//CW Controls
#define SET_CW_MODE 0x70
#define SET_KEYER_MODE 0x71
#define SET_QSK 0x72
#define SET_CW_PADDLE 0x73
#define SET_SPACING 0x75
/* Memory-play Farnsworth text WPM (was SET_MEMORY_TYPE).
 * Param: 0=off (standard spacing); 5–60=overall/text WPM for inter-char/word
 * gaps only on CQ memory play. Character elements stay on SET_WPM.
 * If text WPM >= char WPM, treat as off.
 * Proficio: USB + I2C forward. PIC: apply on memory play when implemented. */
#define SET_MEM_TEXT_WPM 0x76
#define SET_WEIGHT 0x77
#define SET_TX_HOLD 0x7A
#define SET_WPM 0x7B
#define SET_KEYER_INSTALLED 0x7D
#define SET_SIDE_TONE 0x7F
#define CMD_SET_TRANSCEIVER_CW_PITCH 0x90
/* PIC keyer CQ/memory (was unused GET_IAMBIC_TUNING).
 * Param: 0=play, 1=store begin, 2=store end, 3=select slot (next=0..3),
 *        0x20-0x7E=append ASCII. Sticky slot; 4 slots × 48 chars.
 * USB: vendor OUT 2 bytes [param,seq] per transfer; pace after each
 * (Proficio I2C drain; KEYER_MEM_USB_GAP_MS in Radio_send_parameters). */
#define CMD_SET_KEYER_MEMORY 0x9C
#define KEYER_MEM_PLAY          0
#define KEYER_MEM_STORE_BEGIN   1
#define KEYER_MEM_STORE_END     2
#define KEYER_MEM_SELECT        3
#define KEYER_MEM_MAX_CHARS     48
#define KEYER_MEM_USB_GAP_MS    40   /* ms after each 0x9C USB OUT (I2C drain) */
/* After STORE_END: PIC blocks on EEPROM (≈2ms×chars). Host must not push
 * PLAY (or more) until that finishes or Proficio I2C NACKs and drops PLAY. */
#define KEYER_MEM_END_SETTLE_MS 400

#define CMD_GET_SET_NB_ENABLE 0x80
#define CMD_GET_SET_NB_PULSE_WIDTH 0x81
#define CMD_GET_SET_NB_THRESHOLD 0x82
#define CMD_GET_SET_PANADAPTER_STATUS 0x83
#define CMD_GET_SET_SMETER_STATUS 0x84
#define CMD_GET_SET_PANADAPTER_GAIN 0x85
#define CMD_GET_SET_PANADAPTER_BASE 0x86
#define CMD_GET_SET_AGC 0x87
#define CMD_SET_TWO_TONE 0x88
#define CMD_SET_RIT_FREQ 0x89
#define CMD_SET_RIT_STATUS 0x8A
#define CMD_GET_IQ_VALUE 0x8B
#define CMD_SET_FREQ_CAL_CHECK 0x8C
#define CMD_SET_IQ_DEFAULTS 0x8D
#define CMD_GET_SET_AUTO_NOTCH 0x8E
#define CMD_SET_MICROPHONE_STATUS 0x8F




#define SET_DLL_VERSION 0xA0
#define CMD_SET_BAND_POWER_BAND 0xA1
#define CMD_SET_BAND_POWER_POWER 0xA2
#define CMD_GET_KEY_DOWN 0xA4
#define CMD_GET_PTT 0xA5
#define CMD_SET_RIG_TUNE 0xA6
#define CMD_START_CALIBRATE 0xA7
#define CMD_MODE_SET_BY_SERVER 0xA8
#define CMD_SET_TRANSVERTER 0xA9
#define CMD_SET_BAND_VOLUME_DEFAULTS 0xAA
#define CMD_CALIBRATION_MASTER_RESET 0xAB
#define CMD_CALIBRATION_TUNE 0xAC
#define CMD_SET_COMMIT_POWER_VALUES 0xAD
#define CMD_SET_STANDARD_CARRIER 0xAF


#define CMD_GET_FREQ_INIT 0xB0
#define CMD_SET_DISPLAY_MODE 0xB1
#define CMD_GET_SET_FIRMWARE_VERSION 0xB2
#define CMD_GET_SET_MSSDR_VERSION 0xB3
#define CMD_GET_BAND_POWER 0xB4 
#define CMD_GET_SET_SDRCORE_RECV_VERSION 0xB5
#define CMD_SET_MAIN_FREQ 0xB6
#define CMD_SET_MAIN_MODE 0xB7
#define CMD_GET_MODE_INIT 0xB8
#define CMD_GET_BAND_INIT 0xB9
#define CMD_SET_TX_ON 0xBA
#define CMD_SET_DISPLAY_FREQ 0xBB
#define CMD_SET_TX_SET_BY_SERVER 0xBC
#define CMD_GET_SET_SDRCORE_TRANS_VERSION 0xBD
#define CMD_GET_OPTIONS_STATUS 0xBE
#define CMD_GET_TRANSCEIVER_TEMP 0xBF


#define CMD_SET_S_METER 0xC0

#define CMD_SET_FAVS_UPDATE  0xC1
#define CMD_GET_FAVS 0xC2
#define CMD_SET_FAVS_DELETE 0xC4
#define CMD_SET_FAVS_NAME 0xC8
#define CMD_REPORT_IMAGE_VALUE 0xC6
#define CMD_START_STOP_IMAGE_VALUE 0xC7
#define CMD_SET_AUTO_SNAP_STATUS 0xC9
#define CMD_SET_AUTO_SNAP_INDEX 0xCA
#define CMD_SET_AGC_FAST_LEVEL 0xCB
#define CMD_SET_TRANSCEIVER_DISPLAY 0xCC
#define CMD_SET_STAR 0xCD
#define CMD_SET_STEP_VALUE 0xCE
#define CMD_SET_SMETER_DISPLAY 0xCF


#define CMD_SET_BW_LOCUT 0xD0
#define CMD_SET_BW_HICUT 0xD1
#define CMD_SET_CW_PITCH 0xD2
#define CMD_SET_TX_HICUT 0xD3
#define CMD_GET_SET_SMETER 0xD4
#define CMD_GET_SET_PANADAPTER 0xD5


#define CMD_GET_SET_LAST_USED_FREQ 0xD7
#define CMD_GET_SET_LAST_USED_MODE 0xD8
#define CMD_GET_SET_LAST_USED_BAND 0xD9
#define CMD_GET_SET_PANADAPTER_SMOOTHING 0xDA
#define CMD_SET_CW_BW 0xDB
#define CMD_SET_BW_LOCUT_DEFAULT 0xDC
#define CMD_SET_BW_HICUT_DEFAULT 0xDD
#define CMD_SET_CW_BW_DEFAULT 0xDE
#define CMD_SET_PANADAPTER_DISPLAY 0xDF


//Sound device management
#define UNMUTED 0
#define MUTED 1

#define CMD_SET_MIC_GAIN 0xE0
#define CMD_SET_DIGITAL_MIC_GAIN 0x93
#define CMD_SET_VOLUME_ATTN 0xE1
#define CMD_SET_DIGITAL_VOLUME_ATTN 0x96
#define CMD_SET_MAIN_POWER 0xE2
#define CMD_SET_AM_POWER 0xE3
#define CMD_SET_CW_POWER 0xE4
#define CMD_SET_SPEAKER_VOLUME 0xE5
#define CMD_SET_MIC_VOLUME 0xE6
#define CMD_SET_SPEAKER_MUTE 0xE7
#define CMD_SET_MIC_MUTE 0xE8
#define CMD_SET_TUNE_POWER 0xE9
#define CMD_GET_SET_SPEAKER_DEVICE 0xEA
#define CMD_GET_SET_MIC_DEVICE 0xEB
#define CMD_DELETE_SDRCORE_INIT 0xEC
#define CMD_SET_OVERDRIVEN 0xED
#define CMD_SET_COMPRESSION_STATE 0xEE
#define CMD_SET_COMPRESSION_LEVEL 0xEF
#define CMD_SET_PHONES_VOLUME_LEVEL 0x97
#define CMD_SET_PHONES_MIC_GAIN_LEVEL 0x98
#define CMD_SET_DIGITAL_VOLUME_LEVEL 0x99
#define CMD_SET_DIGITAL_MIC_GAIN_LEVEL 0x9A
#define CMD_SET_AUDIO_DEVICE 0x9B
//End Sound Device Management

#define CMD_SET_HDSDR_STATUS 0xF0
#define CMD_GET_HDSDR_STATUS 0xF1
#define CMD_SET_NR 0xA3
#define CMD_SET_VFO 0xF2
#define CMD_SET_PCB_VERSION 0xF3
#define CMD_SET_KEEP_ALIVE 0xF4
/* Payload (first data byte / int) for CMD_SET_KEEP_ALIVE — who sent it */
#define KEEP_ALIVE_FROM_CLIENT 1
#define KEEP_ALIVE_FROM_RECV   2
#define KEEP_ALIVE_FROM_TRANS  3
#define CMD_GET_SET_MSSDR_STATUS 0xF5
#define CMD_SET_BAND 0xF6
#define CMD_SET_PA_BYPASS 0xF7
#define CMD_SET_AMPLIFIER 0xF8
#define CMD_SET_AMPLIFIER_INITIALIZE 0xF9  
#define CMD_SET_AMPLIFIER_POWER 0xFA
#define CMD_GET_AMPLIFIER_POWER 0xFB
#define CMD_SET_FAN_CONTROL 0xFC
#define CMD_SET_FAN_ON_TEMPERATURE 0xFD
#define CMD_CHECK_GUI_STATUS 0xFE
#define CMD_SET_STOP 0xFF

//START EXTENDED COMMANDS
#define CMD_SET_EXTENDED_COMMAND 0x0B

#define CMD_SET_WATERFALL_DISPLAY 0x00
#define CMD_SET_WATERFALL_GAIN 0x01
#define CMD_SET_WATERFALL_GRID 0x02
#define CMD_SET_WATERFALL_ZERO 0x03
#define CMD_SET_WATERFALL_SPEED 0x04
#define CMD_SET_WATERFALL_DIRECTION 0x05
#define CMD_SET_WATERFALL_PALET 0x06
#define CMD_SET_ANTENNA_SWITCH 0x08
#define CMD_SET_IQBD_MONITOR 0x09
#define CMD_SET_IQBD_DATA 0x0A
#define CMD_SET_FORWARD_POWER 0x0B
#define CMD_SET_REVERSE_POWER 0x0C
#define CMD_SET_SWR 0x0D
#define CMD_SET_SOLIDUS_STATUS 0x0E
#define CMD_SET_FORTIS_DEVICES 0x32

//RPi Temperture
#define CMD_RPI_SET_TEMPERATURE 0x12

// MFC Function Codes
#define CMD_MFC_AUTO_ZERO 0x13
#define CMD_MFC_SET_ZERO 0x14
#define CMD_MFC_SET_BAND 0x15
#define CMD_MFC_SET_FAVS 0x16
#define CMD_MFC_SET_STEP 0x17
#define CMD_MFC_SET_TUNE 0x18
#define CMD_MFC_SET_MODE 0x19
#define CMD_MFC_SET_RIT_MODE 0x1A
#define CMD_MFC_SET_CW_BW 0x1B
#define CMD_MFC_SET_HI_BW 0x1C
#define CMD_MFC_SET_RIT 0x1D
#define CMD_SET_GUI_STAR 0x1E

#define CMD_SET_KNOB_SWITCH 0x20
#define CMD_SET_LEFT_SWITCH 0x21
#define CMD_SET_MIDDLE_SWITCH 0x22
#define CMD_SET_RIGHT_SWITCH 0x23
#define CMD_SET_PTT_SWITCH 0x24
#define CMD_SET_METER_HOLD 0x26
#define CMD_SET_RIT_STEP 0x27

#define CMD_SET_SERVER_ERROR 0x30
#define CMD_SET_SERVER_MSG 0x31
//End Extended Commands

// CW Message Management
#define CW_MESSAGE_SIZE 17

//Iambic key type
#define CFG_MODE_IAMBIC 0
#define CFG_MODE_ULTIMATIC 1
#define CFG_MODE_BUG 2
#define CFG_MODE_STRAIGHT 3

//Set Iambic mode on or off
#define CFG_IAMBIC_OFF 0
#define CFG_IAMBIC_ON 1

// Keying memory
#define CFG_MEMORY_TYPE_A 0
#define CFG_MEMORY_TYPE_DAH 1
#define CFG_MEMORY_TYPE_DIT 2
#define CFG_MEMORY_TYPE_B 3

// Keying spacing
#define CFG_SPACING_NONE 0
#define CFG_SPACING_EL 1
#define CFG_SPACING_CHAR 2
#define CFG_SPACING_WORD 3

// Paddle reversal
#define CFG_PADDLE_NORMAL 0
#define CFG_PADDLE_REVERSE 1

// Side Tone Frequency 
#define CFG_SIDE_TONE_FREQ_600 0
#define CFG_SIDE_TONE_FREQ_800 1


// Character dit/dah weight - Use with SET_WEIGHT - increment or decrement by 10 to change weight
#define CFG_WEIGHT_DIST 50

//Iambic speed tuning - allows fine tuning of the Iambic keying speed
#define CFG_IAMBIC_CALIBRATION_DEFAULT 120

//Amount of time the radio is held in TX mode - Use with SET_TX_HOLD. Set this to zero(0) for full QSK keying
#define CFG_TX_HOLD_DEFAULT 127

//Semi Break In on or off - Use with SET_SEMI_BREAKIN
#define CFG_SEMI_BREAKIN_OFF 0
#define CFG_SEMI_BREAKIN_ON 1

//Set all CW parameters to default (CW Mode is set to Off)   - Use with SET_CW_DEFAULTS
#define CFG_CW_DEFAULTS_OFF 0
#define CFG_CW_DEFAULTS_ON 1



