#define _CRT_SECURE_NO_WARNINGS 1
#include "extern.h"
#include "iq.h"

#define MAX_IMAGE_ELEMENT 50
const char *homedir;

struct iq_stack {
    int record;
    int band;
    int iq_offset;
} G_iq_stack[12];

//10M, 12M, 15M,17M,20M,30M,40M,60M,80M,160M
int iq_defaults_PCB[] = {-104, -97, -41, -41, -43, -33, -31, -9, -5, -3, -3, -3};

struct {
    uint32_t freq;
    uint32_t magnitude_low;
    uint32_t magnitude;
    uint32_t magnitude_high;
    float low_cut;
    float center;
    float high_cut;
} image_data[MAX_IMAGE_ELEMENT];

struct {
    float low_cut;
    float center;
    float high_cut;
} image_limits;

uint32_t Report_Image_Value(int calibration_count) {
    char send_buf[20] = { 0 };
    //int calibration_state = 0;
    //int error = 0;
    uint32_t freq = 0;
    uint32_t magnitude_low_max = 0;
    uint32_t magnitude_mid_max = 0;
    uint32_t magnitude_high_max = 0;
    uint32_t average_magnitude_max = 0;
    int low_max = 0;
    int mid_max = 0;
    int high_max = 0;
    int count = 0;

    //print_time();
    //fprintf(G_fp_logfile, "[%d] Report_Image_Value -> Called\n", line_number++);
    for (count = 0; count < calibration_count; count++) {
        if (magnitude_low_max < image_data[count].magnitude_low) {
            magnitude_low_max = image_data[count].magnitude_low;
            low_max = count;
        }
        if (magnitude_mid_max < image_data[count].magnitude) {
            magnitude_mid_max = image_data[count].magnitude;
            mid_max = count;
        }
        if (magnitude_high_max < image_data[count].magnitude_high) {
            magnitude_high_max = image_data[count].magnitude_high;
            high_max = count;
        }
    }
    average_magnitude_max = (uint32_t) ((float) ((magnitude_high_max + magnitude_low_max + magnitude_mid_max)) / 3.0f);
    //print_time();
    //fprintf(G_fp_logfile, "[%d] Report_Image_Value -> average_magnitude_max %ld\n", line_number,
    //        average_magnitude_max);
    return average_magnitude_max;
}

int Get_Image_data(int calibration_element, uint32_t value) {
    int status = 0;
    int calibration_state = 0;
    int count = 0;
    char send_buf[20] = { 0 };

    //print_time();
    //fprintf(G_fp_logfile, "[%d] Update_Image_data -> Called -> calibration_element: %d\n",
    //        line_number++, calibration_element);
    mycalstate.calStart = TRUE;
    mycalstate.calReady = FALSE;
    while (calibration_state == 0 && count++ < 30000) {
        Sleep(1);
        if (mycalstate.calReady == TRUE) {
            image_data[calibration_element].magnitude_low = mycalstate.calMagLow;
            image_data[calibration_element].freq = value;
            image_data[calibration_element].magnitude = mycalstate.calMag;
            image_data[calibration_element].magnitude_high = mycalstate.calMagHigh;
            image_data[calibration_element].low_cut = image_limits.low_cut;
            image_data[calibration_element].center = image_limits.center;
            image_data[calibration_element].high_cut = image_limits.high_cut;
            calibration_state = TRUE;
        }
    }
    if (calibration_state == FALSE) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_Image_data -> Calibration routine -> FAILED -> Loop Count: %d\n",
                line_number++, count);
    }
    //print_time();
    //fprintf(G_fp_logfile, "[%d] Update_Image_data -> Finished.\n", line_number++);
    return calibration_state;
}

void *Check_Image_Level_thread(void *param) {
    int status = 0;
    char send_buf[20] = { 0 };
    int count = 0;
    int start = TRUE;
    uint32_t average_magnitude_max = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] Check_Image_Level_thread -> Started.\n", line_number++);
    while (G_all_threads_run) {
        if (G_Image_Check) {
            if (start) {
                image_limits.low_cut = 598.0f;
                image_limits.center = 600.0f;
                image_limits.high_cut = 602.0f;
                mycalstate.freq_center = 600.0f;
                mycalstate.freq_high = 602.0f;
                mycalstate.freq_low = 598.0f;
                mycalstate.calReady = FALSE;
                mycalstate.Cycle_Count = 10;
                mycalstate.calStart = 1;
                start = FALSE;
            }
            for (count = 0; count < MAX_IMAGE_ELEMENT; count++) {
                status = Get_Image_data(count, 0);
            }
            if (count >= MAX_IMAGE_ELEMENT && status == TRUE) {
                average_magnitude_max = Report_Image_Value(MAX_IMAGE_ELEMENT);
                send_buf[0] = CMD_REPORT_IMAGE_VALUE;
                memcpy(&send_buf[1], &average_magnitude_max, 4);
                sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen);
            }
        }
        if (start == FALSE) {
            start = TRUE;
        }
        Sleep(100);
    }
    pthread_exit(0);
    return NULL;
}

