/**
 * Enter STM32 system (ROM) bootloader for DFU (dfu-util / CubeProgrammer).
 *
 * Reliable path: arm magic + NVIC_SystemReset(), then jump early on the
 * next boot (HSE up, USB not started). Do not live-jump from a running
 * USB session.
 */
#ifndef SYSTEM_BOOT_H
#define SYSTEM_BOOT_H

#include <stdint.h>

/**
 * Jump into ROM system memory bootloader. Does not return.
 * Call only with HSE already running and before USB init (or after reset).
 * Does not call HAL_RCC_DeInit() — ROM DFU needs HSE.
 */
void system_boot_jump(void) __attribute__((noreturn));

/**
 * From USB 0xFE: finish EP0, then arm DFU magic and SystemReset.
 * Polled from main via system_boot_pending().
 */
void system_boot_request_reset(void);
uint8_t system_boot_pending(void);

/** Perform armed reset into DFU (writes magic, NVIC_SystemReset). */
void system_boot_reset_into_dfu(void) __attribute__((noreturn));

/**
 * Early check after board_init(), before USB/app:
 *  - DFU magic from prior 0xFE reset, or
 *  - mother-board BOOT (PA8) held low
 * Either → jump to ROM bootloader.
 */
uint8_t system_boot_check_and_enter(void);

#endif /* SYSTEM_BOOT_H */
