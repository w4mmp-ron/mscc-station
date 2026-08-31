/**
 * Proficio STM32F411 entry.
 * BOOT pin / system bootloader: see system_boot.c + docs/BOOTLOADER.md
 */
#include "proficio_config.h"
#include "board.h"
#include "app.h"
#include "system_boot.h"
#include "usb_device.h"

int main(void)
{
    board_init();

    /* PSoC-style: BOOT jumper low → STM32 ROM bootloader (CubeProgrammer) */
    (void)system_boot_check_and_enter();

    app_init();

    for (;;) {
        if (system_boot_pending()) {
            /* After USB CMD 0xFE — give host a moment, then DFU */
            board_delay_ms(50);
            system_boot_jump();
        }

        app_poll();
        {
            static uint32_t last = 0;
            uint32_t now = board_millis();
            /* Fast blink when USB configured (enumerated); slow otherwise. */
            uint32_t period = usb_device_configured() ? 50u : 500u;
            if ((now - last) >= period) {
                last = now;
                board_led_toggle();
            }
        }
    }
}
