#include "extern.h"
#include "iq.h"

#define BUFLEN 512  //Max length of buffer

#define SERVER "127.0.0.1"
#define IQ_INCREMENT_DIVISOR 100000.0f
#define MIC_CALIBRATION 0.75f


long long G_keep_alive = 0;
extern sp_float G_peak;

char buf[BUFLEN];
uint16_t G_SDRcore_port = 0;
uint16_t ms_sdr_port = 0;
struct sockaddr_in si_me, si_ms_sdr;
int sdrcore_s, ms_sdr_s, recv_len;
int slen = sizeof (si_ms_sdr);
extern struct input_devices G_input_devices[MAX_INPUT_DEVICES];
extern struct input_devices G_digital_input_devices[MAX_INPUT_DEVICES];
extern power_levels G_power_levels;
extern power_stack G_Proficio_Calibration_Levels[];
extern amplifier_stack G_amp_calibration_stack[];
extern amplifier_stack G_amplifier_stack[];
uint8_t G_PCB_Version = 10;
uint8_t opcode;
uint8_t t_opcode_data;
short int *opcode_data;
short int s_opcode_data;
int *op_code_data_32;
int i_opcode_data;
int G_power_file_needs_updated = 0;
int G_Power_Values_initialized = FALSE;
extern iq_stack G_iq_stack[];
char G_mode = 'L';
uint16_t G_band = 0;
int G_mode_change = 0;
uint8_t G_tx_mode = 0;
int G_calibration_index = 0;
uint8_t G_QRP_mode = 1;
float G_temperature = 0.0f;
uint32_t G_potentia_bias = 0;
uint8_t G_Mia_Status = TRUE;
float G_amplifier_fine_calibration = 1.0000000f;
uint8_t G_audio_mode = 1;
int current_mic_volume = 0;
uint8_t mic_gain_step = 5;
uint8_t mic_digital_gain_step = 5;
sp_float previous_mic_volume = 0.0f;
int G_watts_int = 0;
int G_watts_decimal = 0;
float G_watts = 0.0f;
int8_t G_Do_ALC = TRUE;
int8_t G_Allow_ALC_Send = FALSE;
uint8_t G_QSK = FALSE;
uint8_t record_device_set = FALSE;

struct {

    struct {
        int user_power_value; //Populated from power.ini
        int calibration_value; //Populated from power_cal.ini
    } mode[5];
} proficio_table[12]; //This table is for the Proficio 

struct {

    struct {
        float user_power_value; //Populated from amplifier.ini
        float calibration_value; //Populated from amplifier_cal.ini
    };
} amplifier_table[12]; //This table is for the Potentia

