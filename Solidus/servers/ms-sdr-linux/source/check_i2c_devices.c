#include "extern.h"
#include <math.h>
//#include <windows.h>
#include "usbavrcmd.h"
#include <usb.h>
//#include "SRDLL.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"

/*#define MCP23017_ADDRESS 0x20
#define SOLIDUS_TEMP_SENSOR 0x18
#define METER_ADDRESS 0x68
#define IQDB_ADDRESS 0x4D
#define CURRENT_SENSOR_ADDRESS 0x40*/
#define WAIT_A_LONG_TIME 32000

int G_MASTER_CONTROLLER_attached = 0;
int G_SOLIDUS_TEMP_SENSOR_attached = 0;
int G_METER_attached = 0;
int G_IQBD_attached = 0;
int G_CURRENT_SENSOR_attached = 0;
int G_MFC_attached = 0;
int G_Display_attached = 0;
byte G_Amplifier_attached = 0;
byte G_Transceiver_type = 0;
//byte G_Is_Fortis = 0;

//const char *homedir;

struct {
    uint8_t Master_controller;
    uint8_t Mfc;
    uint8_t Solidus_temp_sensor;
    uint8_t Meter;
    uint8_t IQBD;
    uint8_t Current_sensor;
    uint8_t Display;
} I2C_Controls;

int Parse_I2C_Controls_record(char *record, char *field) {
    int status = -1;
    char source_field[80] = {0};
    char *result_field;
    int size = 0;

    strcpy(source_field, field);
    size = strlen(source_field);

    if ((result_field = strstr(record, source_field)) != NULL) {
        status = atoi((result_field + (size + 1)));
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Parse_User_Controls_record. Record: %s, Field: %s, Size: %d, Result: %s\n", line_number++, record, source_field,
        //	size, result_field);
        return status;
    }
    return status;
}

int Init_I2C_Controls() {
    int status = 0;
    //int error_flag = 0;
    FILE *fp_I2C_ini = NULL;
    char init_record[300];
    char field[80] = {0};
    char file_name[PATH_MAX] = {0};
    char *record_status = NULL;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . Called.\n", line_number++);

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . Default Path: %s\n", line_number++, homedir);
        strcat(file_name, homedir);
        strcat(file_name, "/i2c.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . Path: %s\n", line_number++, file_name);
        fp_I2C_ini = fopen(file_name, "r");
        if (fp_I2C_ini != NULL) {
            record_status = fgets(init_record, sizeof (init_record), fp_I2C_ini);
            while (record_status != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . Source record: %s", line_number++, init_record);
                memset(field, 0, sizeof (field));
                strcpy(field, "G_MASTER_CONTROLLER_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.Master_controller = status;
                    G_Transceiver_type = I2C_Controls.Master_controller;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_MASTER_CONTROLLER_attached: %d, G_Transceiver_type: %d\n",
                            line_number++, I2C_Controls.Master_controller, G_Transceiver_type);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "G_MFC_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.Mfc = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_MFC_attached: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "G_SOLIDUS_TEMP_SENSOR_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.Solidus_temp_sensor = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_SOLIDUS_TEMP_SENSOR_attached: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "G_METER_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.Meter = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_METER_attached: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "G_IQBD_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.IQBD = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_IQBD_attached: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "G_CURRENT_SENSOR_attached");
                status = Parse_I2C_Controls_record(init_record, field);
                if (status != -1) {
                    I2C_Controls.Current_sensor = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_CURRENT_SENSOR_attached: %d\n", line_number++, status);
                }
                record_status = fgets(init_record, sizeof (init_record), fp_I2C_ini);
            }
        } else {
            status = -1;
        }
    } else {
        status = -1;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Init_I2C_Controls . G_CURRENT_SENSOR_attached: %d\n", line_number++, status);
    return status;
}

