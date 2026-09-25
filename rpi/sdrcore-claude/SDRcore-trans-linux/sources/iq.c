#include "extern.h"
#include "sdrcore.h"
#include "commands.h"
#include "port_defines.h"
#include "commands.h"
#include "version.h"
//#include "iq.h"

struct iq_stack {
    int record;
    int band;
    int iq_offset;
} G_iq_stack[12];
//10M, 12M, 15M,17M,20M,30M,40M,60M,80M,160M,630M,2200M
//int iq_defaults_PCB_5[] = {-104, -97, -41, -41, -43, -33, -31, -9, -5, -3};
//int iq_defaults_PCB_4[] = {-139, -112, -104, -88, -72, -52, -39, -30, -22, -16};
//int iq_defaults_PCB_C[] = {-85, -40, -86, -42, -20, -17, -2, -5, 5, 5};
//int iq_defaults_PCB_0[] = {-111, -75, -99, -49, -36, -25, -14, -12, -5, 4};
int iq_defaults_PCB[] = {-104, -97, -41, -41, -43, -33, -31, -9, -5, -3, -3, -3};

//const char *homedir;

int delete_iq_ini_file() {
    FILE *IQ_delete = NULL;
    char l_path[PATH_MAX] = {0};
    int lenght = 0;
    int band = 0;
    int status = 0;
    int ret;
    const char* homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/iq.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. Default Path: %s\n", line_number++, l_path);
        print_time();
        fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. iq.ini. Path: %s\n", line_number++, l_path);
        IQ_delete = fopen(l_path, "r");
        if (IQ_delete == NULL) {
            print_time();
            fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. iq.ini not found. Nothing to delete\n", line_number++);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. deleting iq.ini file\n", line_number++);
            fclose(IQ_delete);
            ret = remove(l_path);
            if (ret == 0) {
                print_time();
                fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. The iq.ini file has been deleted \n", line_number++);
                status = 1;
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. The iq.ini file Deletion has FAILED. ret: %d \n", line_number++, ret);
                status = 0;
            }
        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] delete_iq_ini_file. Finished \n", line_number++);
    return status;
}

int Load_Defaults() {
    FILE *IQ_load = NULL;
    char l_path[PATH_MAX] = {0};
    int lenght = 0;
    int band = 0;
    int status = 0;
    char iq_init_record[132];
    int mynumber;
    int record = 0;
    const char* homedir;

    struct {
        char *record_number;
        char *band;
        char *iq_offset;
    } iq_record;

    print_time();
    fprintf(G_fp_logfile, "[%d] Load_Defaults. Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        switch (G_PCB_Version) {
            case 1:
                strcat(l_path, "/rev-0/iq.ini");
                break;
            case 2:
                strcat(l_path, "/rev-c/iq.ini");
                break;
            case 4:
                strcat(l_path, "/rev-4/iq.ini");
                break;
            case 5:
                strcat(l_path, "/rev-5/iq.ini");
                break;
            case 6:
                strcat(l_path, "/rev-g/iq.ini");
                break;
            case 7:
                strcat(l_path, "/rev-m/iq.ini");
                break;
            case 8:
                strcat(l_path, "/rev-u/iq.ini");
                break;
            case 10:
                strcat(l_path, "/rev-MKII/iq.ini");
                break;
        }
        IQ_load = fopen(l_path, "r");
        if (IQ_load != NULL) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Load_Defaults. %s OPENED \n ", line_number++, l_path);
            while (fgets(iq_init_record, sizeof (iq_init_record), IQ_load) != NULL) {
                iq_record.iq_offset = strstr(iq_init_record, "IQ_OFFSET");
                mynumber = atoi((iq_record.iq_offset + 10));
                iq_defaults_PCB[record] = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] Load_Defaults. Record: %d, Offset: %d \n", line_number++,
                        record, iq_defaults_PCB[record]);
                record++;
            }
            print_time();
            fprintf(G_fp_logfile, "[%d] Load_Defaults. record load finished\n ", line_number++);
            fclose(IQ_load);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Load_Defaults. FAILED\n", line_number++);
            status = 0;
        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Load_Defaults. Finished \n", line_number++);
    return status;
}