int Setup_UDP(void) {
    int status = 0;
    WSADATA dll_wsa;
    INT32 address = 0;

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
    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP Thread -> Socket Created\n", line_number++);
    }

    // Set up the SDRcore port for receiving UDP packets from any IP address
    // Initialize structures
    if (G_SDRcore_port == 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> Invalid DLL Port. Using default: %d\n", line_number++, SDRCORE_TRANS_PORT);
        G_SDRcore_port = SDRCORE_TRANS_PORT;
    }
    memset((char*)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(G_SDRcore_port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind socket to port
    if (bind(sdrcore_s, (struct sockaddr*)&si_me, sizeof(si_me)) != 0) {
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
    if ((ms_sdr_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP -> ms_sdr_s -> Create Socket: Failed. Error Code : %s\n", line_number++, strerror(errno));
        status = 1;
    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Setup_UDP ->ms_sdr_s -> Socket Created\n", line_number++);
    }
    memset((char*)&si_ms_sdr, 0, sizeof(si_ms_sdr));
    si_ms_sdr.sin_family = AF_INET;
    si_ms_sdr.sin_port = htons(ms_sdr_port);
    //si_ms_sdr.sin_addr.s_addr = inet_addr(SERVER);
    inet_pton(AF_INET, SERVER, &address);
    si_ms_sdr.sin_addr.s_addr = address;

    return status;
}

void Init_Proficio_table() {
    int band = 0;
    int mode = 0;

    for (band = 0; band < 12; band++) {
        for (mode = 0; mode < 5; mode++) {
            proficio_table[band].mode[mode].calibration_value = G_Proficio_Calibration_Levels[band].power_level;
            switch (mode) {
                case AM_POWER:
                    proficio_table[band].mode[mode].user_power_value = G_power_levels.lsb_power;
                    break;
                case CW_POWER:
                    proficio_table[band].mode[mode].user_power_value = G_power_levels.cw_power;
                    break;
                case LSB_POWER:
                    proficio_table[band].mode[mode].user_power_value = G_power_levels.lsb_power;
                    break;
                case USB_POWER:
                    proficio_table[band].mode[mode].user_power_value = G_power_levels.usb_power;
                    break;
                case TUNE_POWER:
                    proficio_table[band].mode[mode].user_power_value = G_power_levels.tune_power;
                    break;
            }
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] Init_Proficio_table. Band: %d, Mode: %d, calibration_value %d, user_power_level: %d\n",
                line_number++, band, mode, proficio_table[band].mode[mode].calibration_value,
                proficio_table[band].mode[mode].user_power_value);
    }
}

void Init_amplifier_table() {
    int band = 0;
    int power_level = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] Init_amplifier_table. STARTED\n",line_number++);
    for (band = 0; band < 12; band++) {
        amplifier_table[band].user_power_value = (float) (G_amplifier_stack[band].power_level) * 0.01f;
        power_level = G_amp_calibration_stack[band].power_level;
        amplifier_table[band].calibration_value = 1.0f + ((float) power_level / 100.0f);
        print_time();
        fprintf(G_fp_logfile, "[%d] Init_amplifier_table. Band: %d, user_power_level: %f, calibration_value: %f\n",
                line_number++, band, amplifier_table[band].user_power_value, amplifier_table[band].calibration_value);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Init_amplifier_table. FINISHED\n", line_number++);
}

void Init_Power_All(void) {
    print_time();
    fprintf(G_fp_logfile, "[%d] Init_Power_All. STARTED \n", line_number++);
    Init_Proficio_User_power();
    Init_Proficio_calibration(0);
    Init_Proficio_table();
    Init_amplifier_user_values();
    Init_amplifier_calibration();
    Init_amplifier_table();
    print_time();
    G_Power_Values_initialized = TRUE;
    print_time();
    fprintf(G_fp_logfile, "[%d] Init_Power_All. Finished \n", line_number++);
}

float set_QRO_power_level(int band, int mode, int user_power_value) {
    float drive_level = 0.0f;

    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRO_power_level. STARTED. band:%d, mode:%d, user_power: %d \n",
            line_number++, band, mode, user_power_value);
    drive_level = (float) ((float) (proficio_table[band].mode[mode].calibration_value *
            proficio_table[band].mode[mode].user_power_value *
            amplifier_table[band].calibration_value * 100.00f) * 0.0001f);
    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRO_power_level. Proficio Calibration value: %d, \n",
            line_number++, proficio_table[band].mode[mode].calibration_value);
    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRO_power_level. Amplifier Calibration value: %f, \n",
        line_number++, amplifier_table[band].calibration_value);
    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRO_power_level. Finished. drive_level: %f \n", line_number++, drive_level);
    return drive_level;
}

float set_QRP_power_level(int band, int mode, int user_power_value) {
    float power_level = 0.0f;

    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRP_power_level. STARTED \n", line_number++);
    power_level = (float) ((float) (proficio_table[band].mode[mode].calibration_value *
            proficio_table[band].mode[mode].user_power_value) * 0.0001f);
    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRP_power_level. Finished. Band: %d, Mode: %d, Proficio Calibration: %d, "
            "Proficio User Power: %d, Proficio Drive Value: %f\n",
            line_number++, band, mode, proficio_table[band].mode[mode].calibration_value,
            proficio_table[band].mode[mode].user_power_value, power_level);
    print_time();
    fprintf(G_fp_logfile, "[%d] set_QRP_power_level. FINISHED \n", line_number++);
    return power_level;
}

float get_QRO_power_level(int band, int mode) {
    float drive_level = 0.0f;
    float amplifier_calibration_value = 0.0;

    print_time();
    fprintf(G_fp_logfile, "[%d] get_QRO_power_level. STARTED. band: %d, mode: %d \n", line_number++, band, mode);
    if (amplifier_table[band].calibration_value == 0.0) {
        amplifier_calibration_value = 1.0;
    } else {
        amplifier_calibration_value = amplifier_table[band].calibration_value;
    }
    drive_level = (float) ((float) (proficio_table[band].mode[mode].calibration_value *
            proficio_table[band].mode[mode].user_power_value * amplifier_calibration_value) * 0.0001f);
    print_time();
    fprintf(G_fp_logfile, "[%d] get_QRO_power_level. FINISHED. Calibration: %f, drive_level: %f \n",
            line_number++, amplifier_calibration_value, drive_level);
    return drive_level;
}

float get_QRP_power_level(int band, int mode) {
    float power_level = 0.0f;

    power_level = (float) ((float) (proficio_table[band].mode[mode].calibration_value *
            proficio_table[band].mode[mode].user_power_value) * 0.0001f);
    print_time();
    fprintf(G_fp_logfile, "[%d] get_QRP_power_level. Finished. Band: %d, Mode: %d, Proficio Drive Value: %d, user_power_value: %d, calibration_value: %f\n",
            line_number++, band, mode, proficio_table[band].mode[mode].calibration_value,
            proficio_table[band].mode[mode].user_power_value, power_level);
    return power_level;
}