int Load_Defaults() {
    FILE *IQ_Defaults;
    char l_path[MAX_PATH] = {0};
    int status = 0;
    char iq_init_record[132];
    int mynumber;
    int record = 0;

    struct {
        char *record_number;
        char *band;
        char *iq_offset;
    } iq_record;

    print_time();
    fprintf(G_fp_logfile, "[%d] Load_Defaults -> Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        switch (G_PCB_Version) {
            case 1:
                strcat(l_path, "/rev-0/recv-iq.ini");
                break;
            case 2:
                strcat(l_path, "/rev-c/recv-iq.ini");
                break;
            case 4:
                strcat(l_path, "/rev-4/recv-iq.ini");
                break;
            case 5:
                strcat(l_path, "/rev-5/recv-iq.ini");
                break;
            case 6:
                strcat(l_path, "/rev-g/recv-iq.ini");
                break;
            case 7:
                strcat(l_path, "/rev-m/recv-iq.ini");
                break;
            case 8:
                strcat(l_path, "/rev-u/recv-iq.ini");
                break;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] Load_Defaults: %s\n", line_number++, l_path);
        IQ_Defaults = fopen(l_path, "r");
        if (IQ_Defaults != NULL) {
            while (fgets(iq_init_record, sizeof (iq_init_record), IQ_Defaults) != NULL) {
                //iq_record.record_number = strstr(iq_init_record, "RECORD=");
                //iq_record.band = strstr(iq_init_record, "BAND=");
                iq_record.iq_offset = strstr(iq_init_record, "IQ_OFFSET");
                mynumber = atoi((iq_record.iq_offset + 10));
                iq_defaults_PCB[record] = mynumber;
                //print_time();
                //fprintf(G_fp_logfile, "[%d] UDP Thread -> init_IQ_structure -> init_IQ_structure -> Record: G_iq_stack[%d].%d, Band: G_iq_stack[%d].%d, Offset: G_iq_stack[%d].%d\n", line_number++,
                //        record, G_iq_stack[record].record, record, G_iq_stack[record].band, record, G_iq_stack[record].iq_offset);
                record++;
            }
            print_time();
            fprintf(G_fp_logfile, "[%d] Load_Defaults -> ", line_number);
            for (record = 0; record < 12; record++) {
                fprintf(G_fp_logfile, "record: %d, offset: %d ", record, iq_defaults_PCB[record]);
            }
            fprintf(G_fp_logfile, "\n");
            fclose(IQ_Defaults);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Load_Defaults -> File Open FAILED: %s\n",
                    line_number++, l_path);
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Load_Defaults -> My_getenv failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Load_Defaults -> FINSISHED\n", line_number++);
    return (status);
}

int create_iq_ini_file(int force) {
    FILE *IQ_update;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int band = 0;
    int status = 1;
    int create = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Called. force: %d \n", line_number++, force);
    create = force;
    status = Load_Defaults();
    if (status == 1) {
        if ((homedir = My_getenv("HOME")) != NULL) {
            strcpy(l_path, homedir);
            strcat(l_path, "/.local/share/mscc");
            strcat(l_path, "/recv-iq.ini");
            print_time();
            fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Creating %s \n",
                line_number++, l_path);
            IQ_update = fopen(l_path, "w");
            if (IQ_update != NULL) {
                for (band = 0; band < 12; band++) {
                    fprintf(IQ_update, "RECORD=%d,BAND=%d,IQ_OFFSET=%d\n",
                        band, band, iq_defaults_PCB[band]);
                }
                fflush(IQ_update);
                fclose(IQ_update);
                status = 1;
            }
            else {
                print_time();
                fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> FAILED: %s\n", line_number++, l_path);
                status = 0;
            }
            print_time();
            fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Finished \n", line_number++);
        }
    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] create_iq_ini_file - Load_Defaults FAILED  \n", line_number++);
    }
    return status;
}

