#include "extern.h"
//#define MAX_AVERAGE 4
int G_band_marker_low = 0;
int G_band_marker_high = 0;
/* Was 4999 — only ~33 dB of span (MAX_Y/150) above the Y=0 floor, so after client
 * dB CAL the noise was a flat shelf. 16000 ≈ 106 dB of log headroom (uint16 path).
 * Paired with dsputils bias 40 (was 22): deeper Y=0 floor for ~−116 grass; peaks still fit. */
#define MAX_Y 16000
/* Full pan frames to blank after large LO/band change (count decrements once
 * per complete multi-segment frame). History is cleared so this can stay short.
 * Tuned iteratively: 16 bump / 24 delay / 18–20 sweet spot → settle on 20. */
#define PAUSE_CYCLE_COUNT_LIMIT 20
extern panadapter_buffer panbuffer;
extern int G_Panadapter_Pixels;
#pragma pack(1)

struct {
    uint8_t opcode;
    uint8_t sequence;
    uint16_t Y[MAX_X];
} panbuffer_temp;

/* One average bank per 400-bin UDP segment (max 8 for 3200 bins). */
struct {
    struct {
        uint8_t opcode;
        uint8_t sequence;
        uint16_t output_buffer[MAX_X];
    } avg_buffer_output;

    struct {
        uint16_t avg_buffer_input[MAX_X];
    } buffer_input[4];
} panadaper_average[MAX_PAN_SEGMENTS];

/* Clear smoothing history so a PLL glitch does not linger after unpause. */
static void Clear_Panadapter_Average_History(void) {
    int seq;
    int avg;
    int i;

    for (seq = 0; seq < MAX_PAN_SEGMENTS; seq++) {
        for (avg = 0; avg < 4; avg++) {
            for (i = 0; i < MAX_X; i++) {
                panadaper_average[seq].buffer_input[avg].avg_buffer_input[i] = 0;
            }
        }
        for (i = 0; i < MAX_X; i++) {
            panadaper_average[seq].avg_buffer_output.output_buffer[i] = 0;
        }
    }
}

