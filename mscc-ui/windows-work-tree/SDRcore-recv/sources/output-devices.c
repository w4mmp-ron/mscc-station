#include "extern.h"

extern struct output_devices G_output_devices[MAX_OUTPUT_DEVICES];
extern struct output_devices G_digital_output_devices[MAX_OUTPUT_DEVICES];

int num_output_devices_found = 0;
int num_digital_output_devices_found = 0;
int G_num_pulse_output_devices_found = 0;
int sound_ini_file_exists = 0;
char G_audio_device[PATH_MAX] = {0};
char G_digital_audio_device[PATH_MAX] = { 0 };

int Get_Digital_Sound_Device() {
    FILE* fp_Sound_ini;
    char l_path[PATH_MAX] = { 0 };
    char init_record[PATH_MAX] = { 0 };
    int status = TRUE;
    char* end_of_file;
    int record_length = 0;
    const char* homedir;
    //int ret;
    //char audio_device_temp[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/digital-speaker.ini");
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
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. FAIL: file not found\n", line_number++);
            status = FALSE;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_Digital_Sound_Device. FINISHED\n", line_number++);
    }
    return status;
}
int Get_Sound_Device() {
    FILE *fp_Sound_ini;
    char l_path[PATH_MAX] = {0};
    char init_record[PATH_MAX] = {0};
    int status = TRUE;
    char *end_of_file;
    int record_length = 0;
    const char *homedir;
    //int ret;
    //char audio_device_temp[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/operator-speaker.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Sound_Device. STARTED. sound-device.ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            if (end_of_file != NULL) {
                //strncpy(audio_device, init_record,(strlen(init_record) - 1));
                record_length = strlen(init_record);
                strncpy(G_audio_device, init_record, (record_length));
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Sound_Device. input record: %s, audio_device: %s\n",
                        line_number++, init_record, G_audio_device);
                fclose(fp_Sound_ini);
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Sound_Device. FAIL: No record found\n", line_number++);
                status = FALSE;
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Sound_Device. FAIL: file not found\n", line_number++);
            status = FALSE;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_Sound_Device. FINISHED\n", line_number++);
    }
    return status;
}

void build_digital_output_devices(int device_index) {
    static int index = 0;
    char* output_device;

    if (index < MAX_OUTPUT_DEVICES) {
        output_device = strstr(lpInfo->name, G_digital_audio_device);
        if (output_device != NULL) {
            G_digital_output_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_digital_output_devices. %s FOUND\n", line_number++, G_digital_audio_device);
        }
        strcpy(G_digital_output_devices[index].name, lpInfo->name);
        G_digital_output_devices[index].device_index = device_index;
        G_digital_output_devices[index].num_channels = lpInfo->maxOutputChannels;
        num_digital_output_devices_found++;
        G_digital_output_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultOutputDevice) {
            G_digital_output_devices[index].default_output_device = 1;
        }
        index++;
        num_digital_output_devices_found = index;

    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] build_digital_output_devices. Number of output devices exceed permitted limit\n", 
            line_number++);
    }
}

void build_output_devices(int device_index) {
    static int index = 0;
    char *output_device;

    if (index < MAX_OUTPUT_DEVICES) {
        output_device = strstr(lpInfo->name, G_audio_device);
        if (output_device != NULL) {
            G_output_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_output_devices -> %s FOUND\n", line_number++, G_audio_device);
        }
        strcpy(G_output_devices[index].name, lpInfo->name);
        G_output_devices[index].device_index = device_index;
        G_output_devices[index].num_channels = lpInfo->maxOutputChannels;
        num_output_devices_found++;
        G_output_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultOutputDevice) {
            G_output_devices[index].default_output_device = 1;
        }
        index++;
        num_output_devices_found = index;

    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Number of output devices exceed permitted limit\n", line_number++);
    }
}




