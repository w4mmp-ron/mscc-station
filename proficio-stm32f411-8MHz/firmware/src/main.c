/**
 * Proficio STM32F411 — scaffold entry.
 * Not linked to a full startup file yet; structure for Phase 0 bring-up.
 */
#include "proficio_config.h"
#include "board.h"
#include "app.h"

int main(void)
{
    board_init();
    app_init();

    for (;;) {
        app_poll();
        board_led_toggle();
        board_delay_ms(500);
    }
}