void *Panadapter_thread(void *t) {
    int x = 0;
    int send_size = 0;
    int Y_source_index = 0;
    uint8_t tx_high_cut = 0;
    uint8_t tx_low_cut = 0;
    int i = 0;
    int average_count = 0;
    int sequence = 0;
    uint16_t mixing_product = 0;
    int sleep_time = 1;
    int pause_cycle_count = 0;
    int pausing = 0;
    int pixels = 800;
    int segments = 2;
    int mp_bin = 266;

    send_size = sizeof (panadaper_average->avg_buffer_output);
    print_time();
    fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Delaying 5 seconds before the start of Panadapter Data\n", line_number++);
    Sleep(5000); //Let the subsystem set up parameters before sending panadapter data
    print_time();
    fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Panadapter Data Processing Started \n", line_number++);

    panbuffer.panReady = 0;
    Clear_Panadapter_Average_History();
    while (G_all_threads_run) {
        /* Flush smoother history when requested (also set with pause). */
        if (G_Pan_Flush_History) {
            G_Pan_Flush_History = FALSE;
            Clear_Panadapter_Average_History();
        }
        /* New pause: small LO step (short) or large/band (longer). */
        if (G_Pause_Panadapter_Reload) {
            G_Pause_Panadapter_Reload = FALSE;
            G_Pause_Panadapter = TRUE;
            if (G_Pause_Panadapter_Cycles > 0) {
                pause_cycle_count = G_Pause_Panadapter_Cycles;
            } else {
                pause_cycle_count = PAUSE_CYCLE_COUNT_LIMIT;
            }
            Clear_Panadapter_Average_History();
            print_time();
            fprintf(G_fp_logfile, "[%d] Panadapter_thread. Pause armed. cycles=%d\n",
                line_number++, pause_cycle_count);
        }
        pausing = (G_Pause_Panadapter && pause_cycle_count > 0) ? 1 : 0;

        if (G_mode == 'C' || G_mode == 'T') {
            tx_low_cut = 2;
            tx_high_cut = 2;
        }
        else {
            if (G_mode == 'U') {
                tx_low_cut = 0;
                switch (G_tx_band_pass) {
                case 3:
                    tx_high_cut = 62;
                    break;
                case 2:
                    tx_high_cut = 60;
                    break;
                case 1:
                    tx_high_cut = 50;
                    break;
                case 0:
                    tx_high_cut = 40;
                    break;
                }
            }
            else {
                if (G_mode == 'L') {
                    tx_high_cut = 0;
                    switch (G_tx_band_pass) {
                    case 3:
                        tx_low_cut = 62;
                        break;
                    case 2:
                        tx_low_cut = 60;
                        break;
                    case 1:
                        tx_low_cut = 50;
                        break;
                    case 0:
                        tx_low_cut = 40;
                        break;
                    }
                }
                else {
                    if (G_mode == 'A') {
                        switch (G_tx_band_pass) {
                        case 3:
                            tx_high_cut = 62;
                            tx_low_cut = 62;
                            break;
                        case 2:
                            tx_high_cut = 60;
                            tx_low_cut = 60;
                            break;
                        case 1:
                            tx_high_cut = 50;
                            tx_low_cut = 50;
                            break;
                        case 0:
                            tx_high_cut = 40;
                            tx_low_cut = 40;
                            break;
                        }
                    }
                }
            }
        }

        /*
         * panbuffer.Y is up to 3200 uint16. Split into 400-bin packets with sequence 0..N-1
         * so the network does not fragment a single datagram. MSCC assembles by sequence.
         */
        if (panbuffer.panReady & !G_tx_mode) {
            pixels = G_Panadapter_Pixels;
            if (pixels < 800) pixels = 800;
            if (pixels > MAX_PIXELS) pixels = MAX_PIXELS;
            /* Keep multiple of MAX_X */
            pixels = (pixels / MAX_X) * MAX_X;
            if (pixels < MAX_X) pixels = MAX_X;
            segments = pixels / MAX_X;
            if (segments > MAX_PAN_SEGMENTS) segments = MAX_PAN_SEGMENTS;

            /* -12 kHz mixing-product notch scales with bin count (was bin ~266 of 800). */
            mp_bin = pixels / 3;
            if (mp_bin < 2) mp_bin = 2;
            if (mp_bin > pixels - 3) mp_bin = pixels - 3;

            for (sequence = 0; sequence < segments; sequence++) {
                panadaper_average[sequence].avg_buffer_output.opcode = CMD_GET_SET_PANADAPTER;
                panadaper_average[sequence].avg_buffer_output.sequence = (uint8_t)sequence;
                Y_source_index = sequence * MAX_X;

                for (x = 0; x < MAX_X; x++) {
                    if (pausing || (Y_source_index + x) >= pixels) {
                        panbuffer_temp.Y[x] = 0;
                    } else {
                        panbuffer_temp.Y[x] = panbuffer.Y[Y_source_index + x];
                        if (panbuffer_temp.Y[x] > MAX_Y) {
                            panbuffer_temp.Y[x] = MAX_Y;
                        }
                    }
                }
                for (i = 0; i < MAX_X; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                }
                for (average_count = 0; average_count < G_Smoothing; average_count++) {
                    for (i = 0; i < MAX_X; i++) {
                        panadaper_average[sequence].avg_buffer_output.output_buffer[i] +=
                            panadaper_average[sequence].buffer_input[average_count].avg_buffer_input[i];
                    }
                }
                for (i = 0; i < MAX_X; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] =
                        (panadaper_average[sequence].avg_buffer_output.output_buffer[i] +
                            panbuffer_temp.Y[i]) / G_Smoothing;
                }

                /* Monitor TX blanking: scale original 400-wide half cuts */
                if (G_Monitor == 1 && G_tx_mode == TRUE && sequence == 0) {
                    int cut = tx_low_cut;
                    if (cut > MAX_X) cut = MAX_X;
                    for (i = 0; i < MAX_X - cut; i++) {
                        panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                    }
                }
                else if (G_Monitor == 1 && G_tx_mode == TRUE && sequence == segments - 1) {
                    int cut = tx_high_cut;
                    if (cut > MAX_X) cut = MAX_X;
                    for (i = cut; i < MAX_X; i++) {
                        panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                    }
                }
                else if (!pausing) {
                    /* Mixing-product fix if this segment owns mp_bin */
                    if (mp_bin >= Y_source_index && mp_bin < Y_source_index + MAX_X) {
                        int local = mp_bin - Y_source_index;
                        int lo = local - 1;
                        int hi = local + 2;
                        if (lo < 0) lo = 0;
                        if (hi > MAX_X - 1) hi = MAX_X - 1;
                        if (hi > lo) {
                            mixing_product =
                                (panadaper_average[sequence].avg_buffer_output.output_buffer[lo] +
                                 panadaper_average[sequence].avg_buffer_output.output_buffer[hi]) / 2;
                            for (i = lo + 1; i < hi; i++) {
                                panadaper_average[sequence].avg_buffer_output.output_buffer[i] = mixing_product;
                            }
                        }
                    }
                }

                if (pausing) {
                    memset(&panadaper_average[sequence].avg_buffer_output.output_buffer, 0, MAX_X * 2);
                }

                if (sendto(ms_sdr_s, (char*)&panadaper_average[sequence].avg_buffer_output, send_size, 0,
                    (struct sockaddr*)&si_ms_sdr, slen) == SOCKET_ERROR) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Panadapter_thread -> sentto failed seq=%d error: %s\n",
                        line_number++, sequence, strerror(errno));
                }

                for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {
                    memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input,
                        panadaper_average[sequence].buffer_input[(average_count + 1)].avg_buffer_input, (MAX_X * 2));
                }
                memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input, panbuffer_temp.Y,
                    (MAX_X * 2));
            }

            /* Decrement pause once per full multi-segment frame */
            if (pausing) {
                pause_cycle_count--;
                if ((pause_cycle_count % 20) == 0) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Panadapter_thread. Paused. remaining frames: %d (segs=%d)\n",
                        line_number++, pause_cycle_count, segments);
                }
                if (pause_cycle_count <= 0) {
                    G_Pause_Panadapter = FALSE;
                    pause_cycle_count = 0;
                    Clear_Panadapter_Average_History();
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Panadapter_thread. Pause complete — resume spectrum\n",
                        line_number++);
                }
            }

            panbuffer.panReady = 0;
        }
        Sleep(sleep_time); //This iteration finished.
    }
    pthread_exit(0);
    return (NULL);
}
