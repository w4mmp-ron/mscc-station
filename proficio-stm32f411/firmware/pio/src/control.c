/**
 * Control / status HAL — PSoC Control/Status model on Black Pill GPIO.
 *
 * Yellow mother-board nets (Creator pins.jpg / pins2.jpg):
 *   Out: BS0-2, AMP, RX, LED; I2S DIN/BCK/LRCK/SCK (AF later)
 *   In:  KEY_0, KEY_1, PTT, BOOT
 *   I2C: SDA, SCL
 *   I2S data in: DOUT from PCM3060
 *
 * CONTROL_DIN / CONTROL_DOUT bits: software shadow (fabric gates on PSoC;
 * not separate yellow I2S data pins).
 */
#include "control.h"
#include "board_pins.h"
#include "radio_state.h"
#include "proficio_config.h"

static uint8_t s_control = CONTROL_RX | CONTROL_AMP | CONTROL_DOUT;
static volatile uint32_t s_hold_until_ms = 0;
static volatile uint8_t  s_hold_running = 0;

extern uint32_t board_millis(void);

static void apply_control_gpio(uint8_t v)
{
    /* Active-low RX / AMP (PSoC Control bits) */
    HAL_GPIO_WritePin(BOARD_RX_GPIO, BOARD_RX_PIN,
                      (v & CONTROL_RX) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BOARD_AMP_GPIO, BOARD_AMP_PIN,
                      (v & CONTROL_AMP) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* CONTROL_DIN / CONTROL_DOUT: shadow only (no yellow pins) */
    (void)CONTROL_DIN;
    (void)CONTROL_DOUT;

    /* LED1: Control bit0 → on-module PC13 active-low */
    if (v & CONTROL_LED) {
        HAL_GPIO_WritePin(BOARD_LED_GPIO, BOARD_LED_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(BOARD_LED_GPIO, BOARD_LED_PIN, GPIO_PIN_SET);
    }
}

void control_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    /* LED */
    g.Pin = BOARD_LED_PIN;
    HAL_GPIO_Init(BOARD_LED_GPIO, &g);

    /* Outputs: RX, BS0 */
    g.Pin = BOARD_RX_PIN | BOARD_BS0_PIN;
    HAL_GPIO_Init(GPIOA, &g);

    /* Outputs: AMP, BS1, BS2 */
    g.Pin = BOARD_AMP_PIN | BOARD_BS1_PIN | BOARD_BS2_PIN;
    HAL_GPIO_Init(GPIOB, &g);

    /* Inputs: KEY_0, KEY_1 (pull-up, open = high = key up) */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin = BOARD_KEY0_PIN | BOARD_KEY1_PIN;
    HAL_GPIO_Init(GPIOB, &g);

    /* Inputs: PTT, BOOT (pull-up; BOOT low = enter ROM bootloader) */
    g.Pull = GPIO_PULLUP;
    g.Pin = BOARD_PTT_PIN | BOARD_BOOT_PIN;
    HAL_GPIO_Init(GPIOA, &g);

    /* Optional VBUS */
    g.Pull = GPIO_NOPULL;
    g.Pin = BOARD_VBUS_SENSE_PIN;
    HAL_GPIO_Init(GPIOA, &g);

    /* I2S pins configured by i2s_audio_init() when audio feature enabled */

    s_control = CONTROL_RX | CONTROL_AMP | CONTROL_DOUT;
    apply_control_gpio(s_control);
    Band_Control_Write(CONTROL_BAND_20_30);
}

uint8_t Control_Read(void)
{
    return s_control;
}

void Control_Write(uint8_t value)
{
    s_control = value;
    apply_control_gpio(s_control);
}

uint8_t Status_Read(void)
{
    uint8_t s = 0;
    GPIO_PinState ptt_pin;

    /* Keys: high = open = bit set (PSoC STATUS_KEY_*) */
    if (HAL_GPIO_ReadPin(BOARD_KEY0_GPIO, BOARD_KEY0_PIN) == GPIO_PIN_SET) {
        s |= STATUS_KEY_0;
    }
    if (HAL_GPIO_ReadPin(BOARD_KEY1_GPIO, BOARD_KEY1_PIN) == GPIO_PIN_SET) {
        s |= STATUS_KEY_1;
    }

    /* BOOT yellow pin → STATUS_BOOT */
    if (HAL_GPIO_ReadPin(BOARD_BOOT_GPIO, BOARD_BOOT_PIN) == GPIO_PIN_SET) {
        s |= STATUS_BOOT;
    }

    /*
     * PTT yellow input. PSoC: pin → hardware inverter → Status.
     * Active-low footswitch: pin low ⇒ PTT active after invert-equivalent.
     */
    ptt_pin = HAL_GPIO_ReadPin(BOARD_PTT_GPIO, BOARD_PTT_PIN);
#if BOARD_PTT_ACTIVE_LOW
    if (ptt_pin == GPIO_PIN_RESET) {
        s |= STATUS_PTT;
        E_PTT = TRUE;
    } else {
        E_PTT = FALSE;
    }
#else
    if (ptt_pin == GPIO_PIN_SET) {
        s |= STATUS_PTT;
        E_PTT = TRUE;
    } else {
        E_PTT = FALSE;
    }
#endif

    extern uint8_t g_status_beat;
    if (g_status_beat) {
        s |= STATUS_BEAT;
    }

    return s;
}

void Band_Control_Write(uint8_t band_code)
{
    HAL_GPIO_WritePin(BOARD_BS0_GPIO, BOARD_BS0_PIN,
                      (band_code & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BOARD_BS1_GPIO, BOARD_BS1_PIN,
                      (band_code & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BOARD_BS2_GPIO, BOARD_BS2_PIN,
                      (band_code & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void cw_hold_start_ms(uint32_t ms)
{
    s_hold_until_ms = board_millis() + ms;
    s_hold_running = 1;
    E_cw_hold = TRUE;
}

void cw_hold_poll(void)
{
    if (!s_hold_running) {
        return;
    }
    if ((int32_t)(board_millis() - s_hold_until_ms) >= 0) {
        s_hold_running = 0;
        E_cw_hold = FALSE;
    }
}

uint8_t cw_hold_active(void)
{
    return s_hold_running;
}

void cw_hold_force_expired(void)
{
    s_hold_running = 0;
    E_cw_hold = FALSE;
}

uint8_t g_status_beat = 0;
