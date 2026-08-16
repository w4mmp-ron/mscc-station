/**
 * Enter STM32 system (ROM) bootloader for STM32CubeProgrammer / DFU / ST-Link.
 * Same *concept* as PSoC BOOT jumper + bootload tool; tools are ST's off-the-shelf.
 */
#ifndef SYSTEM_BOOT_H
#define SYSTEM_BOOT_H

#include <stdint.h>

/**
 * Jump immediately into ROM system memory bootloader (USB DFU / USART, etc.).
 * Does not return.
 */
void system_boot_jump(void) __attribute__((noreturn));

/** Request jump soon (from USB handler); polled in main loop. */
void system_boot_request_reset(void);
uint8_t system_boot_pending(void);

/**
 * Early check: mother-board BOOT pin low (PSoC-style jumper).
 * Call after board_init(), before USB start.
 */
uint8_t system_boot_check_and_enter(void);

#endif /* SYSTEM_BOOT_H */
