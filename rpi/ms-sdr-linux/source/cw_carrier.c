/*
 * cw-carrier (2026-11-20, separate cw-carrier repo): the radio (Black Pill 3.90+) plays a
 * stored 1 ms CW carrier block, so sdrcore-trans need not stream to the radio while
 * receiving or in CW, and sdrcore-recv stops receive during voice / digital transmit.
 * For a Pi Zero 2 W, whose USB loses receive when both streams run.
 *
 * mscc.ini CW_CARRIER=1 turns it on (default 0 = everything as before). At startup ms-sdr
 * asks the radio (CMD_SET_CW_CARRIER 0x15 IN, 1 byte, bit 0 = feature present); only then
 * does it send 0x15 value 1 to trans and recv.
 *
 * trans sends the block here as [0x15][384 bytes] (96 int16 I/Q frames); ms-sdr writes it
 * to the radio in 6 pieces of 64 bytes (0x15 OUT, wValue = piece 0..5, piece 5 makes it
 * active), reads the status back (bit 1 = block active) and answers trans 0x15 value 2.
 * Every 5 s in CW it checks the radio still has a block (a radio reset clears it) and
 * re-arms trans if not. Stop_all clears the radio's block (0x15 OUT, wValue 0xFF, no data),
 * so the key can't send RF with the servers down.
 */
#include "extern.h"
#include "usbavrcmd.h"
#include <string.h>

#define CWC_BLOCK_BYTES   384
#define CWC_PIECE_BYTES   64
#define CWC_PIECES        6
#define CWC_CHECK_SECONDS 5

uint8_t G_cw_carrier_ini = FALSE;   /* mscc.ini CW_CARRIER= */
uint8_t G_cw_carrier_ok = FALSE;    /* option on and the radio has the feature */

static int radio_status(void) {
    unsigned char b = 0;
    int r = usbControlMsgIN(CMD_SET_CW_CARRIER, 0, 0, (char *)&b, 1);
    return (r == 1) ? (int)b : -1;
}

void Cw_carrier_startup(void) {
    int st;

    if (!G_cw_carrier_ini) {
        return;
    }
    if (G_firmware_type != FIRMWARE_TYPE_STM32) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_startup. CW_CARRIER=1 but the radio is not a Black Pill: off\n",
                line_number++);
        return;
    }
    st = radio_status();
    if (st < 0 || (st & 0x01) == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_startup. radio has no stored CW carrier (status %d, needs pill 3.90+): off\n",
                line_number++, st);
        return;
    }
    G_cw_carrier_ok = TRUE;
    SDRcore_trans_send_param(CMD_SET_CW_CARRIER, 1);
    SDRcore_recv_send_param(CMD_SET_CW_CARRIER, 1);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cw_carrier_startup. ON (radio status %d): trans + recv told\n",
            line_number++, st);
}

/* Command_Interface: [0x15][384 bytes] from trans. */
void Cw_carrier_block(const char *msg, int len) {
    static int logged_ok = 0;
    int i, r, st;

    if (!G_cw_carrier_ok) {
        return;
    }
    if (len != 1 + CWC_BLOCK_BYTES) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_block. wrong length %d (want %d), ignored\n",
                line_number++, len, 1 + CWC_BLOCK_BYTES);
        return;
    }
    for (i = 0; i < CWC_PIECES; i++) {
        r = usbControlMsgOUT(CMD_SET_CW_CARRIER, i, 0, (char *)&msg[1 + i * CWC_PIECE_BYTES], CWC_PIECE_BYTES);
        if (r != CWC_PIECE_BYTES) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Cw_carrier_block. piece %d to the radio FAILED (%d)\n",
                    line_number++, i, r);
            return;
        }
    }
    st = radio_status();
    if (st < 0 || (st & 0x02) == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_block. radio status %d after the block: not active\n",
                line_number++, st);
        return;
    }
    SDRcore_trans_send_param(CMD_SET_CW_CARRIER, 2);
    if (!logged_ok) {
        logged_ok = 1;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_block. block in the radio (first one; later ones not logged)\n",
                line_number++);
    }
}

/* Flusher_thread, once a second. */
void Cw_carrier_tick(void) {
    static int seconds = 0;
    int st;

    if (!G_cw_carrier_ok || G_mode != 'C') {
        seconds = 0;
        return;
    }
    if (++seconds < CWC_CHECK_SECONDS) {
        return;
    }
    seconds = 0;
    st = radio_status();
    if (st >= 0 && (st & 0x02) == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Cw_carrier_tick. radio has no block in CW: trans re-armed\n",
                line_number++);
        SDRcore_trans_send_param(CMD_SET_CW_CARRIER, 1);
    }
}

/* Stop_all: no servers = no stored carrier in the radio. */
void Cw_carrier_stop(void) {
    if (!G_cw_carrier_ok) {
        return;
    }
    (void)usbControlMsgOUT(CMD_SET_CW_CARRIER, 0xFF, 0, NULL, 0);
    G_cw_carrier_ok = FALSE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cw_carrier_stop. radio's stored carrier cleared\n", line_number++);
}
