/**
 * PCM3060 I2C control + triple audio buffers (PSoC pcm3060.c port).
 */
#include "pcm3060.h"
#include "board.h"
#include "board_pins.h"
#include "i2c_bus.h"
#include "i2s_audio.h"
#include <string.h>

/* U2/J5 A28 RESET → PA2: hold PCM3060 in reset, then release (active-low). */
static void pcm3060_hw_reset_pulse(void)
{
    HAL_GPIO_WritePin(BOARD_CODEC_RESET_GPIO, BOARD_CODEC_RESET_PIN, GPIO_PIN_RESET);
    board_delay_ms(2);
    HAL_GPIO_WritePin(BOARD_CODEC_RESET_GPIO, BOARD_CODEC_RESET_PIN, GPIO_PIN_SET);
    board_delay_ms(5);
}

uint8_t RxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE];
uint8_t TxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE];

static volatile uint8_t s_buf_idx = 0;
static uint8_t *s_tx_override = 0;

uint8_t audio_buf_index(void)
{
    return s_buf_idx;
}

void audio_buf_advance(void)
{
    s_buf_idx = (uint8_t)((s_buf_idx + 1u) % USB_AUDIO_BUFS);
}

uint8_t *PCM3060_TxBuf(void)
{
    if (s_tx_override) {
        return s_tx_override;
    }
    return TxI2S[s_buf_idx];
}

uint8_t *PCM3060_RxBuf(void)
{
    return RxI2S[s_buf_idx];
}

void PCM3060_SetTxBufAddress(uint8_t *src)
{
    s_tx_override = src;
}

void PCM3060_SetTxBufAddressDefault(void)
{
    s_tx_override = 0;
}

uint8_t PCM3060_SetRegister(uint8_t reg, uint8_t val)
{
    uint8_t attempt;
    for (attempt = 0; attempt < 8u; attempt++) {
        if (i2c_write_reg(PCM3060_I2C_ADDR7, reg, val)) {
            return 0; /* success like PSoC (0 = ok) */
        }
    }
    return 1;
}

uint8_t PCM3060_Init(void)
{
    memset(RxI2S, 0, sizeof(RxI2S));
    memset(TxI2S, 0, sizeof(TxI2S));
    s_buf_idx = 0;
    s_tx_override = 0;
    pcm3060_hw_reset_pulse();
    i2s_audio_init();
    return PCM3060_Stop();
}

void PCM3060_Start(void)
{
    (void)PCM3060_SetRegister(0x40, 0xC0); /* Wakeup */
    (void)PCM3060_SetRegister(0x45, 0x80); /* Slow rolloff */
    (void)PCM3060_SetRegister(0x41, 0xFF); /* Volume L full */
    (void)PCM3060_SetRegister(0x42, 0xFF); /* Volume R full */
    i2s_audio_start();
}

uint8_t PCM3060_Stop(void)
{
    uint8_t ret = PCM3060_SetRegister(0x40, 0xF0); /* Sleep */
    i2s_audio_stop();
    return ret;
}

void PCM3060_Adj_Output_Volume(uint8_t level)
{
    /* PSoC used band volume tables; simple dual-channel set here */
    (void)PCM3060_SetRegister(0x41, level);
    (void)PCM3060_SetRegister(0x42, level);
}
