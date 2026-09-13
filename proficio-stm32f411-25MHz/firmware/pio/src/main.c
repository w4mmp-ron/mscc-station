/**
 * Proficio STM32F411 entry.
 * Linked at 0x08008000 (32KB flash DFU bootloader occupies 0x08000000).
 * BOOT / DFU: flash bootloader (KEY/PA0); see release/BLACKPILL-DFU-BOOTLOADER-README.md
 */
#include "proficio_config.h"
#include "board.h"
#include "app.h"
#include "system_boot.h"
#include "usb_device.h"
#include "stm32f4xx.h"

#define APP_VECTOR_TABLE 0x08008000u

int main(void)
{
    /* Bootloader may have set this; enforce for standalone debug too */
    SCB->VTOR = APP_VECTOR_TABLE;
    __DSB();
    __ISB();

    /*
     * Optional: PA8 / 0xFE → ST ROM DFU (unreliable on 25 MHz; prefer KEY+flash BL).
     */
    (void)system_boot_check_and_enter();

    board_init();
    app_init();

    for (;;) {
        if (system_boot_pending()) {
            board_delay_ms(50);
            system_boot_reset_into_dfu();
        }

        app_poll();
        {
            static uint32_t last = 0;
            uint32_t now = board_millis();
            uint32_t period = usb_device_configured() ? 50u : 500u;
            if ((now - last) >= period) {
                last = now;
                board_led_toggle();
            }
        }
    }
}
