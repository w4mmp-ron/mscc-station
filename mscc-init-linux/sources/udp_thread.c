#include "extern.h"

#define BUFLEN 512  //Max length of buffer

#define SERVER "127.0.0.1"
#define PA_SAMPLE_TYPE      paFloat32
#define MAX_CALIBRATION_ELEMENT 2000
#define CW_CENTER 600

#define CALIBRATION_LOW_LIMIT 5000000
#define CALBIRATION_LOW_LIMIT_LOOSE 1000000
extern iq_stack G_iq_stack[];
extern agc_state agcstate;
extern anotch_state anstate;

char buf[BUFLEN];
int count = 0;

uint16_t G_SDRcore_port = 0;
uint16_t ms_sdr_port = 0;
struct sockaddr_in si_me, si_ms_sdr;
int sdrcore_s, ms_sdr_s, recv_len;
int slen = sizeof (si_ms_sdr);
extern struct output_devices G_output_devices[MAX_OUTPUT_DEVICES];
int G_calibration_element = 0;
int G_Smoothing = 2;
uint8_t G_tx_mode = 0;
uint8_t G_Panadapter_Blocks = 8;
uint8_t G_PCB_Version = 0;
sp_float G_cw_low_cut = 500.0f;
sp_float G_cw_high_cut = 700.0f;
sp_float G_CW_pitch = 600.0;
char G_mode = 'U';
uint32_t Calibration_Low_Limit = CALIBRATION_LOW_LIMIT;
uint8_t G_Monitor = 0;
uint8_t G_tx_band_pass = 0;
uint8_t G_Image_Check = FALSE;

struct {
    uint32_t freq;
    uint32_t magnitude_low;
    uint32_t magnitude;
    uint32_t magnitude_high;
    float low_cut;
    float center;
    float high_cut;
} cal_data[MAX_CALIBRATION_ELEMENT];

struct {
    float low_cut;
    float center;
    float high_cut;
} calibration_limits;

uint8_t opcode;
uint8_t t_opcode_data;
short int *opcode_data;
short int s_opcode_data;
int *op_code_data_32;
int i_opcode_data;

