#include "extern.h"
//#include "commands.h"
//#include "version.h"

extern struct input_devices G_input_devices[MAX_INPUT_DEVICES];
extern struct input_devices G_digital_input_devices[MAX_INPUT_DEVICES];
int num_input_devices_found = 0;
int num_digital_input_devices_found = 0;
int sound_ini_file_exists = 0;
char G_operator_audio_device[PATH_MAX] = {0};
char G_digital_audio_device[PATH_MAX] = { 0 };

int Get_Digital_Sound_Device() {
    FILE* fp_Sound_ini;
    char l_path[PATH_MAX] = { 0 };
    char init_record[PATH_MAX] = { 0 };
    int status = TRUE;
    char* end_of_file;
    int record_length = 0;
    const char* homedir;
   
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/digital-microphone.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. STARTED. sound-device.ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof(init_record), fp_Sound_ini);
            if (end_of_file != NULL) {
                //strncpy(audio_device, init_record,(strlen(init_record) - 1));
                record_length = strlen(init_record);
                strncpy(G_digital_audio_device, init_record, (record_length));
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. input record: %s, audio_device: %s\n",
                    line_number++, init_record, G_digital_audio_device);
                fclose(fp_Sound_ini);
            }
            else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. FAIL: No record found\n", line_number++);
                status = FALSE;
            }
        }
        else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. FAIL: file not found\n", line_number++);
            status = FALSE;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. FINISHED\n", line_number++);
    }
    return status;
}

int Get_Operator_Sound_Device() {
    FILE *fp_Sound_ini;
    char l_path[PATH_MAX] = {0};
    char init_record[PATH_MAX] = {0};
    int status = 0;
    char *end_of_file;
    const char* homedir;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/operator-microphone.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Operator_Sound_Device. STARTED. sound-device.ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            if (end_of_file != NULL) {
                strncpy(G_operator_audio_device, init_record, (strlen(init_record)));
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Operator_Sound_Device. input record: %s, audio_device: %s\n",
                        line_number++, init_record, G_operator_audio_device);
                fclose(fp_Sound_ini);
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Operator_Sound_Device. FAIL: No record found\n", line_number++);
            }
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Get_Operator_Sound_Device. FAIL: file not found\n", line_number++);
        }
    }
    return status;
}

void build_digital_input_devices(int device_index) {
    static int index = 0;
    char* input_device;
    int found = FALSE;

    if (index < MAX_INPUT_DEVICES) {
        input_device = strstr(lpInfo->name, G_digital_audio_device);
        if (input_device != NULL) {
            G_digital_input_device_index = index;
            found = TRUE;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_digital_input_devices. %s FOUND\n", line_number++, G_digital_audio_device);
        }
        strcpy(G_digital_input_devices[index].name, lpInfo->name);
        G_digital_input_devices[index].device_index = device_index;
        G_digital_input_devices[index].num_channels = lpInfo->maxInputChannels;
        num_digital_input_devices_found++;
        G_digital_input_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultInputDevice) {
            G_digital_input_devices[index].default_input_device = 1;
        }
        index++;
        num_digital_input_devices_found = index;
    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] build_digital_input_devices. Number of input devices exceed permitted limit\n",
            line_number++);
    }
}
void build_input_devices(int device_index) {
    static int index = 0;
    char *input_device;

    if (index < MAX_INPUT_DEVICES) {
        input_device = strstr(lpInfo->name, G_operator_audio_device);
        if (input_device != NULL) {
            G_input_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_input_devices. %s FOUND\n", line_number++, G_operator_audio_device);
        }
        strcpy(G_input_devices[index].name, lpInfo->name);
        G_input_devices[index].device_index = device_index;
        G_input_devices[index].num_channels = lpInfo->maxInputChannels;
        G_input_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultInputDevice) {
            G_input_devices[index].default_input_device = 1;
        }
        //print_time();
        //fprintf(G_fp_logfile, "[%d] build_input_device. index: %d, name: %s, dev_index: %d, channels: %d, record_num: %d \n",
        //        line_number++, index, G_input_devices[index].name, G_input_devices[index].device_index, G_input_devices[index].num_channels,
        //        G_input_devices[index].record_number);
        index++;
        num_input_devices_found = index;
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] build_input_devices. Number of input devices exceed permitted limit\n", line_number++);
    }
}