/*int create_iq_ini_file_copy(int force) {
    FILE *IQ_update;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int band = 0;
    int status = 1;
    int create = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Called. force: %d \n", line_number++, force);
    create = force;
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/recv-iq.ini");
        if (create == 0) {
            IQ_update = fopen(l_path, "r");
            if (IQ_update == NULL) {
                print_time();
                fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> %s not found. Creating %s \n",
                        line_number++, l_path, l_path);
                create = 1;
            }
            if (create == 1) {
                print_time();
                fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Creating %s \n",
                        line_number++, l_path);
                IQ_update = fopen(l_path, "w");
                if (IQ_update != NULL) {
                    for (band = 0; band < 12; band++) {
                        fprintf(IQ_update, "RECORD=%d,BAND=%d,IQ_OFFSET=%d\n",
                                band, band, iq_defaults_PCB[band]);
                    }
                    fflush(IQ_update);
                    fclose(IQ_update);
                    status = 1;
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Creation Failed: %s\n", line_number++, l_path);
                    status = 0;
                }
            }
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] create_iq_ini_file -> Finished \n", line_number++);
        return status;
    }
}*/

int update_iq_ini_file() {
    FILE *IQ_update;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int band = 0;
    int status = 1;

    print_time();
    fprintf(G_fp_logfile, "[%d] update_iq_ini_file -> Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/recv-iq.ini");
        IQ_update = fopen(l_path, "w");
        if (IQ_update != NULL) {
            for (band = 0; band < 12; band++) {
                fprintf(IQ_update, "RECORD=%d,BAND=%d,IQ_OFFSET=%d\n", G_iq_stack[band].record, G_iq_stack[band].band,
                        G_iq_stack[band].iq_offset);
                print_time();
                fprintf(G_fp_logfile, "[%d] update_iq_ini_file -> RECORD=%d,BAND=%d,IQ_OFFSET=%d\n", line_number++,
                        G_iq_stack[band].record, G_iq_stack[band].band, G_iq_stack[band].iq_offset);
            }
            fflush(IQ_update);
            fclose(IQ_update);
            status = 1;
            print_time();
            fprintf(G_fp_logfile, "[%d] update_iq_ini_file -> Finished \n", line_number++);
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] update_iq_ini_file -> Open file failed\n", line_number++);
            status = 0;
        }
    }
    return status;
}

void IQ_calc(int iq_int) {
    sp_float cormult;
    //int temp_iq;
    sp_float corstep = .05 / 100.0f; // 5% range, divided by 100 steps

    int intval = 90; // Let's say the current correction value is set to 90 in the GUI
    intval = iq_int;
    cormult = 1.0f - (corstep * fabs((sp_float) intval)); // calculate correction multiplier
    mystate.iMult = mystate.qMult = 1.0f;
    if (intval >= 0) mystate.iMult = cormult;
    else mystate.qMult = cormult;
    print_time();
    fprintf(G_fp_logfile, "[%d] IQ_calc -> iq_offset: %d, Imult: %f, Qmult: %f\n",
            line_number++, iq_int, mystate.iMult, mystate.qMult);
}

int Init_IQ_structure() {
    FILE *IQ_ini;
    int status = 1;
    char l_path[MAX_PATH] = {0};
    char iq_init_record[132];
    int mynumber;
    int record = 0;

    struct {
        char *record_number;
        char *band;
        char *iq_offset;
    } iq_record;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/recv-iq.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Init_IQ_structure -> recv-iq.ini-> Path: %s\n",
                line_number++, l_path);
        IQ_ini = fopen(l_path, "r");
        if (IQ_ini != NULL) {
            while (fgets(iq_init_record, sizeof (iq_init_record), IQ_ini) != NULL) {
                iq_record.record_number = strstr(iq_init_record, "RECORD=");
                iq_record.band = strstr(iq_init_record, "BAND=");
                iq_record.iq_offset = strstr(iq_init_record, "IQ_OFFSET");
                mynumber = atoi((iq_record.record_number + 7));
                G_iq_stack[record].record = mynumber;
                mynumber = atoi((iq_record.band + 5));
                G_iq_stack[record].band = mynumber;
                mynumber = atoi((iq_record.iq_offset + 10));
                G_iq_stack[record].iq_offset = mynumber;
                //print_time();
                //fprintf(G_fp_logfile, "[%d] UDP Thread -> init_IQ_structure -> init_IQ_structure -> Record: G_iq_stack[%d].%d, Band: G_iq_stack[%d].%d, Offset: G_iq_stack[%d].%d\n", line_number++,
                //        record, G_iq_stack[record].record, record, G_iq_stack[record].band, record, G_iq_stack[record].iq_offset);
                record++;
            }
            fclose(IQ_ini);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Init_IQ_structure -> Open file failed\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time();
        fprintf(G_fp_logfile, "[%d] Init_IQ_structure -> My_getenv failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Init_IQ_structure -> Finished\n", line_number++);
    return status;
}