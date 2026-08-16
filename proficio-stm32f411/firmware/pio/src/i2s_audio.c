/**
 * I2S2 full-duplex to PCM3060.
 * Pins: PB12 WS, PB13 CK, PB15 SD (TX→DIN), PB14 ext SD (RX←DOUT), PC6 MCK
 *
 * Master transmit @ ~96 kHz 16-bit stereo. I2S2ext for full-duplex RX.
 * DMA circular into/out of current PCM3060 Tx/Rx buffers.
 */
#include "i2s_audio.h"
#include "pcm3060.h"
#include "sync_sof.h"
#include "board_pins.h"
#include <string.h>

I2S_HandleTypeDef hi2s2;
I2S_HandleTypeDef hi2s2ext;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_i2s2ext_rx;

static uint8_t s_running = 0;

/* Working DMA buffers — one frame; multi-buffer rotate via audio_buf_advance */
static uint16_t s_tx_dma[I2S_BUF_SIZE / 2u];
static uint16_t s_rx_dma[I2S_BUF_SIZE / 2u];

static void i2s_gpio_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PB12 WS, PB13 CK, PB15 SD — AF5 I2S2 */
    g.Pin = BOARD_I2S_LRCK_PIN | BOARD_I2S_BCK_PIN | BOARD_I2S_DIN_PIN;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &g);

    /* PB14 I2S2ext_SD — AF6 on F411 */
    g.Pin = BOARD_I2S_DOUT_PIN;
#ifdef GPIO_AF6_SPI2
    g.Alternate = GPIO_AF6_SPI2;
#else
    g.Alternate = GPIO_AF6_I2S2ext;
#endif
    HAL_GPIO_Init(GPIOB, &g);

    /* PC6 MCK — AF5 */
    g.Pin = BOARD_I2S_MCLK_PIN;
    g.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &g);
}

static void i2s_dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* SPI2_TX = DMA1 Stream4 Channel0 */
    hdma_spi2_tx.Instance = DMA1_Stream4;
    hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi2_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi2_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi2_tx);
    __HAL_LINKDMA(&hi2s2, hdmatx, hdma_spi2_tx);

    /* I2S2_EXT_RX = DMA1 Stream3 Channel3 */
    hdma_i2s2ext_rx.Instance = DMA1_Stream3;
    hdma_i2s2ext_rx.Init.Channel = DMA_CHANNEL_3;
    hdma_i2s2ext_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2s2ext_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2s2ext_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2s2ext_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_i2s2ext_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_i2s2ext_rx.Init.Mode = DMA_CIRCULAR;
    hdma_i2s2ext_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_i2s2ext_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2s2ext_rx);
    __HAL_LINKDMA(&hi2s2ext, hdmarx, hdma_i2s2ext_rx);

    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
}

void i2s_audio_init(void)
{
    __HAL_RCC_SPI2_CLK_ENABLE();

    i2s_gpio_init();

    /*
     * I2S master TX, Philips standard, 16-bit data on 16-bit frame.
     * AudioFreq 96k — PLLI2S configured below.
     */
    hi2s2.Instance = SPI2;
    hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_96K;
    hi2s2.Init.CPOL = I2S_CPOL_LOW;
    hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;

    /* PLLI2S for 96 kHz from 96 MHz SYSCLK path (HSE 25 MHz typical) */
    {
        RCC_PeriphCLKInitTypeDef p = {0};
        p.PeriphClockSelection = RCC_PERIPHCLK_I2S;
        p.PLLI2S.PLLI2SN = 192;
        p.PLLI2S.PLLI2SR = 2;
        (void)HAL_RCCEx_PeriphCLKConfig(&p);
    }

    if (HAL_I2S_Init(&hi2s2) != HAL_OK) {
        /* Leave unusable; Start will no-op */
        return;
    }

    /* Full duplex extension handle */
    hi2s2ext.Instance = I2S2ext;
    memcpy(&hi2s2ext.Init, &hi2s2.Init, sizeof(hi2s2.Init));
    hi2s2ext.Init.Mode = I2S_MODE_SLAVE_RX;

    i2s_dma_init();
    s_running = 0;
}

void i2s_audio_start(void)
{
    uint16_t n = (uint16_t)(I2S_BUF_SIZE / 2u);

    memcpy(s_tx_dma, PCM3060_TxBuf(), I2S_BUF_SIZE);
    memset(s_rx_dma, 0, sizeof(s_rx_dma));

    if (HAL_I2SEx_TransmitReceive_DMA(&hi2s2, s_tx_dma, s_rx_dma, n) != HAL_OK) {
        /* Fallback: TX-only if full duplex helper unavailable */
        (void)HAL_I2S_Transmit_DMA(&hi2s2, s_tx_dma, n);
    }
    s_running = 1;
}

void i2s_audio_stop(void)
{
    (void)HAL_I2S_DMAStop(&hi2s2);
    s_running = 0;
}

uint8_t i2s_audio_running(void)
{
    return s_running;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != SPI2) {
        return;
    }
    /* Reload TX from current USB buffer; copy RX into host buffer */
    memcpy(PCM3060_RxBuf(), s_rx_dma, I2S_BUF_SIZE);
    memcpy(s_tx_dma, PCM3060_TxBuf(), I2S_BUF_SIZE);
    audio_buf_advance();
    sync_sof_on_i2s_frame(); /* one buffer ≈ 1 ms @ 96 kHz */
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    (void)hi2s;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    (void)hi2s;
}

void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2s2ext_rx);
}

void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

void i2s_audio_on_tx_half(void) {}
void i2s_audio_on_tx_cplt(void) {}
void i2s_audio_on_rx_half(void) {}
void i2s_audio_on_rx_cplt(void) {}
