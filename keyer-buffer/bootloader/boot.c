/*
 * Keyer PIC16F18326 I2C bootloader, version 2 (2026-11-12). Build: build.bat
 * (version 1: whole row in one 66-byte write; 2: 16-byte pieces, each I2C transfer ~2 ms)
 *
 * Flash: 0x0000-0x03FF bootloader, 0x0400-0x3FFF keyer app (built with code offset 0x400).
 * Vectors: reset 0x0000 -> here; interrupt 0x0004 -> 0x0404 (app ISR).
 * I2C slave address 0x5C (app stays on 0x40). No interrupts used here.
 *
 * After reset: run the app unless
 *   - EEPROM 0xFF == 0x5C   (app got opcode 0x9F 0x5C, or an update was cut short), or
 *   - flash 0x400 == 0x3FFF (app missing / erased; the host writes row 0x400 last).
 * EEPROM 0x00-0xFE (keyer settings, CQ memories) is never touched.
 *
 * Commands = one I2C write each, carried out at STOP:
 *   01 aH aL          set word address
 *   05 off d0..d15    put 16 bytes in the row buffer at byte offset off (0, 16, 32, 48)
 *   02 sum            erase + write the 32-word row at address from the row buffer (lo byte
 *                     first), check, address += 32. sum = 8-bit sum of the 64 buffer bytes.
 *                     Row must be 32-aligned, 0x400..0x3FE0.
 *   03                next reads return flash bytes from address (lo, hi, ...), address advances
 *                     (carries on across reads, so 16 bytes per read is fine)
 *   04 A5             leave: clear EEPROM flag, reset (runs the app if 0x400 is written)
 * Reads (when not after 03): status, version, aH, aL
 *   status 00 OK, 01 bad address, 02 bad sum, 03 bad length, 04 read-back differs, 05 unknown cmd
 * A row write takes ~5 ms (CPU stalls, SCL held); the host waits ~10 ms before reading status.
 */
#include <xc.h>
#include <stdint.h>

#pragma config FEXTOSC = OFF, RSTOSC = HFINT32, CLKOUTEN = OFF, CSWEN = ON, FCMEN = ON
#pragma config MCLRE = ON, PWRTE = OFF, WDTE = OFF, LPBOREN = OFF, BOREN = ON, BORV = LOW
#pragma config PPS1WAY = ON, STVREN = ON, DEBUG = OFF
#pragma config WRT = OFF, LVP = ON
#pragma config CP = OFF, CPD = OFF

#define BOOT_I2C_ADDR   0x5C
#define BOOT_VERSION    0x02
#define APP_START       0x0400
#define FLASH_END       0x4000
#define ROW_WORDS       32
#define EE_FLAG_ADDR    0xF0FF      /* EEPROM byte 0xFF, NVMREGS space */
#define EE_FLAG_STAY    0x5C

#define ST_OK       0x00
#define ST_BAD_ADDR 0x01
#define ST_BAD_SUM  0x02
#define ST_BAD_LEN  0x03
#define ST_VERIFY   0x04
#define ST_BAD_CMD  0x05

/* Interrupt vector: bare jump to the app's ISR at 0x404. Not a C __interrupt function:
 * XC8 would add "bsf int$flags" (a RAM write) before the jump. Bootloader never enables GIE. */
asm("psect intredirect,class=CODE,delta=2,abs,ovrld");
asm("org 0x0004");
asm("movlp 0x04");
asm("goto 0x404 & 0x7FF");

#define PIECE           16
static uint8_t row[ROW_WORDS * 2];
static uint8_t rx[2 + PIECE];
static uint8_t rx_len;
static uint8_t rx_over;
static uint16_t addr;
static uint8_t status;
static uint8_t read_flash;   /* 1 after cmd 03 */
static uint8_t tx_idx;

static void nvm_unlock(void)
{
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    NOP();
    NOP();
}

static uint16_t flash_read(uint16_t a)
{
    NVMCON1bits.NVMREGS = 0;
    NVMADRH = (uint8_t)(a >> 8);
    NVMADRL = (uint8_t)a;
    NVMCON1bits.RD = 1;
    NOP();
    NOP();
    return ((uint16_t)NVMDATH << 8) | NVMDATL;
}

static uint8_t ee_read(uint16_t a)
{
    NVMCON1bits.NVMREGS = 1;
    NVMADRH = (uint8_t)(a >> 8);
    NVMADRL = (uint8_t)a;
    NVMCON1bits.RD = 1;
    NOP();
    NOP();
    return NVMDATL;
}

static void ee_write(uint16_t a, uint8_t v)
{
    NVMCON1bits.NVMREGS = 1;
    NVMADRH = (uint8_t)(a >> 8);
    NVMADRL = (uint8_t)a;
    NVMDATL = v;
    NVMCON1bits.FREE = 0;
    NVMCON1bits.LWLO = 0;
    NVMCON1bits.WREN = 1;
    nvm_unlock();
    while (NVMCON1bits.WR) { }      /* only used on the way out */
    NVMCON1bits.WREN = 0;
}

/* 05 off d0..d15: copy a piece into the row buffer; returns status. */
static uint8_t row_load(void)
{
    uint8_t i;
    uint8_t off = rx[1];

    if (rx_len != 2 + PIECE)
        return ST_BAD_LEN;
    if ((off & (PIECE - 1)) || off >= ROW_WORDS * 2)
        return ST_BAD_ADDR;
    for (i = 0; i < PIECE; i++)
        row[off + i] = rx[2 + i];
    return ST_OK;
}