void Set_Mic_Volume() {
    switch (mic_gain_step) {
        case 0:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 0.500000f);
            break;
        case 1:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 0.700000f);
            break;
        case 2:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 0.900000f);
            break;
        case 3:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 1.1000000f);
            break;
        case 4:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 1.200000f);
            break;
        case 5:
            G_mic_volume = (((float) (((float) current_mic_volume)) / 100.000000000000000f) * 1.300000f);
            break;
    }
    previous_mic_volume = G_mic_volume;
    print_time();
    fprintf(G_fp_logfile, "[%d] Set_Mic_Volume. mic_gain_step: %d, current_mic_volume: %d, G_mic_volume: %f\n",
            line_number++, mic_gain_step, current_mic_volume, G_mic_volume);
}

void Set_Digital_Mic_Volume() {
    switch (mic_digital_gain_step) {
    case 0:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 0.500000f);
        break;
    case 1:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 0.700000f);
        break;
    case 2:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 0.900000f);
        break;
    case 3:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 1.1000000f);
        break;
    case 4:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 1.200000f);
        break;
    case 5:
        G_mic_volume = (((float)(((float)current_mic_volume)) / 100.000000000000000f) * 1.300000f);
        break;
    }
    previous_mic_volume = G_mic_volume;
    print_time();
    fprintf(G_fp_logfile, "[%d] Set_Digital_Mic_Volume. mic_gain_step: %d, current_mic_volume: %d, G_mic_volume: %f\n",
        line_number++, mic_gain_step, current_mic_volume, G_mic_volume);
}