int Check_Threads() {
    int status = 0;
    int sleep_time = 1;

    if (I2C_Controls.Master_controller == 1) {
        if (G_MASTER_CONTROLLER_attached == FALSE) {
            Gui_Add_Message("MASTER_CONTROLLER THREAD FAILED. \n MSCC WILL NOW STOP ");
            sleep_time = 3000;
            status = -1;
        }
    }

    if (I2C_Controls.Mfc == 1) {
        if (G_MFC_attached == FALSE) {
            Gui_Add_Message("MFC THREAD NOT RUNNING. \n MSCC WILL NOW STOP");
            sleep_time = 3000;
            status = -1;
        }
    }

    if (I2C_Controls.Solidus_temp_sensor == 1) {
        if (G_SOLIDUS_TEMP_SENSOR_attached == FALSE) {
            Gui_Add_Message("SOLIDUS TEMPERATURE THREAD NOT RUNNING. \n MSCC WILL NOW STOP");
            sleep_time = 3000;
            status = -1;
        }
    }

    if (I2C_Controls.Meter == 1) {
        if (G_METER_attached == FALSE) {
            Gui_Add_Message("METER THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
            sleep_time = 3000;
            status = -1;
        }
    }

    if (I2C_Controls.IQBD == 1) {
        if (G_IQBD_attached == FALSE) {
            Gui_Add_Message("IQBD THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
            sleep_time = 3000;
            status = -1;
        }
    }

    if (I2C_Controls.Current_sensor == 1) {
        if (G_CURRENT_SENSOR_attached == FALSE) {
            Gui_Add_Message("CURRENT SENSOR THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
            sleep_time = 3000;
            status = -1;
        }
    }
    Sleep(sleep_time);
    return status;
}

int Check_Display_Address() {
    int read_status = 0;
    //char filename[PATH_MAX];
    char buffer[20] = {0};
    char message[PATH_MAX];
    int count = 0;
    int read_success = FALSE;

    buffer[0] = 0xFF;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Display_Address. STARTED\n", line_number++);
    read_status = Display_Open(DISPLAY_ADDRESS_1);
    if (read_status >= 0) {
        read_success = TRUE;
        E_display_addr = DISPLAY_ADDRESS_1;
    }
    if (read_success == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Display_Address. device check FAILED: %s\n",
                line_number++, strerror(errno));
        strcpy(message, "Check_Display_Address device check FAILED");
        Gui_Add_Message(message);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Display_Address. FINISHED\n", line_number++);
    return E_display_addr;
}

int Check_MFC_Address() {
    char filename[PATH_MAX];
    int read_status = 0;
    char buffer[20] = {0};
    //int count = 0;
    int read_success = FALSE;

    buffer[0] = 0xFF;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_MFC_Address. STARTED. Primary Address: 0x%0x\n", line_number++, MFC_ADDRESS_1);
    read_status = Read_i2c(I2C_BUSS, filename, MFC_ADDRESS_1, 0x00, buffer, 1);
    if (read_status >= 0) {
        read_success = TRUE;
        G_MFC_Address = MFC_ADDRESS_1;
    }
    if (read_success == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_MFC_Address. Alternate Address: 0x%0x\n", line_number++, MFC_ADDRESS_2);
        read_status = Read_i2c(I2C_BUSS, filename, MFC_ADDRESS_2, 0x00, buffer, 1);
        if (read_status >= 0) {
            read_success = TRUE;
            G_MFC_Address = MFC_ADDRESS_2;
        }
    }
    if (read_success == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_MFC_Address. FINISHED. device check FAILED: %s\n",
                line_number++, strerror(errno));
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_MFC_Address. FINISHED.\n",line_number++);
    return G_MFC_Address;
}

int Check_i2c_devices() {
    int status = 0;
    int read_status = 0;
    char filename[PATH_MAX];
    char buffer[20] = {0};
    char message[PATH_MAX];
    uint8_t started = FALSE;
    uint32_t start_count = 0;
    int test = TRUE;

    while (started == FALSE && start_count++ < WAIT_A_LONG_TIME) {
        if (I2C_Controls.Master_controller != FALSE) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Check_i2c_devices . G_Transceiver_type: %d\n",
                    line_number++, G_Transceiver_type);
            read_status = Read_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x00, buffer, 1);
            if (read_status < 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . MCP23017_ADDRESS device check FAILED: %s\n",
                        line_number++, strerror(errno));
                strcpy(message, "MCP23017_ADDRESS device check FAILED");
                Gui_Add_Message(message);
            } else {
                G_MASTER_CONTROLLER_attached = TRUE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . MCP23017_ADDRESS device check PASSED\n",
                        line_number++);
            }
        }
        if (I2C_Controls.Mfc == TRUE) {
            G_MFC_Address = Check_MFC_Address();
            if (G_MFC_Address == 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . MCP23008_ADDRESS (MFC) device check FAILED: %s\n",
                        line_number++, strerror(errno));
                strcpy(message, "MCP23008_ADDRESS device check FAILED");
                Gui_Add_Message(message);
            } else {
                G_MFC_attached = TRUE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . MCP23008_ADDRESS device check PASSED\n",
                        line_number++);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . Calling Check_Display_Address \n",
                        line_number++);
                E_display_addr = Check_Display_Address();
                if (E_display_addr != 0) {
                    G_Display_attached = 1;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_i2c_devices . DISPLAY device check PASSED. Address: %d \n",
                            line_number, E_display_addr);
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_i2c_devices . DISPLAY device check FAILED. Address: %d \n",
                            line_number, E_display_addr);
                }
            }
        }

        if (I2C_Controls.Meter) {
            read_status = Read_i2c(I2C_BUSS, filename, METER_ADDRESS, 0x00, buffer, 1);
            if (read_status < 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . METER_ADDRESS device check FAILED: %s\n",
                        line_number++, strerror(errno));
                strcpy(message, "METER_ADDRESS device check FAILED");
                Gui_Add_Message(message);
            } else {
                G_METER_attached = TRUE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_i2c_devices . METER_ADDRESS device check PASSED\n",
                        line_number++);
            }
        }


        started = TRUE;
    }
    if (started == FALSE || start_count >= WAIT_A_LONG_TIME) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_i2c_devices . GUI did NOT start\n", line_number++);
        status = -1;
    }
    return status;
}