int Check_iq_ini_file() {
    FILE* IQ_create = NULL;
    char l_path[PATH_MAX] = { 0 };
    int lenght = 0;
    int band = 0;
    int status = 0;
    const char* homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] create_iq_ini_file. Called \n", line_number++);
    status = Load_Defaults();
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/iq.ini");
        IQ_create = fopen(l_path, "r");
        if (IQ_create == NULL) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Check_iq_ini_file. iq.ini not found. Creating iq.ini \n", line_number++);
            IQ_create = fopen(l_path, "w");
            if (IQ_create != NULL) {
                for (band = 0; band < 12; band++) {
                    fprintf(IQ_create, "RECORD=%d,BAND=%d,IQ_OFFSET=%d\n", band, band, iq_defaults_PCB[band]);
                }
                fclose(IQ_create);
            }
            else {
                print_time();
                fprintf(G_fp_logfile, "[%d] create_iq_ini_file. Creation of iq.ini file Failed\n", line_number++);
                status = 0;
            }

        }
        else {
            print_time();
            fprintf(G_fp_logfile, "[%d] create_iq_ini_file. File exists. Not creating iq.ini file\n", line_number++);
            status = 1;
        }
    }
    else {
        status = 0;
        print_time();
        fprintf(G_fp_logfile, "[%d] Check_iq_ini_file.  My_getenv failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Check_iq_ini_file. Finished \n", line_number++);
    return status;
}

int update_iq_ini_file() {
    FILE *IQ_update = NULL;
    char l_path[PATH_MAX] = {0};
    int lenght = 0;
    int band = 0;
    int status = 0;
    const char* homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] update_iq_ini_file. Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/iq.ini");
        IQ_update = fopen(l_path, "w");
        if (IQ_update != NULL) {
            for (band = 0; band < 12; band++) {
                fprintf(IQ_update, "RECORD=%d,BAND=%d,IQ_OFFSET=%d\n", G_iq_stack[band].record, G_iq_stack[band].band,
                        G_iq_stack[band].iq_offset);
                print_time();
                fprintf(G_fp_logfile, "[%d] update_iq_ini_file. RECORD=%d,BAND=%d,IQ_OFFSET=%d\n", line_number++,
                        G_iq_stack[band].record, G_iq_stack[band].band, G_iq_stack[band].iq_offset);
            }
            fflush(IQ_update);
            fclose(IQ_update);
            status = 1;
            print_time();
            fprintf(G_fp_logfile, "[%d] update_iq_ini_file. Finished \n", line_number++);
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] update_iq_ini_file. Open file failed\n", line_number++);
            status = 0;
        }
    }
    return status;
}
extern double fabs(double x);

void IQ_calc(int iq_int) {
    sp_float cormult;
    //int temp_iq;
    sp_float corstep = .05f / 100.0f; // 5% range, divided by 100 steps

    int intval = 90; // Let's say the current correction value is set to 90 in the GUI
    intval = iq_int;
    cormult = 1.0f - (corstep * fabs((sp_float) intval)); // calculate correction multiplier
    iMult = qMult = 1.0f;
    if (intval >= 0) iMult = cormult;
    else qMult = cormult;
    print_time();
    fprintf(G_fp_logfile, "[%d] IQ_calc. iq_offset: %d, Imult: %f, Qmult: %f\n",
            line_number++, iq_int, iMult, qMult);
}

int init_IQ_structure() {
    FILE *IQ_ini = NULL;
    int status = 1;
    char l_path[PATH_MAX] = {0};
    char iq_init_record[132];
    int mynumber;
    int record = 0;
    const char* homedir;

    struct {
        char *record_number;
        char *band;
        char *iq_offset;
    } iq_record;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/iq.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] init_IQ_structure. iq.ini. Path: %s\n", line_number++, l_path);
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
                //fprintf(G_fp_logfile, "[%d] init_IQ_structure. Record: %d, Band:%d, Offset: %d\n", line_number++,
                //	G_iq_stack[record].record, G_iq_stack[record].band, G_iq_stack[record].iq_offset);
                record++;
            }
            fclose(IQ_ini);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] UDP init_IQ_structure. Open file failed\n", line_number++);
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] init_IQ_structure.  My_getenv failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] init_IQ_structure. Finished\n", line_number++);
    return status;
}