/* Erase + write the row at addr from row[]; returns status. */
static uint8_t row_write(void)
{
    uint8_t i;
    uint8_t sum = 0;
    uint16_t a;

    if (rx_len != 2)
        return ST_BAD_LEN;
    if ((addr & (ROW_WORDS - 1)) || addr < APP_START || addr >= FLASH_END)
        return ST_BAD_ADDR;
    for (i = 0; i < ROW_WORDS * 2; i++)
        sum += row[i];
    if (sum != rx[1])
        return ST_BAD_SUM;

    /* erase */
    NVMCON1bits.NVMREGS = 0;
    NVMADRH = (uint8_t)(addr >> 8);
    NVMADRL = (uint8_t)addr;
    NVMCON1bits.FREE = 1;
    NVMCON1bits.WREN = 1;
    nvm_unlock();
    NVMCON1bits.FREE = 0;

    /* load 31 latches, the 32nd starts the write */
    NVMCON1bits.LWLO = 1;
    a = addr;
    for (i = 0; i < ROW_WORDS; i++) {
        NVMADRH = (uint8_t)(a >> 8);
        NVMADRL = (uint8_t)a;
        NVMDATL = row[2 * i];
        NVMDATH = row[2 * i + 1];
        if (i == ROW_WORDS - 1)
            NVMCON1bits.LWLO = 0;
        nvm_unlock();
        a++;
    }
    NVMCON1bits.WREN = 0;

    /* check */
    a = addr;
    for (i = 0; i < ROW_WORDS; i++) {
        uint16_t w = ((uint16_t)(row[2 * i + 1] & 0x3F) << 8) | row[2 * i];
        if (flash_read(a) != w)
            return ST_VERIFY;
        a++;
    }
    addr += ROW_WORDS;
    return ST_OK;
}

static void command(void)
{
    read_flash = 0;
    switch (rx[0]) {
    case 0x01:
        if (rx_len != 3) { status = ST_BAD_LEN; break; }
        addr = ((uint16_t)rx[1] << 8) | rx[2];
        status = ST_OK;
        break;
    case 0x02:
        status = row_write();
        break;
    case 0x05:
        status = row_load();
        break;
    case 0x03:
        read_flash = 1;
        tx_idx = 0;
        status = ST_OK;
        break;
    case 0x04:
        if (rx_len != 2 || rx[1] != 0xA5) { status = ST_BAD_LEN; break; }
        ee_write(EE_FLAG_ADDR, 0xFF);
        RESET();
        break;
    default:
        status = ST_BAD_CMD;
        break;
    }
}

static uint8_t next_tx(void)
{
    uint8_t b;
    if (read_flash) {
        uint16_t w = flash_read(addr);
        if (tx_idx & 1) {
            b = (uint8_t)(w >> 8);
            addr++;
        } else {
            b = (uint8_t)w;
        }
        tx_idx++;
        return b;
    }
    switch (tx_idx++ & 3) {
    case 0:  return status;
    case 1:  return BOOT_VERSION;
    case 2:  return (uint8_t)(addr >> 8);
    default: return (uint8_t)addr;
    }
}

/* Same idle pin state as the keyer app (key lines off, sidetone off), plus MSSP1 on RC0/RC1. */
static void boot_init(void)
{
    LATA = 0x00;
    LATC = 0x08;
    TRISA = 0x30;
    TRISC = 0x03;
    ANSELA = 0x00;
    ANSELC = 0x00;
    WPUA = 0x30;
    WPUC = 0x00;
    RC0PPS = 0x18;      /* SCL1 out */
    SSP1CLKPPS = 0x10;  /* SCL1 in  = RC0 */
    RC1PPS = 0x19;      /* SDA1 out */
    SSP1DATPPS = 0x11;  /* SDA1 in  = RC1 */

    SSP1STAT = 0x00;
    SSP1ADD = (uint8_t)(BOOT_I2C_ADDR << 1);
    SSP1MSK = 0xFE;
    SSP1CON2 = 0x01;    /* SEN: clock stretching */
    SSP1CON1 = 0x26;    /* SSPEN, 7-bit slave */
    PIR1bits.SSP1IF = 0;
}

void main(void)
{
    uint8_t pending = 0;

    if (ee_read(EE_FLAG_ADDR) != EE_FLAG_STAY && flash_read(APP_START) != 0x3FFF) {
        asm("pagesel 0x400");
        asm("goto 0x400");
    }

    boot_init();
    for (;;) {
        if (PIR1bits.SSP1IF) {
            PIR1bits.SSP1IF = 0;
            if (SSP1CON1bits.SSPOV) {
                SSP1CON1bits.SSPOV = 0;
                rx_over = 1;
            }
            if (!SSP1STATbits.D_nA) {               /* address byte */
                (void)SSP1BUF;
                if (SSP1STATbits.R_nW) {
                    if (!read_flash)
                        tx_idx = 0;
                    SSP1BUF = next_tx();
                } else {
                    rx_len = 0;
                    rx_over = 0;
                    pending = 1;
                }
            } else if (SSP1STATbits.R_nW) {         /* master reading */
                if (!SSP1CON2bits.ACKSTAT)
                    SSP1BUF = next_tx();
            } else {                                /* master writing */
                uint8_t b = SSP1BUF;
                if (rx_len < sizeof rx)
                    rx[rx_len++] = b;
                else
                    rx_over = 1;
            }
            SSP1CON1bits.CKP = 1;
        }
        if (pending && SSP1STATbits.P) {
            pending = 0;
            if (rx_len == 0)
                ;
            else if (rx_over)
                status = ST_BAD_LEN;
            else
                command();
        }
    }
}
