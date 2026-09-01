#include "extern.h"
//#define MAX_AVERAGE 4
int G_band_marker_low = 0;
int G_band_marker_high = 0;
#define MAX_Y 4999

extern panadapter_buffer panbuffer;
#pragma pack(1)

struct {
    uint8_t opcode;
    uint8_t sequence;
    uint16_t Y[400];

} panbuffer_temp;

struct {

    struct {
        uint8_t opcode;
        uint8_t sequence;
        uint16_t output_buffer[400];
    } avg_buffer_output;

    struct {
        uint16_t avg_buffer_input[400];
    } buffer_input[4];
} panadaper_average[2];

void *Panadapter_thread(void *t) {
    int x = 0;
    int send_size = 0;
    int Y_source_index = 0;
    //int marker_low = 0;
    //int marker_high = 0;
    //int marker_center = 0;
    uint8_t tx_high_cut = 0;
    uint8_t tx_low_cut = 0;
    int i = 0;
    int average_count = 0;
    int sequence = 0;
    //uint16_t max_y = 0;
    uint16_t mixing_product = 0;
    int sleep_time = 1;
    //int test = 1;

    send_size = sizeof (panadaper_average->avg_buffer_output);
    print_time();
    fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Delaying 5 seconds before the start of Panadapter Data\n", line_number++);
    Sleep(5000); //Let the subsystem set up parameters before sending panadapter data
    print_time();
    fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Panadapter Data Processing Started \n", line_number++);

    panbuffer.panReady = 0;
    while (G_all_threads_run) {
        if (G_mode == 'C' || G_mode == 'T') {
            tx_low_cut = 2;
            tx_high_cut = 2;
        } else {
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
            } else {
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
                } else {
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
        sequence = 0;
        //panbuffer.Y is 800 uint16_t or 1600 bytes. This is too long to send over the Internet without the network segmenting the packet
        //Segmenting may cause packets to arrive out of order. The panbuffer.Y buffer is split into two 800 byte packets 
        //and sent individually
        //Each with a unique segment indicator. MSCC will discard both segments if they arrive out of order.

        //Build the first 800 byte packet
        if (panbuffer.panReady) {//Process the panbuffer.Y buffer if panbufer.panReady is true
            panadaper_average[sequence].avg_buffer_output.opcode = CMD_GET_SET_PANADAPTER;
            panadaper_average[sequence].avg_buffer_output.sequence = sequence;
            for (x = 0; x < MAX_X; x++) {
                panbuffer_temp.Y[x] = panbuffer.Y[x];
                if (panbuffer_temp.Y[x] > MAX_Y) {
                    panbuffer_temp.Y[x] = MAX_Y;
                    //print_time();
                    //fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Max_Y: %ld \n", line_number++, panbuffer_temp.Y[x]);
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
            if (G_Monitor == 1 && G_tx_mode == TRUE) {
                for (i = 0; i < 400 - tx_low_cut; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                }
            } else {
                //This removes the -12KHz mixing product from the spectrum display
                mixing_product = (panadaper_average[sequence].avg_buffer_output.output_buffer[265] +
                        panadaper_average[sequence].avg_buffer_output.output_buffer[269]) / 2;
                for (i = 266; i < 268; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = mixing_product;
                }
            }

            //Send the first 800 byte packet.
            if (sendto(ms_sdr_s, (char *) &panadaper_average[sequence].avg_buffer_output, send_size, 0,
                    (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Panadapter_thread -> sentto failed with error code : %s\n", line_number++, strerror(errno));
            }
            for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {
                memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input,
                        panadaper_average[sequence].buffer_input[(average_count + 1)].avg_buffer_input, (MAX_X * 2));
            }
            memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input, panbuffer_temp.Y,
                    (MAX_X * 2));

            //Now build the second packet
            sequence = 1;
            Y_source_index = MAX_X;
            for (x = 0; x < MAX_X; x++, Y_source_index++) {
                panbuffer_temp.Y[x] = panbuffer.Y[Y_source_index];
                if (panbuffer_temp.Y[x] > MAX_Y) {
                    panbuffer_temp.Y[x] = MAX_Y;
                    //print_time();
                    //fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Max_Y: %ld \n", line_number++, panbuffer_temp.Y[x]);
                }
            }
            for (i = 0; i < MAX_X; i++) {
                panadaper_average[1].avg_buffer_output.output_buffer[i] = 0;
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
            if (G_Monitor == 1 && G_tx_mode == TRUE) {

                for (i = tx_high_cut; i < 400; i++) {
                    panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
                }
            }
            //panadaper_average[sequence].avg_buffer_output.output_buffer[marker_high] = MAX_MARKER_SIZE;
            panadaper_average[sequence].avg_buffer_output.opcode = CMD_GET_SET_PANADAPTER;
            panadaper_average[sequence].avg_buffer_output.sequence = sequence;
            //Send the second packet
            if (sendto(ms_sdr_s, (char *) &panadaper_average[1].avg_buffer_output,
                    send_size, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Panadapter_thread -> sentto failed with error code : %s\n", line_number++, strerror(errno));
            }
            for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {
                memcpy(panadaper_average[1].buffer_input[average_count].avg_buffer_input,
                        panadaper_average[1].buffer_input[(average_count + 1)].avg_buffer_input, (MAX_X * 2));
            }
            memcpy(panadaper_average[1].buffer_input[average_count].avg_buffer_input, panbuffer_temp.Y, (MAX_X * 2));
            panbuffer.panReady = 0;
        }
        Sleep(sleep_time); //This iteration finished.
    }
    pthread_exit(0);
    return (NULL);
}