int Setup_UDP(void) {
    int status = 0;
    WSADATA dll_wsa;

    print_time();
    fprintf(G_fp_logfile, "[%d] Setup_UDP -> Initializing Winsock...\n", line_number++);
    if (WSAStartup(MAKEWORD(2, 2), &dll_wsa) != NO_ERROR)
    {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Initializing Winsock: Failed. Error Code : %d\n", line_number++, WSAGetLastError());
        status = 1;
    }
    if ((sdrcore_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Create Socket: Failed. Error Code : %s\n", line_number++, strerror(errno));
        status = 1;
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP Thread -> Socket Created\n", line_number++);
    }

    // Set up the SDRcore port for receiving UDP packets from any IP address
    // Initialize structures
    if (G_SDRcore_port == 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Invalid DLL Port. Using default: %d\n", line_number++, SDRCORE_RECV_PORT);
        G_SDRcore_port = SDRCORE_RECV_PORT;
    }
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(G_SDRcore_port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind socket to port
    if (bind(sdrcore_s, (struct sockaddr*) &si_me, sizeof (si_me)) != 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Bind Failed. Error Code : %s\n", line_number++, strerror(errno));
        status = 1;
    }

    //Setup  UDP configuration for sending to ms-sdr
    if (ms_sdr_port == 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Invalid MS-SDR Port. Using default: %d\n", line_number++, MS_SDR_PORT);
        ms_sdr_port = MS_SDR_PORT;
    }
    if ((ms_sdr_s = socket(AF_INET, SOCK_DGRAM ,IPPROTO_UDP)) == INVALID_SOCKET) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> ms_sdr_s -> Create Socket: Failed. Error Code : %s\n", line_number++, strerror(errno));
        status = 1;
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP ->ms_sdr_s -> Socket Created\n", line_number++);
    }
    memset((char *) &si_ms_sdr, 0, sizeof (si_ms_sdr));
    si_ms_sdr.sin_family = AF_INET;
    si_ms_sdr.sin_port = htons(ms_sdr_port);
    si_ms_sdr.sin_addr.s_addr = inet_addr(SERVER);
    return status;
}

int update_calibrate_data(uint32_t value) {
    int status = 0;
    int calibration_state = 0;
    int count = 0;
    char send_buf[20];

    print_time();
    fprintf(G_fp_logfile, "[%d] update_calibrate_data -> Called -> Data: %ld\n", line_number++, value);
    while (calibration_state == 0 && count < 30000) {
        Sleep(1);
        if (mycalstate.calReady == 1) {
            cal_data[G_calibration_element].magnitude_low = mycalstate.calMagLow;
            cal_data[G_calibration_element].freq = value;
            cal_data[G_calibration_element].magnitude = mycalstate.calMag;
            cal_data[G_calibration_element].magnitude_high = mycalstate.calMagHigh;
            cal_data[G_calibration_element].low_cut = calibration_limits.low_cut;
            cal_data[G_calibration_element].center = calibration_limits.center;
            cal_data[G_calibration_element].high_cut = calibration_limits.high_cut;
            calibration_state = 1;
        } else {
            count++;
        }
    }
    if (calibration_state) {
        G_calibration_element++;
        send_buf[0] = CMD_SET_CAL_DATA_PROCESSED;
        send_buf[1] = 1;
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
        print_time();
        fprintf(G_fp_logfile, "[%d] update_calibrate_data -> Calibration data sent. Loop Count: %d, send_buf[0]: %X\n", line_number++, count,
                send_buf[0]);
    } else {
        G_calibration_element = 0;
        send_buf[0] = CMD_SET_CAL_DATA_PROCESSED;
        send_buf[1] = 0;
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
        print_time();
        fprintf(G_fp_logfile, "[%d] update_calbrate_data -> Calibration routine -> FAILED -> Loop Count: %d\n", line_number++, count);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] update_calibrate_data -> Finished.\n", line_number++);
    return status;
}

uint32_t Send_calibration_data() {
    char send_buf[20];
    //int calibration_state = 0;
    //int error = 0;
    uint32_t freq;
    uint32_t magnitude_low_max = 0;
    uint32_t magnitude_mid_max = 0;
    uint32_t magnitude_high_max = 0;
    uint32_t average_magnitude_max = 0;
    int low_max = 0;
    int mid_max = 0;
    int high_max = 0;
    int count = 0;


    print_time();
    fprintf(G_fp_logfile, "[%d] Send_calibration_data -> Called\n", line_number++);
    for (count = 0; count < G_calibration_element; count++) {
        if (magnitude_low_max < cal_data[count].magnitude_low) {
            magnitude_low_max = cal_data[count].magnitude_low;
            low_max = count;
        }
        if (magnitude_mid_max < cal_data[count].magnitude) {
            magnitude_mid_max = cal_data[count].magnitude;
            mid_max = count;
        }
        if (magnitude_high_max < cal_data[count].magnitude_high) {
            magnitude_high_max = cal_data[count].magnitude_high;
            high_max = count;
        }
    }
    average_magnitude_max = (uint32_t) ((float) ((magnitude_high_max + magnitude_low_max + magnitude_mid_max)) / 3.0f);
    if (average_magnitude_max > Calibration_Low_Limit) {
        freq = (cal_data[low_max].freq + cal_data[mid_max].freq + cal_data[high_max].freq) / 3;
    } else {
        freq = 0;
    }
    send_buf[0] = CMD_SET_CALIBRATION_DATA;
    memcpy(&send_buf[1], &freq, 4);
    sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
    G_calibration_element = 0;
    print_time();
    fprintf(G_fp_logfile, "[%d] Send_calibration_data -> average_magnitude_max %ld\n", line_number,
            average_magnitude_max);
    print_time();
    fprintf(G_fp_logfile, "[%d] Send_calibration_data -> calculate_offset_freq -> Finished. calculate_offset_freq: %ld\n",
            line_number++, freq);
    return freq;
}

int CW_snap_update_calibrate_data() {
    int status = 0;
    int calibration_state = 0;
    int count = 0;
    char send_buf[20];

    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> Called.\n", line_number++);
    while (calibration_state == 0 && count < 30000) {
        Sleep(1);
        if (mycalstate.calReady == 1) {
            cal_data[G_calibration_element].magnitude_low = mycalstate.calMagLow;
            cal_data[G_calibration_element].magnitude = mycalstate.calMag;
            cal_data[G_calibration_element].magnitude_high = mycalstate.calMagHigh;
            cal_data[G_calibration_element].low_cut = calibration_limits.low_cut;
            cal_data[G_calibration_element].center = calibration_limits.center;
            cal_data[G_calibration_element].high_cut = calibration_limits.high_cut;
            print_time();
            fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> cal_data[G_calibration_element].low_cut: %f\n",
                    line_number++, cal_data[G_calibration_element].low_cut);
            print_time();
            fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> cal_data[G_calibration_element].center: %f\n",
                    line_number++, cal_data[G_calibration_element].center);
            print_time();
            fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> cal_data[G_calibration_element].high_cut: %f\n",
                    line_number++, cal_data[G_calibration_element].high_cut);
            calibration_state = 1;
        } else {
            count++;
        }
    }
    if (calibration_state) {
        G_calibration_element++;
        send_buf[0] = CMD_CW_SNAP_DATA_PROCESSED;
        send_buf[1] = 1;
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
        print_time();
        fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> Calibration data sent. Loop Count: %d, G_calibration_element %d\n",
                line_number++, count, G_calibration_element);
    } else {
        G_calibration_element = 0;
        send_buf[0] = CMD_CW_SNAP_DATA_PROCESSED;
        send_buf[1] = 0;
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
        print_time();
        fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> Calibration routine -> FAILED -> Loop Count: %d\n", line_number++, count);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_update_calibrate_data -> Finished.\n", line_number++);
    return status;
}