void Set_Mic_Mute(uint8_t mic_status) {

    if (mic_status == 1) {
        G_mic_volume = 0.0f;
    } else {
        G_mic_volume = previous_mic_volume;
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Set_Mic_Mute. mic_status: %d, G_mic_volume: %f\n",
            line_number++, mic_status, G_mic_volume);
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
    float freq = 0.0f;
    sp_float high_cut = 2700.0f;
    sp_float low_cut = 75.0f;
    int iq_band = NO_IQ_BAND;
    int iq_offset = 0;
    uint32_t operating_band = 0;
    uint32_t previous_operating_band = 0;
    uint8_t mic_muted = 0;
    int device_input_record_index = 0;
    uint8_t UDP_status = 0;
    int transceiver_calibration_index = 0;
    int transceiver_calibration_band = 0;
    int amp_calibration_band = 0;
    uint8_t mode = 0;
    float temp_temp = 0.0f;
    uint8_t upper_byte = 0;
    uint8_t lower_byte = 0;
    int potentia_temp = 0;
    uint32_t potentia_bias = 0;
    int8_t amplifier_fine = 1;
    uint8_t previous_G_tx_mode = 0;
    int watts = 0;
    uint8_t stream_running = TRUE;
    int stream_status = 0;

#pragma pack(1)

    struct {
        uint8_t opcode;
        int iq_value;
    } iq_buffer;

    status = Setup_UDP();
    if (status == 0) {
        G_network_initialized = 1;
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread. Running\n", line_number++);
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread. Waiting for data...\n", line_number++);
    } else {
        printf("UDP Setup_UDP Failed\n");
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread. Socket Create failed. UDP Thread exiting\n", line_number++);
        G_all_threads_run = 0;
        pthread_exit(NULL);
    }

    Sleep(1000);
    mystate.txPower = 0.0f;
    mystate.amCarrier = (sp_float) (G_power_levels.am_power) * 0.01f;
    print_time();
    fprintf(G_fp_logfile, "[%d] UDP Thread. G_Do_ALC: %d, G_ALC_multiplier: %f\n", line_number++, G_Do_ALC, G_ALC_multiplier);
    iq_band = NO_IQ_BAND;
    while (G_all_threads_run) {
        if ((recv_len = recvfrom(sdrcore_s, buf, BUFLEN, 0, (struct sockaddr *) &si_me, &slen)) == SOCKET_ERROR) {
            print_time();
            fprintf(G_fp_logfile, "[%d] UDP Thread. recvfrom Failed. Error Code : %s. UDP Thread Exiting\n", line_number++,
                    strerror(errno));
            UDP_status = 1;
            G_all_threads_run = 0;
        }
        opcode = (uint8_t) buf[0];
        t_opcode_data = (uint8_t) (buf[1]);
        opcode_data = (short int*) &buf[1];
        memcpy(&s_opcode_data, opcode_data, 2);
        memcpy(&i_opcode_data, opcode_data, 4);

        if (mystate.txPower > 1.0f) {
            print_time();
            fprintf(G_fp_logfile, "[%d] UDP Thread. WARNING: mystate.txPower > 1.0f.  Value: %f \n",
                    line_number++, mystate.txPower);
        }

        switch (opcode) {
        case CMD_SET_AUDIO_DEVICE:
            print_time();
            fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AUDIO_DEVICE. t_opcode_data: %d\n",
                line_number++, t_opcode_data);
            switch (t_opcode_data) {
            case DIGITAL_AUDIO:
                G_audio_mode = DIGITAL_AUDIO;
                stream_status = manage_stream(0, G_input_devices[G_input_device_index].device_index,
                    G_input_devices[G_input_device_index].num_channels);
                stream_status = manage_stream(1, G_digital_input_devices[G_digital_input_device_index].device_index,
                    G_digital_input_devices[G_digital_input_device_index].num_channels);
                break;
            case OPERATOR_AUDIO:
                G_audio_mode = OPERATOR_AUDIO;
                stream_status = manage_stream(0, G_digital_input_devices[G_digital_input_device_index].device_index,
                    G_digital_input_devices[G_digital_input_device_index].num_channels);
                stream_status = manage_stream(1, G_input_devices[G_input_device_index].device_index,
                    G_input_devices[G_input_device_index].num_channels);
                break;
            }
            break;

            case SET_SEMI_BREAKIN:
                G_QSK = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. SET_SEMI_BREAKIN. G_QSK: %d \n", line_number++, G_QSK);
                break;

            case CMD_SET_FOWARD_POWER_VALUE:
                memcpy(&i_opcode_data, opcode_data, 4);
                G_watts_decimal = (int) (i_opcode_data & 0x000000FF);
                G_watts_int = (int) (i_opcode_data >> 10);
                if (G_watts_decimal >= 100) {
                    G_watts = (float) ((float) G_watts_int + ((float) (G_watts_decimal / 1000.0f)));
                } else {
                    G_watts = (float) ((float) G_watts_int + ((float) (G_watts_decimal / 100.0f)));
                }
                //print_time();
                //fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_FOWARD_POWER_VALUE. G_watts_int: %d,G_watts_decimal: %d,G_watts: %f\n",
                //        line_number++, G_watts_int, G_watts_decimal, G_watts);
                break;

            case CMD_SET_ALC_MULTIPLIER:
                if (t_opcode_data == 0) {
                    G_Do_ALC = FALSE;
                } else {
                    G_ALC_multiplier = (float) 11.0 / 10.0f;
                    G_Do_ALC = TRUE;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_ALC_MULTIPLIER. received_data: %d, G_ALC_multiplier: %f, G_Do_ACL: %d\n",
                    line_number++, t_opcode_data, G_ALC_multiplier, G_Do_ALC);
                break;

            case CMD_SET_MIC_GAIN:
                //mic_gain_step = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_GAIN. mic_gain_step: %d. NOOP\n",
                        line_number++, mic_gain_step);
                //Set_Mic_Volume();
                break;

            case CMD_SET_DIGITAL_MIC_GAIN:
                //mic_digital_gain_step = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_DIGITAL_MIC_GAIN. mic_digital_gain_step: %d.  NOOP\n",
                    line_number++, mic_digital_gain_step);
                //Set_Digital_Mic_Volume();
                break;

            case CMD_SET_SDRCORE_TRANS_INITIALIZE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_SDRCORE_TRANS_INITIALIZE. STARTED State: %d \n", line_number++,
                        t_opcode_data);
                Init_Power_All();
                break;

            case CMD_GET_MIA_STATUS:
                G_Mia_Status = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_MIA_STATUS. Mia_status: %d \n", line_number++, G_Mia_Status);
                break;

            case CMD_SET_MICROPHONE_STATUS:
                break;

            case CMD_GET_POTENTIA_TEMPERATURE:
                op_code_data_32 = (int*) &buf[1];
                memcpy(&i_opcode_data, op_code_data_32, 4);
                potentia_temp = i_opcode_data;
                upper_byte = potentia_temp >> 8;
                lower_byte = (uint8_t) potentia_temp;
                temp_temp = (float) lower_byte / 16.0f;
                G_temperature = ((float) upper_byte * 16) + temp_temp;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_POTENTIA_TEMPERATURE: %f\n",
                        line_number++, G_temperature);
                G_temperature_received = TRUE;
                break;

            case CMD_GET_POTENTIA_BIAS:
                op_code_data_32 = (int*) &buf[1];
                memcpy(&i_opcode_data, op_code_data_32, 4);
                potentia_bias = i_opcode_data;
                G_potentia_bias = potentia_bias;
                G_bias_received = TRUE;
                break;

            case CMD_SET_PA_BYPASS:
                if (t_opcode_data == 0) {
                    G_QRP_mode = 1;
                } else {
                    G_QRP_mode = 0;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_PA_BYPASS. G_QRP_mode: %d\n", line_number++, G_QRP_mode);
                break;

                //Amplifier Routines
            case CMD_SET_AMPLIFIER_CALIBRATION_RESET:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AMPLIFIER_CALIBRATION_RESET. received_data: %d\n",
                        line_number++, t_opcode_data);
                //Create_amplifier_calibration_file(t_opcode_data);
                //G_power_file_needs_updated = TRUE;
                break;

            case CMD_AMPLIFIER_TUNE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_AMPLIFIER_TUNE. NOOP\n",
                        line_number++);
                break;

            case CMD_GET_AMP_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_AMP_POWER. NOOP\n", line_number++);
                break;

            case CMD_SET_AMPLIFIER_BAND:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AMPLIFIER_BAND. NOOP\n", line_number++);
                break;

            case CMD_SET_POTENTIA_CALIBRATION:
                op_code_data_32 = (int*) &buf[1];
                memcpy(&i_opcode_data, op_code_data_32, 4);
                amplifier_fine = i_opcode_data;
                G_amp_calibration_stack[amp_calibration_band].power_level = amplifier_fine;
                amplifier_table[amp_calibration_band].calibration_value = amplifier_fine;
                G_power_file_needs_updated = TRUE;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_POTENTIA_CALIBRATION Band: %d, Calibration Value: %d\n",
                        line_number++, amp_calibration_band, amplifier_fine);
                break;

            case CMD_SET_AMPLIFIER_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AMPLIFIER_POWER. STARTED. Power: %d\n", line_number++,t_opcode_data);
                amplifier_table[amp_calibration_band].user_power_value = (float) (t_opcode_data) * 0.01f;
                G_power_file_needs_updated = TRUE;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AMPLIFIER_POWER. Finished. Amplifier Band: %d, Mode: %d, Power: %d\n",
                        line_number++, amp_calibration_band, mode, t_opcode_data);
                break;
                //End of Amplifier Routines

                //The following commands are for calibrating the Proficio or the Geminus
            case CMD_SET_BAND_POWER_BAND:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_BAND_POWER_BAND. Called\n", line_number++);
                transceiver_calibration_band = s_opcode_data;
                switch (transceiver_calibration_band) {
                    case 2200:
                        transceiver_calibration_index = POWER_B2200;
                        break;
                    case 630:
                        transceiver_calibration_index = POWER_B630;
                        break;
                    case 160:
                        transceiver_calibration_index = POWER_B160;
                        break;
                    case 80:
                        transceiver_calibration_index = POWER_B80;
                        break;
                    case 60:
                        transceiver_calibration_index = POWER_B60;
                        break;
                    case 40:
                        transceiver_calibration_index = POWER_B40;
                        break;
                    case 30:
                        transceiver_calibration_index = POWER_B30;
                        break;
                    case 20:
                        transceiver_calibration_index = POWER_B20;
                        break;
                    case 17:
                        transceiver_calibration_index = POWER_B17;
                        break;
                    case 15:
                        transceiver_calibration_index = POWER_B15;
                        break;
                    case 12:
                        transceiver_calibration_index = POWER_B12;
                        break;
                    case 10:
                        transceiver_calibration_index = POWER_B10;
                        break;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_BAND_POWER_BAND. transceiver_calibration_band: %d, index: %d\n",
                        line_number++, transceiver_calibration_band, transceiver_calibration_index);
                break;

            case CMD_SET_BAND_POWER_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_BAND_POWER_POWER. Called\n", line_number++);
                proficio_table[transceiver_calibration_index].mode[mode].calibration_value = t_opcode_data;
                G_power_file_needs_updated = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_BAND_POWER_POWER. Finished. Calibration Band: %d, Mode: %d, Power: %d\n",
                        line_number++, transceiver_calibration_band, mode, t_opcode_data);
                break;
                //End of commands for calibrating the Proficio

            case CMD_SET_PCB_VERSION:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_PCB_VERSION. Version: %d\n", line_number++, t_opcode_data);
                G_PCB_Version = t_opcode_data;
                //status = Check_iq_ini_file();
                status = init_IQ_structure();
                if (status == 1) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP_Thread. init_IQ_structure . Successful \n", line_number++);
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP_Thread. init_IQ_structure. Failed\n", line_number++);
                }
                //Create_amplifier_calibration_file(FALSE);
                //Init_Power_All();
                break;

            case CMD_SET_TWO_TONE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_TWO_TONE. state: %d\n", line_number++, t_opcode_data);
                mystate.twoToneFlag = t_opcode_data;
                mystate.initDSPflag = 1;
                break;

            case CMD_SET_COMPRESSION_STATE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_COMPRESSION_STATE. state: %d\n", line_number++, t_opcode_data);
                micstate.enabled = t_opcode_data;
                break;

            case CMD_SET_COMPRESSION_LEVEL:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_COMPRESSION_LEVEL. level: %d\n", line_number++, t_opcode_data);
                micstate.pregain = (sp_float) (pow(10.0f, (sp_float) t_opcode_data / 20.0f));
                break;

            case CMD_DELETE_SDRCORE_INIT:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_DELETE_SDRCORE_INIT. level: %d\n", line_number++, t_opcode_data);
                break;

            case CMD_SET_KEEP_ALIVE:
                G_keep_alive++;
                break;

            case CMD_SET_SDR_CORE_BAND:
                operating_band = s_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_SDR_CORE_BAND. Called, operating_band: %d\n",
                        line_number++, operating_band);
                if (previous_operating_band != operating_band) {
                    init_IQ_structure();
                    switch (operating_band) {
                        case 2200:
                            iq_offset = G_iq_stack[IQ_B2200].iq_offset;
                            G_calibration_index = POWER_B2200;
                            G_band = POWER_B2200;
                            amp_calibration_band = G_band;
                            break;
                        case 630:
                            iq_offset = G_iq_stack[IQ_B630].iq_offset;
                            G_calibration_index = POWER_B630;
                            G_band = POWER_B630;
                            amp_calibration_band = G_band;
                            break;
                        case 160:
                            iq_offset = G_iq_stack[IQ_B160].iq_offset;
                            G_calibration_index = POWER_B160;
                            G_band = POWER_B160;
                            amp_calibration_band = G_band;
                            break;
                        case 80:
                            iq_offset = G_iq_stack[IQ_B80].iq_offset;
                            G_calibration_index = POWER_B80;
                            G_band = POWER_B80;
                            amp_calibration_band = G_band;
                            break;
                        case 60:
                            iq_offset = G_iq_stack[IQ_B60].iq_offset;
                            G_calibration_index = POWER_B60;
                            G_band = POWER_B60;
                            amp_calibration_band = G_band;
                            break;
                        case 40:
                            iq_offset = G_iq_stack[IQ_B40].iq_offset;
                            G_calibration_index = POWER_B40;
                            G_band = POWER_B40;
                            amp_calibration_band = G_band;
                            break;
                        case 30:
                            iq_offset = G_iq_stack[IQ_B30].iq_offset;
                            G_calibration_index = POWER_B30;
                            G_band = POWER_B30;
                            amp_calibration_band = G_band;
                            break;
                        case 20:
                            iq_offset = G_iq_stack[IQ_B20].iq_offset;
                            G_calibration_index = POWER_B20;
                            G_band = POWER_B20;
                            amp_calibration_band = G_band;
                            break;
                        case 17:
                            iq_offset = G_iq_stack[IQ_B17].iq_offset;
                            G_calibration_index = POWER_B17;
                            G_band = POWER_B17;
                            amp_calibration_band = G_band;
                            break;
                        case 15:
                            iq_offset = G_iq_stack[IQ_B15].iq_offset;
                            G_calibration_index = POWER_B15;
                            G_band = POWER_B15;
                            amp_calibration_band = G_band;
                            break;
                        case 12:
                            iq_offset = G_iq_stack[IQ_B12].iq_offset;
                            G_calibration_index = POWER_B12;
                            G_band = POWER_B12;
                            amp_calibration_band = G_band;
                            break;
                        case 10:
                            iq_offset = G_iq_stack[IQ_B10].iq_offset;
                            G_calibration_index = POWER_B10;
                            G_band = POWER_B10;
                            amp_calibration_band = G_band;
                            break;
                    }
                    IQ_calc(iq_offset);
                    previous_operating_band = operating_band;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_SDR_CORE_BAND. Finished. band: %d\n",
                        line_number++, G_band);
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
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_IQ_BAND Called. IQ_band: %d, IQ_offset: %d\n",
                        line_number++, iq_band, G_iq_stack[iq_band].iq_offset);
                if (sendto(ms_sdr_s, (char *) &iq_buffer, sizeof (iq_buffer), 0, (struct sockaddr *) &si_ms_sdr, slen) ==
                        SOCKET_ERROR) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_IQ_BAND. sentto failed with error code : %s\n",
                            line_number++, strerror(errno));
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_IQ_BAND. Finished. band: %d\n", line_number++, iq_band);
                break;

            case CMD_SET_COMMIT_IQ:
                if (iq_band != NO_IQ_BAND) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_COMMIT_IQ. band: %d\n", line_number++, iq_band);
                    Sleep(50);
                    status = update_iq_ini_file();
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_COMMIT_IQ. IQ BAND NOT SET\n", line_number++);
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
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_IQ_OFFSET. IQ_offset  %d\n", line_number++, iq_offset);
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_COMMIT_IQ. IQ BAND NOT SET\n", line_number++);
                }
                break;

            case CMD_SET_IQ_DEFAULTS:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_IQ_DEFAULTS. Called\n", line_number++);
                status = delete_iq_ini_file();
                if (status == 1) {
                    status = Check_iq_ini_file();
                    if (status == 1) {
                        status = init_IQ_structure();
                        if (status == 1) {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP_Thread. init_IQ_structure . Successful \n", line_number++);
                        } else {
                            print_time();
                            fprintf(G_fp_logfile, "[%d] UDP_Thread. init_IQ_structure. Failed\n", line_number++);
                        }
                    }
                }
                break;

            case CMD_SET_TX_ON:
                G_tx_mode = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_TX_ON. G_tx_mode: %d\n", line_number++,G_tx_mode);
                if (previous_G_tx_mode != G_tx_mode) {
                    switch (G_tx_mode) {
                        case 0:
                            //status = manage_stream(0, G_input_devices[G_input_device_index].device_index,
                            //        G_input_devices[G_input_device_index].num_channels);
                            //Pa_StopStream(stream);
                            Set_Mic_Mute(TRUE); //Turn off the microphone
                            G_null_count = 0;
                            break;
                        case 1:
                            //status = manage_stream(1, G_input_devices[G_input_device_index].device_index,
                            //        G_input_devices[G_input_device_index].num_channels);
                            //Pa_StartStream(stream);
                            Set_Mic_Mute(FALSE); //Turn ON the microphone
                            break;
                    }
                    previous_G_tx_mode = G_tx_mode;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_TX_ON. G_tx_mode: %d. FINISHED\n", line_number++, G_tx_mode);
                
                break;

            case CMD_GET_SET_MIC_DEVICE:
                device_input_record_index = t_opcode_data;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_SET_MIC_DEVICE. Calling manage_stream with param: %d \n", line_number++, device_input_record_index);

                manage_stream(0, G_input_devices[device_input_record_index].device_index, G_input_devices[device_input_record_index].num_channels);
                Sleep(50);
                manage_stream(1, G_input_devices[device_input_record_index].device_index, G_input_devices[device_input_record_index].num_channels);
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_SET_MIC_DEVICE. Calling set_selected_device with param: %d \n", line_number++, device_input_record_index);
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_SET_MIC_DEVICE. Calling update_sound_ini \n", line_number++);
                record_device_set = TRUE;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_GET_SET_MIC_DEVICE. Finished \n", line_number++);
                break;

            case CMD_SET_MIC_VOLUME:
                if (t_opcode_data > 100) {
                    t_opcode_data = 100;
                }
                current_mic_volume = t_opcode_data;
                switch (G_audio_mode) {
                case OPERATOR_AUDIO:
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_VOLUME.  volume: %d, Calling Set_Mic_Volume\n",
                        line_number++, current_mic_volume);
                    Set_Mic_Volume();
                    break;
                case DIGITAL_AUDIO:
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_VOLUME.  volume: %d, Calling Set_Digital_Mic_Volume\n",
                        line_number++, current_mic_volume);
                    Set_Digital_Mic_Volume();
                    break;
                }
                if (G_mode == 'U' || G_mode == 'L') {
                    mystate.initDSPflag = 1;
                    print_time();
                    fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_VOLUME in U/L mode. mystate.txPower: %f, G_mic_volume %f\n",
                            line_number++, mystate.txPower, G_mic_volume);
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_VOLUME. Finished. New Mic Volume: %f, peek: %f\n",
                        line_number++, G_mic_volume, G_peak);
                break;

            case CMD_SET_MIC_MUTE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_MUTE: %d \n", line_number++, t_opcode_data);
                Set_Mic_Mute(t_opcode_data);
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MIC_MUTE. Finished\n", line_number++);
                break;

                //If the subsystems and Proficio are in TUNE mode, set power levels so the new output power levels are produced immediately
                //Set the AM base carrier level
            case CMD_SET_AM_CARRIER:
                setFilterOffsets(low_cut, high_cut);
                mystate.amCarrier = (float) (t_opcode_data) * 0.01f;
                mystate.initDSPflag = 1;
                G_power_file_needs_updated = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_AM_CARRIER. New Power: %f\n", line_number++, mystate.amCarrier);
                break;

            case CMD_SET_SSB_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_SSB_POWER. power: %d \n",
                        line_number++, t_opcode_data);
                set_selected_power(USB_POWER, t_opcode_data);
                set_selected_power(LSB_POWER, t_opcode_data);
                set_QRO_power_level(G_calibration_index, USB_POWER, t_opcode_data);
                set_QRO_power_level(G_calibration_index, LSB_POWER, t_opcode_data);
                set_QRP_power_level(G_calibration_index, USB_POWER, t_opcode_data);
                set_QRP_power_level(G_calibration_index, LSB_POWER, t_opcode_data);
                set_QRP_power_level(G_calibration_index, USB_POWER, t_opcode_data);
                set_QRO_power_level(G_calibration_index, LSB_POWER, t_opcode_data);
                if (G_mode == 'L' || G_mode == 'U') {
                    setFilterOffsets(low_cut, high_cut);
                    mystate.initDSPflag = 1;
                }
                G_power_file_needs_updated = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_SSB_POWER. Finished. Mic Volume: %f\n",
                        line_number++, G_mic_volume);
                break;

            case CMD_SET_CW_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_CW_POWER. power: %d \n",
                        line_number++, t_opcode_data);
                set_selected_power(CW_POWER, t_opcode_data);
                set_QRO_power_level(G_calibration_index, CW_POWER, t_opcode_data);
                set_QRP_power_level(G_calibration_index, CW_POWER, t_opcode_data);
                if (G_mode == 'C') {
                    setFilterOffsets(low_cut, high_cut);
                    mystate.initDSPflag = 1;
                }
                G_power_file_needs_updated = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_CW_POWER. Finished.\n",
                        line_number++);
                break;

            case CMD_SET_TUNE_POWER:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_TUNE_POWER. power: %d, G_calibration_index: %d \n",
                        line_number++, t_opcode_data, G_calibration_index);
                set_selected_power(TUNE_POWER, t_opcode_data);
                set_QRO_power_level(G_calibration_index, TUNE_POWER, t_opcode_data);
                set_QRP_power_level(G_calibration_index, TUNE_POWER, t_opcode_data);
                if (G_mode == 'T') {
                    setFilterOffsets(low_cut, high_cut);
                    mystate.initDSPflag = 1;
                }
                G_power_file_needs_updated = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_TUNE_POWER. Finished.\n",
                        line_number++);
                break;

            case CMD_SET_MAIN_MODE:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE. Called\n", line_number++);
                G_Allow_ALC_Send = FALSE;
                mode = t_opcode_data;
                switch (t_opcode_data) {
                    case 0:
                        G_mode = 'A';
                        mystate.opmode = MODE_AM;
                        G_Allow_ALC_Send = TRUE;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE: New Mode: %c,\n", line_number++,
                                G_mode);
                        break;
                    case 1:
                        G_mode = 'L';
                        mystate.opmode = MODE_LSB;
                        G_Allow_ALC_Send = TRUE;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE: New Mode: %c, Mic Volume: %f\n",
                                line_number++, G_mode, G_mic_volume);
                        break;
                    case 2:
                        G_mode = 'U';
                        mystate.opmode = MODE_USB;
                        G_Allow_ALC_Send = TRUE;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE: New Mode: %c, Mic Volume: %f\n",
                                line_number++, G_mode, G_mic_volume);
                        break;
                    case 3:
                        G_mode = 'C';
                        mystate.opmode = MODE_CW;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE: New Mode: %c,\n", line_number++,
                                G_mode);
                        break;
                    case 4:
                        G_mode = 'T';
                        mystate.opmode = MODE_TUNE;
                        print_time();
                        fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE: New Mode: %c,\n", line_number++,
                                G_mode);
                        break;
                    case 5:
                        G_mode = 'F';
                        break;
                    case 6:
                        G_mode = 'D';
                        break;
                }
                setFilterOffsets(low_cut, high_cut);
                mystate.initDSPflag = 1;
                G_mode_change = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_MAIN_MODE. Finished\n", line_number++);
                break;

            case CMD_SET_TX_HICUT:
                switch (t_opcode_data) {
                    case 0:
                        high_cut = 2400.0f;
                        break;
                    case 1:
                        high_cut = 2700.0f;
                        break;
                    case 2:
                        high_cut = 3000.0f;
                        break;
                    case 3:
                        high_cut = 5500.0f;
                        break;
                }
                setFilterOffsets(low_cut, high_cut);
                mystate.initDSPflag = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_BW: New filter: Low: %f, High: %f\n", line_number++, low_cut, high_cut);
                break;
            case CMD_SET_STOP:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. CMD_SET_STOP: %d. sdrcore-trans stopped\n", line_number++, t_opcode_data);
                Update_amplifier_calibration();
                G_all_threads_run = 0;
                break;
            default:
                print_time();
                fprintf(G_fp_logfile, "[%d] UDP Thread. UNKNOWN OP CODE: 0x%X\n", line_number++, opcode);
                break;
        }
    }
    if (UDP_status) {
        print_time();
        fprintf(G_fp_logfile, "[%d] UDP Thread. UPD Read FAILED\n", line_number++);
        MessageBoxA(NULL, "UDP Critical UDP Read FAILURE.  SDR-Core is terminating", "SDRCore-Trans", MB_OK | MB_ICONSTOP);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] UDP Thread. Exit\n", line_number++);
    pthread_exit(0);
    return (NULL);
}
