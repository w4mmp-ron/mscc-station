/**
 * TinyUSB config — Proficio Black Pill F411
 * Vendor (Proficio opcodes) + UAC2 stereo 96 kHz 16-bit duplex.
 */
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define BOARD_TUD_RHPORT      0
#define BOARD_TUD_MAX_SPEED   OPT_MODE_FULL_SPEED

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined (OPT_MCU_STM32F4)
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#define CFG_TUD_ENABLED       1
#define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__((aligned(4)))
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif

#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          1
#define CFG_TUD_AUDIO           1

#define CFG_TUD_VENDOR_RX_BUFSIZE  64
#define CFG_TUD_VENDOR_TX_BUFSIZE  64

/* ---- UAC2: stereo 96 kHz 16-bit duplex (Proficio IQ) ---- */
#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS          1
#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE    96000
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX      2
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX      2

#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX  2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX          16
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX  2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX          16

#define CFG_TUD_AUDIO_ENABLE_EP_IN               1
#define CFG_TUD_AUDIO_ENABLE_EP_OUT              1
#define CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP        0

#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN \
  TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE, \
                    CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX, \
                    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)

#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT \
  TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE, \
                    CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX, \
                    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)

#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX        CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX       CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ     (CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX * 2)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ    (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX * 2)

#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT            2
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ         64

/* Interface numbers / descriptor length (no tusb.h include — avoid recursion) */
#include "usb_descriptors.h"
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN            TUD_AUDIO_PROFICIO_DESC_LEN

#endif /* TUSB_CONFIG_H */