uint16_t CW_snap_Send_calibration_data() {
    char send_buf[20];
    //int calibration_state = 0;
    //int error = 0;
    uint32_t magnitude_low_max = 0;
    uint32_t magnitude_mid_max = 0;
    uint32_t magnitude_high_max = 0;
    int low_max_count = 0;
    int mid_max_count = 0;
    int high_max_count = 0;
    int count = 0;
    uint16_t tune_delta = 0;
    uint8_t sign = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data -> Called. G_calibration_element: %d\n", line_number++, G_calibration_element);
    for (count = 0; count < G_calibration_element; count++) {
        if (magnitude_low_max < cal_data[count].magnitude_low) {
            magnitude_low_max = cal_data[count].magnitude_low;
            low_max_count = count;

        }
        if (magnitude_mid_max < cal_data[count].magnitude) {
            magnitude_mid_max = cal_data[count].magnitude;
            mid_max_count = count;

        }
        if (magnitude_high_max < cal_data[count].magnitude_high) {
            magnitude_high_max = cal_data[count].magnitude_high;
            high_max_count = count;

        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data.  low_max_count: %d\n", line_number++, low_max_count);
    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data.  mid_max_count: %d\n", line_number++, mid_max_count);
    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data.  high_max_count: %d\n", line_number++, high_max_count);
    if (cal_data[low_max_count].magnitude_low >= cal_data[mid_max_count].magnitude &&
            cal_data[low_max_count].magnitude_low >= cal_data[high_max_count].magnitude_high) {
        tune_delta = (uint16_t) cal_data[low_max_count].low_cut;
        print_time();
        fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data. low_cut: %f\n", line_number++, cal_data[low_max_count].low_cut);
        sign = 0;
    }
    if (cal_data[mid_max_count].magnitude >= cal_data[low_max_count].magnitude_low &&
            cal_data[mid_max_count].magnitude >= cal_data[high_max_count].magnitude_high) {
        tune_delta = (uint16_t) cal_data[mid_max_count].center;
        print_time();
        fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data. center: %f\n", line_number++, cal_data[mid_max_count].center);
        sign = 1;
    }
    if (cal_data[high_max_count].magnitude_high >= cal_data[low_max_count].magnitude_low &&
            cal_data[high_max_count].magnitude_high >= cal_data[mid_max_count].magnitude) {
        tune_delta = (uint16_t) cal_data[high_max_count].high_cut;
        print_time();
        fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data. high_cut: %f\n", line_number++, cal_data[high_max_count].high_cut);
        sign = 2;
    }
    switch (sign) {
        case 0:
            tune_delta = (uint16_t) G_CW_pitch - tune_delta;
            break;
        case 1:
            tune_delta = 0;
            break;
        case 2:
            tune_delta = tune_delta - (uint16_t) G_CW_pitch;
            break;

    }
    send_buf[0] = CMD_CW_SNAP_SET_CALIBRATION_DATA;
    send_buf[1] = sign;
    memcpy(&send_buf[2], &tune_delta, 2);
    sendto(ms_sdr_s, send_buf, sizeof (send_buf), 0, (struct sockaddr *) &si_ms_sdr, slen);
    G_calibration_element = 0;
    print_time();
    fprintf(G_fp_logfile, "[%d] CW_snap_Send_calibration_data -> Finished. delta: %d, sign: %d\n", line_number++, tune_delta, sign);
    return tune_delta;
}

void *Send_smeter_thread(void *param) {
    //uint16_t average_signal = 0;
    sp_float f_smeter = 0;
    int16_t i_smeter = 0;
    char send_buf[20];
    int status = 0;
    uint8_t error_reported = 0;
    //int l_slen = 0;

    Sleep(6000); //Let the subsystem start before sending S Meter data
    print_time();
    fprintf(G_fp_logfile, "[%d] Send_smeter_thread -> Started.\n", line_number++);
    print_time();
    fprintf(G_fp_logfile, "[%d] Send_smeter_thread -> Started -> Send of S-Meter data started \n", line_number++);
    send_buf[0] = CMD_GET_SET_SMETER;
    while (G_all_threads_run) {
        Sleep(SEND_INTERVAL);
        f_smeter = mystate.peakRxSignalDbm;
        i_smeter = (int16_t) f_smeter;
        if (!G_tx_mode) {
            memcpy(&send_buf[1], &i_smeter, 2);
            if (sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                if (error_reported < 10) {//Report the error 10 time and then stop reporting. 10 reports is sufficient to know something is wrong
                    print_time();
                    fprintf(G_fp_logfile, "[%d]  Send_smeter_thread-> sentto failed with error code : %s\n", line_number++, strerror(errno));
                    status = 1;
                    error_reported++;
                }
            }
        }
    }
    pthread_exit(0);
    return (NULL);
}

void set_cw_bandwidth(int cw_size, int cw_center) {
    switch (cw_size) { //Width of the CW filter index. 0-> 200Hz. 1 -> 400Hz. 2-> 1.8KHz
        case 0: //200Hz filter width
            switch (cw_center) { //CW PITCH - Center of the filter index.  0-> 400Hz. 1-> 500Hz. 2-> 600Hz. 3-> 700Hz. 4-> 800Hz. 
                case 0:
                    G_cw_low_cut = 300.0f;
                    G_cw_high_cut = 500.0f;
                    break;
                case 1:
                    G_cw_low_cut = 400.0f;
                    G_cw_high_cut = 600.0f;
                    break;
                case 2:
                    G_cw_low_cut = 500.0f;
                    G_cw_high_cut = 700.0f;
                    break;
                case 3:
                    G_cw_low_cut = 600.0f;
                    G_cw_high_cut = 800.0f;
                    break;
                case 4:
                    G_cw_low_cut = 700.0f;
                    G_cw_high_cut = 900.0f;
                    break;
            }
            break;
        case 1: //400Hz filter width
            switch (cw_center) {
                case 0:
                    G_cw_low_cut = 200.0f;
                    G_cw_high_cut = 600.0f;
                    break;
                case 1:
                    G_cw_low_cut = 300.0f;
                    G_cw_high_cut = 700.0f;
                    break;
                case 2:
                    G_cw_low_cut = 400.0f;
                    G_cw_high_cut = 800.0f;
                    break;
                case 3:
                    G_cw_low_cut = 500.0f;
                    G_cw_high_cut = 900.0f;
                    break;
                case 4:
                    G_cw_low_cut = 600.0f;
                    G_cw_high_cut = 1000.0f;
                    break;
            }
            break;
        case 2: //1.8Hz filter width
            G_cw_low_cut = 75.0f;
            G_cw_high_cut = 1800.00f;
            /*switch (cw_center) {
            case 0: //400Hz
                    G_cw_low_cut = 400.0 + 75.0f;
                    G_cw_high_cut = 400.0 + 1800.0f;
                    break;
            case 1:	//500Hz
                    G_cw_low_cut = 500.0 + 75.0f;
                    G_cw_high_cut = 500.0 + 1800.0f;
                    break;
            case 2:	//600Hz
                    G_cw_low_cut = 600.0 + 75.0f;
                    G_cw_high_cut = 600.0 + 1800.0f;
                    break;
            case 3:	//700Hz
                    G_cw_low_cut = 700.0 + 75.0f;
                    G_cw_high_cut = 700.0 + 1800.0f;
                    break;
            case 4: //800Hz
                    G_cw_low_cut = 800.0 + 75.0f;
                    G_cw_high_cut = 800.0 + 1800.0f;
                    break;
            }
            break;*/
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] set_cw_bandwidth -> Filter size: %d, Filter Center: %d\n", line_number++, cw_size, cw_center);
}

void Speaker_Volume_Ramp_Up(float target_volume) {
    float volume = 0.0f;
    print_time();
    fprintf(G_fp_logfile, "[%d] Speaker_Volume_Ramp_Up -> target_volume: %f\n", line_number++, target_volume);
    //while (volume < target_volume) {
    //	Sleep(25);
    //	volume += 0.01f;
    //	volumeLevel = volume;
    Sleep(100);
    G_volumeLevel = target_volume;
}

float Get_Volume_Attn(int index) {
    float attenuation = 0.0f;
    switch (index) {
        case 0:
            attenuation = 1.0f;
            break;
        case 5:
            attenuation = 0.16f;
            break;
        case 4:
            attenuation = 0.32f;
            break;
        case 3:
            attenuation = 0.48f;
            break;
        case 2:
            attenuation = 0.64f;
            break;
        case 1:
            attenuation = 0.80f;
            break;
    }
    return attenuation;
}
int Get_IQ_Record(int band) {
    int record = 200;
    switch (band) {
        case 2200:
            record = IQ_B2200;
            break;
        case 630:
            record = IQ_B630;
            break;
        case 160:
            record = IQ_B160;
            break;
        case 80:
            record = IQ_B80;
            break;
        case 60:
            record = IQ_B60;
            break;
        case 40:
            record = IQ_B40;
            break;
        case 30:
            record = IQ_B30;
            break;
        case 20:
            record = IQ_B20;
            break;
        case 17:
            record = IQ_B17;
            break;
        case 15:
            record = IQ_B15;
            break;
        case 12:
            record = IQ_B12;
            break;
        case 10:
            record = IQ_B10;
            break;
    }
    return record;
}

void *UDP_Thread(void *my_param) {
    int status = 0;
    int stream_status = 0;
    sp_float low_cut;
    sp_float high_cut;
    static sp_float previous_high_cut = 2700.0f;
    static sp_float previous_low_cut = 75.0f;
    int cw_mode = 0;
    uint8_t speaker_muted = 0;
    float master_volume = 0.0f;
    float volume_attn = 1.0f;
    float master_side_tone_volume = 0.0f;
    int iq_offset = 0;
    int operating_band = 0;
    int previous_operating_band = 0;
    int iq_band = 200;
    uint8_t NB_Enable = 0;
    float NB_Threshold = 5.0f;
    int NB_Pulse_Width = 30;
    int cw_center = 0;
    int cw_bw = 0;
    int cw_bw_received = 0;
    int cw_pitch = 0;
    int cw_bw_set = 0;
    float AGC_Release = 1.0f;
    float AGC_Release_level = 1.0f;
    int AGC_Selected = 0;
    uint8_t UDP_status = 0;
    uint8_t previous_G_tx_mode = 0;
    int iq_init_status = 0;


#pragma pack(1)

    struct {
        uint8_t opcode;
        int iq_value;
    } iq_buffer;

    status = Setup_UDP();
    if (status == 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread -> Running\n", line_number++);
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread -> Waiting for data...\n", line_number++);
    } else {
        printf("UDP Setup_UDP Failed\n");
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread -> Socket Create failed. UDP Tread exiting\n", line_number++);
        pthread_exit(NULL);
    }

    print_output_devices();
    initBlanker(0, NB_Enable, NB_Threshold, NB_Pulse_Width);
    //Initialize the calibration routine limits to default
    calibration_limits.low_cut = 598.0f;
    calibration_limits.center = 600.0f;
    calibration_limits.high_cut = 602.0f;
    set_cw_bandwidth(cw_bw, cw_center);
    initAGC(1000.0f);
    //Load_Defaults();
    //iq_init_status = Init_IQ_structure();
    //if (iq_init_status == 0) {
    //    create_iq_ini_file(1);
    //}
    while (G_all_threads_run) {
        if ((recv_len = recvfrom(sdrcore_s, buf, BUFLEN, 0, (struct sockaddr *) &si_me, &slen)) == SOCKET_ERROR) {
            print_time();
            fprintf(G_fp_logfile, "[%d] UDP Thread -> recvfrom Failed. Error Code : %s. UDP Thread Exiting\n", line_number++, strerror(errno));
            UDP_status = 1;
            G_all_threads_run = 0;
        }
        opcode = (uint8_t) buf[0];
        t_opcode_data = (uint8_t) (buf[1]);
        opcode_data = (short int*) &buf[1];
        memcpy(&s_opcode_data, opcode_data, 2);

        switch (opcode) {
            case CMD_START_STOP_IMAGE_VALUE:
                G_Image_Check = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_START_STOP_IMAGE_VALUE -> G_Image_Check: %d\n",
                        line_number++, G_Image_Check);
                break;

            case CMD_SET_TX_HICUT:
                G_tx_band_pass = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_HICUT -> G_tx_band_pass: %d\n",
                        line_number++, G_tx_band_pass);
                break;

            case CMD_SET_MONITOR:
                G_Monitor = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_MONITOR -> G_Monitor: %d\n",
                        line_number++, G_Monitor);
                break;

            case SET_SIDE_TONE_VOLUME:
                print_time();
                if (t_opcode_data < 100) {
                    master_side_tone_volume = t_opcode_data * 0.01f * volume_attn;
                    G_t_volumeLevel = master_side_tone_volume;
                }
                fprintf(G_fp_logfile, "[%d] UDP Thread -> SET_SIDE_TONE_VOLUME. master_side_tone_volume %f\n",
                        line_number++, master_side_tone_volume);
                break;

            case CMD_SET_VOLUME_ATTN:
                volume_attn = Get_Volume_Attn(t_opcode_data);
                if (speaker_muted == FALSE) {
                    G_volumeLevel = master_volume * volume_attn;
                }
                G_t_volumeLevel = master_side_tone_volume * volume_attn;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_VOLUME_ATTN -> volume_attn: %f, master_volume: %f, G_volumeLevel: %f\n",
                        line_number++, volume_attn, master_volume, G_volumeLevel);
                break;

            case CMD_SET_CAL_LOOSE:
                print_time();
                if (t_opcode_data == 1) {
                    Calibration_Low_Limit = CALBIRATION_LOW_LIMIT_LOOSE;
                } else {
                    Calibration_Low_Limit = CALIBRATION_LOW_LIMIT;
                }
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CAL_LOOSE -> Value: %d\n", line_number++,
                        Calibration_Low_Limit);
                break;

            case CMD_GET_SET_AUTO_NOTCH:
                anstate.enabled = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_AUTO_NOTCH -> Enabled: %d\n", line_number++, anstate.enabled);
                break;

            case CMD_SET_TX_ON:
                G_tx_mode = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON -> G_tx_mode: %d\n", line_number++, G_tx_mode);
                if (previous_G_tx_mode != G_tx_mode) {
                    if (G_tx_mode == TRUE) {
                        if (G_mode != 'C') {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON \n",
                                    line_number++);
                            if (G_Monitor == 0) {
                                print_time();
                                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON -> G_Monitor: %d \n",
                                        line_number++, G_Monitor);
                                Pa_StopStream(stream);
                            }
                        } else {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON -> Starting Tone Stream stream: \n",
                                    line_number++);
                            stream_status = manage_stream(0, G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                            stream_status = manage_sidetone_stream(0, G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                            stream_status = manage_sidetone_stream(1, G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                        }
                    } else {
                        if (G_mode != 'C') {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON \n",
                                    line_number++);
                            if (G_Monitor == 0) {
                                print_time();
                                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON -> G_Monitor: %d \n",
                                        line_number++, G_Monitor);
                                Pa_StartStream(stream);
                            }
                        } else {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_TX_ON -> Stopping Tone Stream stream: \n",
                                    line_number++);
                            stream_status = manage_sidetone_stream(0,
                                    G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                            stream_status = manage_stream(0, G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                            stream_status = manage_stream(1, G_output_devices[G_output_device_index].device_index,
                                    G_output_devices[G_output_device_index].num_channels);
                        }
                    }
                    previous_G_tx_mode = G_tx_mode;
                }
                break;

            case CMD_SET_PCB_VERSION:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_PCB_VERSION -> Version: %d\n", line_number++, t_opcode_data);
                G_PCB_Version = t_opcode_data;
                iq_init_status = Init_IQ_structure();
                if (iq_init_status == 0) {
                    create_iq_ini_file(1);
                }
                iq_init_status = Init_IQ_structure();
                if (iq_init_status == 1) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP_Thread -> init_IQ_structure  -> Successful \n", line_number++);
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP_Thread -> init_IQ_structure -> Failed\n", line_number++);
                }
                break;

            case CMD_GET_SET_AGC:
                switch (t_opcode_data) {
                    case 0:
                        AGC_Release = 2000.0f;
                        AGC_Selected = 0;
                        break;
                    case 1:
                        AGC_Release = 1000.0f;
                        AGC_Selected = 1;
                        break;
                    case 2:
                        AGC_Release = AGC_Release_level;
                        AGC_Selected = 2;
                        break;
                }
                initAGC(AGC_Release); //Sets releaseOverride
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_AGC. index: %d, AGC_Release: %f \n", line_number++,
                        t_opcode_data, AGC_Release);
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_AGC.  releasems: %f, AGC_Initializing: %d\n", line_number++,
                        agcstate.releasems, AGC_Initializing);
                break;

            case CMD_SET_AGC_FAST_LEVEL:
                AGC_Release_level = (float) s_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_AGC_FAST_LEVEL. AGC_Release_level: %f \n", line_number++,
                        AGC_Release_level);
                switch (AGC_Selected) {
                    case 0:
                        break;
                    case 1:
                        break;
                    case 2:
                        AGC_Release = AGC_Release_level;
                        initAGC(AGC_Release); //Sets releaseOverride
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_AGC_FAST_LEVEL. releasems: %f, AGC_Initializing: %d\n", line_number++,
                                agcstate.releasems, AGC_Initializing);
                        break;
                }
                break;

            case CMD_GET_SET_PANADAPTER_REFRESH:
                G_Panadapter_Blocks = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_PANADAPTER_REFRESH: %d\n", line_number++,
                        G_Panadapter_Blocks);
                break;

            case CMD_SET_CW_PITCH:
                print_time();
                cw_center = t_opcode_data;
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CW_PITCH: %d\n", line_number++, cw_center);
                G_Pitch_changed = 1;
                if (cw_bw_set) {
                    switch (cw_center) {
                        case 0:
                            G_CW_pitch = 400.00;
                            G_side_tone_pitch = 400.00f;
                            break;
                        case 1:
                            G_CW_pitch = 500.00;
                            G_side_tone_pitch = 500.00f;
                            break;
                        case 2:
                            G_CW_pitch = 600.00;
                            G_side_tone_pitch = 600.00f;
                            break;
                        case 3:
                            G_CW_pitch = 700.00;
                            G_side_tone_pitch = 700.00f;
                            break;
                        case 4:
                            G_CW_pitch = 800.00;
                            G_side_tone_pitch = 800.00f;
                            break;
                    }
                    set_cw_bandwidth(cw_bw, cw_center);
                    if (!cw_mode) {
                        mystate.lastHRFiltHigh = high_cut;
                        mystate.lastHRFiltLow = previous_low_cut;
                    } else {
                        mystate.lastHRFiltHigh = G_cw_high_cut;
                        mystate.lastHRFiltLow = G_cw_low_cut;
                    }
                    mystate.initDSPflag = TRUE;
                    cw_bw_set = FALSE;
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CW_PITCH: ERROR CW Bandwidth NOT SET\n", line_number++);
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CW_PITCH -> Finished\n", line_number++);
                break;

            case CMD_CW_SNAP_SET_DELTA:
                calibration_limits.low_cut = (float) (G_CW_pitch - (float) t_opcode_data);
                calibration_limits.high_cut = (float) (G_CW_pitch + (float) t_opcode_data);
                calibration_limits.center = (float) G_CW_pitch;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CW_SNAP_Delta: %d\n", line_number++, t_opcode_data);
                break;

            case CMD_CW_SNAP_START:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_CW_SNAP_START  \n", line_number++);
                mycalstate.freq_center = calibration_limits.center;
                mycalstate.freq_high = calibration_limits.high_cut;
                mycalstate.freq_low = calibration_limits.low_cut;
                mycalstate.calReady = FALSE;
                mycalstate.calStart = 1;
                CW_snap_update_calibrate_data();
                break;

            case CMD_CW_SNAP_FINISHED:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_CW_SNAP_FINISHED \n", line_number++);
                CW_snap_Send_calibration_data();
                break;

            case CMD_GET_SET_PANADAPTER_SMOOTHING:
                G_Smoothing = t_opcode_data + 2;
                break;

            case CMD_GET_SET_NB_ENABLE:
                NB_Enable = t_opcode_data;
                initBlanker(1, NB_Enable, NB_Threshold, NB_Pulse_Width);
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_NB_ENABLE -> NB_Enable: %d\n", line_number++, NB_Enable);
                break;

            case CMD_GET_SET_NB_THRESHOLD:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_NB_THRESHOLD -> s_opcode_data: %d\n", line_number++, s_opcode_data);
                NB_Threshold = (float) (1000 - s_opcode_data); //Threshold is received as value from 1 to 1000.  This reverses the effect of the GUI slider
                NB_Threshold = (NB_Threshold) / (float) 100.0;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_NB_THRESHOLD -> NB_Threshold: %f\n", line_number++, NB_Threshold);
                if (NB_Enable == 1) {
                    initBlanker(1, NB_Enable, NB_Threshold, NB_Pulse_Width);
                }
                break;

            case CMD_GET_SET_NB_PULSE_WIDTH:
                NB_Pulse_Width = s_opcode_data;
                if (NB_Enable == 1) {
                    initBlanker(1, NB_Enable, NB_Threshold, NB_Pulse_Width);
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> NB_PULSE_WIDTH -> NB_Pulse_Width: %d\n", line_number++, NB_Pulse_Width);
                break;

            case CMD_DELETE_SDRCORE_INIT:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_DELETE_SDRCORE_INIT \n", line_number++);
                delete_ini_file();
                break;

            case CMD_SET_KEEP_ALIVE:
                G_keep_alive++;
                //fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_KEEP_ALIVE -> Called. \n", line_number++);
                //update_calibrate_data(i_opcode_data);
                break;

            case CMD_SET_SDR_CORE_BAND:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_SDR_CORE_BAND -> Called\n", line_number++);
                operating_band = s_opcode_data;
                if (previous_operating_band != operating_band) {
                    Init_IQ_structure();
                    switch (operating_band) {
                        case 2200:
                            iq_offset = G_iq_stack[IQ_B2200].iq_offset;
                            break;
                        case 630:
                            iq_offset = G_iq_stack[IQ_B630].iq_offset;
                            break;
                        case 160:
                            iq_offset = G_iq_stack[IQ_B160].iq_offset;
                            break;
                        case 80:
                            iq_offset = G_iq_stack[IQ_B80].iq_offset;
                            break;
                        case 60:
                            iq_offset = G_iq_stack[IQ_B60].iq_offset;
                            break;
                        case 40:
                            iq_offset = G_iq_stack[IQ_B40].iq_offset;
                            break;
                        case 30:
                            iq_offset = G_iq_stack[IQ_B30].iq_offset;
                            break;
                        case 20:
                            iq_offset = G_iq_stack[IQ_B20].iq_offset;
                            break;
                        case 17:
                            iq_offset = G_iq_stack[IQ_B17].iq_offset;
                            break;
                        case 15:
                            iq_offset = G_iq_stack[IQ_B15].iq_offset;
                            break;
                        case 12:
                            iq_offset = G_iq_stack[IQ_B12].iq_offset;
                            break;
                        case 10:
                            iq_offset = G_iq_stack[IQ_B10].iq_offset;
                            break;
                    }
                    IQ_calc(iq_offset);
                    previous_operating_band = operating_band;
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_SDR_CORE_BAND -> band: %d\n",
                            line_number++, operating_band);
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_SDR_CORE_BAND -> band NOT Changed. band: %d\n",
                            line_number++, operating_band);
                }
                break;

            case CMD_SET_IQ_BAND:
                iq_band = Get_IQ_Record(s_opcode_data);
                G_iq_stack[iq_band].band = iq_band;
                G_iq_stack[iq_band].record = iq_band;
                iq_offset = G_iq_stack[iq_band].iq_offset;
                IQ_calc(iq_offset);
                iq_buffer.opcode = CMD_GET_IQ_VALUE;
                iq_buffer.iq_value = G_iq_stack[iq_band].iq_offset;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_IQ_BAND Called -> IQ_band: %d, IQ_offset: %d\n",
                        line_number++, iq_band, G_iq_stack[iq_band].iq_offset);
                if (sendto(ms_sdr_s, (char *) &iq_buffer, sizeof (iq_buffer), 0,
                        (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_IQ_BAND -> sentto failed with error code : %s\n",
                            line_number++, strerror(errno));
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_IQ_BAND -> Finished -> band: %d\n",
                        line_number++, iq_band);
                break;

            case CMD_SET_IQ_DEFAULTS:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_IQ_DEFAULTS \n", line_number++);
                status = create_iq_ini_file(1);
                Init_IQ_structure();
                break;

            case CMD_SET_COMMIT_IQ:
                if (iq_band != NO_IQ_BAND) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_COMMIT_IQ -> band: %d\n",
                            line_number++, iq_band);
                    Sleep(50);
                    status = update_iq_ini_file();
                    if (status == 1) {
                        Init_IQ_structure();
                    }
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_COMMIT_IQ -> IQ BAND NOT SET\n", line_number++);
                }
                break;

            case CMD_SET_IQ_OFFSET:
                if (iq_band != NO_IQ_BAND) {
                    op_code_data_32 = (int*) &buf[1];
                    memcpy(&i_opcode_data, op_code_data_32, 4);
                    iq_offset = i_opcode_data;
                    G_iq_stack[iq_band].iq_offset = iq_offset;
                    IQ_calc(iq_offset);
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_IQ_OFFSET -> IQ_offset  %d\n", line_number++, iq_offset);
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_COMMIT_IQ -> IQ BAND NOT SET\n", line_number++);
                }
                break;

            case CMD_SET_CALIBRATION_START:
                op_code_data_32 = (int*) &buf[1];
                memcpy(&i_opcode_data, op_code_data_32, 4);
                calibration_limits.low_cut = 598.0f;
                calibration_limits.center = 600.0f;
                calibration_limits.high_cut = 602.0f;
                mycalstate.freq_center = calibration_limits.center;
                mycalstate.freq_high = calibration_limits.high_cut;
                mycalstate.freq_low = calibration_limits.low_cut;
                mycalstate.calReady = FALSE;
                mycalstate.calStart = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CALIBRATION_START -> Called. Frequency: %ld\n", line_number++, i_opcode_data);
                update_calibrate_data(i_opcode_data);
                break;

            case CDM_SET_CALIBRATE_CYCLE_COUNT:
                op_code_data_32 = (int*) &buf[1];
                memcpy(&i_opcode_data, op_code_data_32, 4);
                mycalstate.Cycle_Count = (uint32_t) i_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CDM_SET_CALIBRATE_CYCLE_COUNT -> Called. Cycle Count: %ld\n", line_number++, mycalstate.Cycle_Count);
                break;

            case CMD_SET_CALIBRATION_FINISHED:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CALIBRATION_FINISHED -> Called. \n", line_number++);
                Send_calibration_data();
                break;

                /*case CMD_GET_SET_SPEAKER_DEVICE:
                        device_output_record_index = t_opcode_data;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_SPEAKER_DEVICE -> Calling manage_stream with param: %d \n", line_number++, device_output_record_index);
                        manage_stream(0, G_output_devices[device_output_record_index].device_index, G_output_devices[device_output_record_index].num_channels);
                        Sleep(50);
                        manage_stream(1, G_output_devices[device_output_record_index].device_index, G_output_devices[device_output_record_index].num_channels);
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_SPEAKER_DEVICE -> Calling set_selected_device with param: %d \n", line_number++, device_output_record_index);
                        set_selected_device(device_output_record_index);
                        print_output_devices();
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_GET_SET_SPEAKER_DEVICE -> Calling update_sound_ini \n", line_number++);
                        update_sound_ini();
                        print_output_devices();
                        break;
                 * */

            case CMD_SET_SPEAKER_VOLUME:
                master_volume = (float) t_opcode_data * 0.01f;
                G_volumeLevel = master_volume * volume_attn;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_SPEAKER_VOLUME, t_opcode_data: %d master_volume: %f \n",
                        line_number++, t_opcode_data, master_volume);
                break;

            case CMD_SET_SPEAKER_MUTE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_SPEAKER_MUTE: %d \n", line_number++, t_opcode_data);
                if (t_opcode_data == 1) {
                    if (speaker_muted == FALSE) {
                        G_volumeLevel = 0.0f;
                        speaker_muted = TRUE;
                    }
                } else {
                    if (speaker_muted == TRUE) {
                        G_volumeLevel = master_volume * volume_attn;
                        speaker_muted = FALSE;
                    }
                }
                break;

            case CMD_SET_STOP:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_STOP: %d. sdrcore-recv stopped\n",
                        line_number++, t_opcode_data);
                //Pa_StopStream(stream);
                //Pa_CloseStream(stream);
                G_all_threads_run = 0;
                Sleep(1000);
                break;

            case CMD_SET_MAIN_MODE:
                cw_mode = 0;
                switch (t_opcode_data) {
                    case 0:
                        G_mode = 'A';
                        mystate.opmode = MODE_AM;
                        mystate.lastHRFiltLow = previous_low_cut;
                        mystate.lastHRFiltHigh = previous_high_cut;
                        break;
                    case 1:
                        G_mode = 'L';
                        mystate.opmode = MODE_LSB;
                        mystate.lastHRFiltLow = previous_low_cut;
                        mystate.lastHRFiltHigh = previous_high_cut;
                        break;
                    case 2:
                        G_mode = 'U';
                        mystate.opmode = MODE_USB;
                        mystate.lastHRFiltLow = previous_low_cut;
                        mystate.lastHRFiltHigh = previous_high_cut;
                        break;
                    case 3:
                        G_mode = 'C';
                        cw_mode = 1;
                        mystate.opmode = MODE_USB;
                        mystate.lastHRFiltLow = G_cw_low_cut;
                        mystate.lastHRFiltHigh = G_cw_high_cut;
                        break;
                    case 4:
                        G_mode = 'T';
                        mystate.lastHRFiltLow = previous_low_cut;
                        mystate.lastHRFiltHigh = previous_high_cut;
                        break;
                    case 5:
                        G_mode = 'F';
                        break;
                    case 6:
                        G_mode = 'D';
                        break;
                }
                setFilterOffsets(mystate.lastHRFiltLow, mystate.lastHRFiltHigh);
                mystate.initDSPflag = TRUE;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_MAIN_MODE: New Mode %c\n", line_number++, G_mode);
                break;

            case CMD_SET_CW_BW:
                cw_bw_received = t_opcode_data; //Data is sent inverted
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CW_BW -> Called. cw_bw_recevied: %d\n", line_number++, cw_bw_received);
                cw_bw_set = TRUE;
                switch (cw_bw_received) {
                    case 2:
                        set_cw_bandwidth(0, cw_center);
                        cw_bw = 0;
                        break;
                    case 1:
                        set_cw_bandwidth(1, cw_center);
                        cw_bw = 1;
                        break;
                    case 0:
                        set_cw_bandwidth(2, cw_center);
                        cw_bw = 2;
                        break;
                }
                if (cw_mode) {
                    mystate.lastHRFiltHigh = G_cw_high_cut;
                    mystate.lastHRFiltLow = G_cw_low_cut;
                    setFilterOffsets(mystate.lastHRFiltLow, mystate.lastHRFiltHigh);
                    mystate.initDSPflag = TRUE;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_CW_BW: Index: %d,New filter -> Low: %f, High: %f, CW MODE: %d\n",
                        line_number++, cw_bw, mystate.lastHRFiltLow, mystate.lastHRFiltHigh, cw_mode);
                break;

            case CMD_SET_BW_HICUT:
                switch (t_opcode_data) {
                    case 4:
                        high_cut = 2400.0f;
                        previous_high_cut = high_cut;
                        break;
                    case 3:
                        high_cut = 2700.0f;
                        previous_high_cut = high_cut;
                        break;
                    case 2:
                        high_cut = 3000.0f;
                        previous_high_cut = high_cut;
                        break;
                    case 1:
                        high_cut = 4000.0f;
                        previous_high_cut = high_cut;
                        break;
                    case 0:
                        high_cut = 5500.0f;
                        previous_high_cut = high_cut;
                        break;
                }
                if (!cw_mode) {
                    mystate.lastHRFiltHigh = high_cut;
                    mystate.lastHRFiltLow = previous_low_cut;
                    setFilterOffsets(mystate.lastHRFiltLow, mystate.lastHRFiltHigh);
                    mystate.initDSPflag = TRUE;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_BW_HICUT: New filter -> Low: %f, High: %f, CW MODE: %d\n",
                        line_number++, mystate.lastHRFiltLow, mystate.lastHRFiltHigh, cw_mode);
                break;

            case CMD_SET_BW_LOCUT:
                switch (t_opcode_data) {
                    case 4:
                        low_cut = 75.0f;
                        previous_low_cut = low_cut;
                        break;
                    case 3:
                        low_cut = 100.0f;
                        previous_low_cut = low_cut;
                        break;
                    case 2:
                        low_cut = 200.0f;
                        previous_low_cut = low_cut;
                        break;
                    case 1:
                        low_cut = 300.0f;
                        previous_low_cut = low_cut;
                        break;
                    case 0:
                        low_cut = 500.0f;
                        previous_low_cut = low_cut;
                        break;
                }
                if (!cw_mode) {
                    mystate.lastHRFiltLow = low_cut;
                    mystate.lastHRFiltHigh = previous_high_cut;
                    setFilterOffsets(mystate.lastHRFiltLow, mystate.lastHRFiltHigh);
                    mystate.initDSPflag = TRUE;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread -> CMD_SET_BW_LOCUT: New filter: Low: %f, High %f \n",
                        line_number++, mystate.lastHRFiltLow, mystate.lastHRFiltHigh);
                break;
        }
    }
    if (UDP_status) {
        MessageBoxA(NULL, "UDP Critical UDP Read FAILURE.  SDR-Core is terminating", "SDRCore-Trans", MB_OK | MB_ICONSTOP);
    }
    pthread_exit(0);
    return (NULL);
}
