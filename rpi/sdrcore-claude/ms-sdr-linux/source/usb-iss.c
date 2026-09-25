#include "extern.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <WinBase.h>
#include "usbavrcmd.h"

#define I2C_SGL 0x53
#define I2C_AD0 0x54
#define I2C_AD1 0x55
#define I2C_AD2 0x56
#define I2C_TEST 0x58
#define I2C_SET_SER_NUM  0x37
#define METER_BYPASS_LIMIT 10

#define FORTIS_ANTENNA_SWITCH_OFF 0X01 //INACTIVE (OFF) HIGH
#define FORTIS_FILTER_160 0x0A
#define FORTIS_FILTER_80 0x08
#define FORTIS_FILTER_60_40 0x06
#define FORTIS_FILTER_30_20 0x04
#define FORTIS_FILTER_17_15 0x02
#define FORTIS_FILTER_12_10 0x00
#define FORTIS_FILTER_NO_BAND 0x01


const char* homedir;
char USB_ISS_Port_Name[MAX_PATH] = { 0 };
int USB_baud_rate = 19200;
int USB_parity = 0;
int USB_stop_bits = 0;
int USB_data_bits = 8;
int USB_baud_rates[8] = { 1200,2400,4800,9600,19200,38400,57600,115200 };
int USB_parity_values[3] = { PARITY_NONE,PARITY_EVEN,PARITY_ODD };
int USB_stop_bit_values[2] = { ONESTOPBIT,TWOSTOPBITS };
int USB_data_bit_values[3] = { 7,8,9 };

HANDLE USB_hSerial = 0;
int USB_Serial_port_open = 0;
DWORD USB_com_time_out = 20;
DCB USB_dcbSerialParams = { 0 };
COMMTIMEOUTS USB_timeouts = { 0 };
static BOOL USB_com_read_status = TRUE;
DWORD dwBytesRead = 0;
BOOL error;
int result = 0;
DWORD bytes_to_write;
DWORD bytes_written;
LPVOID lpMsgBuf;
LPVOID lpDisplayBuf;
DWORD dw;
uint8_t I2C_BUSY;
const char* thread_name;
uint8_t Failure_Reported = FALSE;
byte GP_A = { 0 };
int read_operation_count = 0;
int write_operation_count = 0;

/*int Process_meter() {
    static int state = TRUE;
    int16_t band = FREQ_OUT_OF_RANGE;
    static int pass_count = 0;

    if (G_TX == TRUE && G_SetModeRxTX == TRUE) {
        if (G_Meter_Allow == FALSE) {
            G_Meter_Allow = TRUE;
            state = FALSE;
        }
        else {
            state = TRUE;
            pass_count = 0;
        }
    }
    return state;
}

int Process_MFC() {
    static int state = TRUE;
    static int pass_count = 0;

    if (G_MFC_allow == FALSE && pass_count++ == 0) {
        G_MFC_allow = TRUE;
        state = FALSE;
    }
    else {
        state = TRUE;
        pass_count = 0;
    }

    return state;
}

int Process_Display() {
    int state = TRUE;
    static int pass_count = 0;

    if (G_Display_allow == FALSE && pass_count++ == 0) {
        G_Display_allow = TRUE;
        state = FALSE;
    }
    else {
        state = TRUE;
        pass_count = 0;
    }
    return state;
}
*/


int MCP23008_Controller_main(unsigned char GPIO_A) {
    char filename[MAX_PATH] = { 0 };
    int status = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] MCP23008_Controller_main -> GPIOA: 0x%02X,\n",
    //    line_number++, GPIO_A);
    status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x09, &GPIO_A, 1, "MCP23008_Controller_main");
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MCP23008_Controller_main -> Write_i2c FAILED: %d\n", line_number++, status);
    }
    return status;
}

int MCP_Init() {
    char filename[MAX_PATH] = { 0 };
    int status = 0;
    unsigned char buffer;

    buffer = 0x00;
    status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x00, &buffer, 1, "MCP_Init");
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MCP_Init -> Write_i2c FAILED: %d\n", line_number++, status);
    }
    return status;
}

int Manage_MCP23008() {
    int status = 0;
    static unsigned char previous_gp_a = 0;

    if ((previous_gp_a != GP_A)) {
        status = MCP23008_Controller_main(GP_A);
        previous_gp_a = GP_A;
        //if (G_QRP_TX_ACTIVE == TRUE) {
        //    G_QRP_TX_ACTIVE = FALSE;
        //}
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. FINISHED\n", line_number++, thread_name, __func__);
    }
    return status;
}

void Manage_antenna_switch() {
    static byte previous_antenna_on_off = 100;

    if (G_TX == FALSE) {
        if (G_Antenna_Switch != previous_antenna_on_off) {
            previous_antenna_on_off = G_Antenna_Switch;
            if (G_Antenna_Switch == 0) {
                GP_A = GP_A | FORTIS_ANTENNA_SWITCH_OFF;
            }
            else {
                GP_A = GP_A & ~FORTIS_ANTENNA_SWITCH_OFF;
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. %s G_Antenna_Switch: %d, GP_A: 0x%02X\n",line_number++, thread_name, __func__, 
                                                                        G_Antenna_Switch, GP_A);
        }
    }
}

void Manage_filters() {
    static int previous_band = 0xff;
    int16_t band = FREQ_OUT_OF_RANGE;

    if (G_TX == FALSE && G_band_normal != GENERAL_BAND) {
        if (G_band_normal == FREQ_OUT_OF_RANGE) {
            band = G_Amplifier_band;
        }
        else {
            band = G_band_normal;
        }
        if (previous_band != band) {
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Master_controller. Manage_filters. START band:%d\n", line_number++,band);
            GP_A = GP_A & FORTIS_FILTER_NO_BAND;
            switch (band) {
            case 160:
                GP_A = GP_A | FORTIS_FILTER_160;
                break;
            case 80:
                GP_A = GP_A | FORTIS_FILTER_80;
                break;
            case 60:
                GP_A = GP_A | FORTIS_FILTER_60_40;
                break;
            case 40:
                GP_A = GP_A | FORTIS_FILTER_60_40;
                break;
            case 30:
                GP_A = GP_A | FORTIS_FILTER_30_20;
                break;
            case 20:
                GP_A = GP_A | FORTIS_FILTER_30_20;
                break;
            case 17:
                GP_A = GP_A | FORTIS_FILTER_17_15;
                break;
            case 15:
                GP_A = GP_A | FORTIS_FILTER_17_15;
                break;
            case 12:
                GP_A = GP_A | FORTIS_FILTER_12_10;
                break;
            case 10:
                GP_A = GP_A | FORTIS_FILTER_12_10;
                break;
            default:
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] Master_controller. Manage_filters. Invalid band:%d\n",line_number++,band);
                break;
            }
            previous_band = band;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. %s FINISHED. band: %d, Filter: 0x%02X \n", line_number++, thread_name, __func__, band, GP_A);
        }
    }
}

void* Master_controller(void* param) {
    int MCP23008_status = 0;
    static int master_state = 0;
    int token_status = 0;
    long long previous_lock_failed = 0;
    long long lock_failed_diff = 0;
    long long previous_lock_failed_diff = 0;
    long long lock_failed_limit = 350;
    long long lock_failed = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Master_controller STARTED. \n", line_number++);
    MCP_Init();
    thread_name = __func__;
    while (G_all_threads_run) {
        token_status = pthread_mutex_trylock(&Master_Device_Token_available);
        if (token_status != EBUSY) {
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Master_controller. token_status: %d\n", line_number++, token_status);
            switch (master_state) {
            case 0:
                Manage_filters();
                master_state++;
                break;
            case 1:
                Manage_antenna_switch();
                master_state++;
                break;
            case 2:
                MCP23008_status = Manage_MCP23008();
                master_state = 0;
                //Sleep(Get_random_time());
                break;
            }
            pthread_mutex_unlock(&Master_Device_Token_available);
        }
        else {
            lock_failed++;
            lock_failed_diff = lock_failed - previous_lock_failed;
            if (lock_failed_diff > LOCK_FAILED_LIMIT) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. lock FAILED: %I64d, previouslock_failed: %I64d, lock_failed_diff: %I64d\n",
                    line_number++, __func__, lock_failed, previous_lock_failed, lock_failed_diff);
            }
            previous_lock_failed = lock_failed;
            previous_lock_failed_diff = lock_failed_diff;
        }
        Sleep(Get_random_time());
    }
    Sleep(10);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Master_controller. Normal Exit\n", line_number++);
    pthread_exit(0);
    return NULL;
}

int I2C_Check_Device(int device_address) {
    unsigned char sbuf[20] = { 0 };
    static BOOL com_read_status = TRUE;
    int status = 0;

    sbuf[0] = I2C_TEST; 						// USB_ISS byte
    sbuf[1] = (unsigned char)device_address << 1;    // Device address to check
    print_time(0);
    fprintf(G_fp_logfile,"[%d] I2C_Check_Device. STARTED. device_address 0x%x, shifted address: 0x%x\n", 
                                                                        line_number++, device_address, sbuf[1]);
    bytes_to_write = 2;
    error = WriteFile(USB_hSerial, sbuf, bytes_to_write, &bytes_written, NULL);
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] I2C_Check_Device. WriteFile. FAILED %s\n", line_number++, strerror(errno));
    }
    else {
        memset(sbuf, 0, sizeof(sbuf));
        com_read_status = ReadFile(USB_hSerial, sbuf, 1, &dwBytesRead, NULL);
        if (error <= 0) {
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] I2C_Check_Device. ReadFile. FAILED %s\n", line_number++, strerror(errno));
        }
        else {
            status = sbuf[0];
            print_time(0);
            fprintf(G_fp_logfile, "[%d] I2C_Check_Device. FINISHED status %d, dwBytesRead: %d\n", line_number++, status, dwBytesRead);
        }
    }
    return status;
}

void Get_Version(void)
{
    unsigned char sbuf[20] = { 0 };
    static BOOL com_read_status = TRUE;
    int status = 1;

    sbuf[0] = 0x5A; 						// USB_ISS byte
    sbuf[1] = 0x01;							// Software return byte

    //if (write(fd, sbuf, 2) < 0) perror("display_version write");	// Write data to USB-ISS
    //if (tcdrain(fd) < 0) perror("display_version tcdrain");
    //if (read(fd, sbuf, 3) < 0) perror("display_version read");	// Read data back from USB-ISS, module ID and software version
    bytes_to_write = 2;
    error = WriteFile(USB_hSerial, sbuf, bytes_to_write, &bytes_written, NULL);
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_Version. WriteFile. FAILED %s\n", line_number++, strerror(errno));
    }
    com_read_status = ReadFile(USB_hSerial, sbuf, 3, &dwBytesRead, NULL);
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_Version. ReadFile. FAILED %s\n", line_number++, strerror(errno));
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Version. USB-ISS Module ID: %u \n", line_number++,sbuf[0]);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Version. USB-ISS Software v: %u:0x%x \n", line_number++,sbuf[1], sbuf[2]);
}

int Initialize_i2c_mode(int speed)
{
    unsigned char sbuf[20] = { 0 };
    static BOOL com_read_status = TRUE;
    int status = 1;

    sbuf[0] = 0x5A;							// USB_ISS command
    sbuf[1] = 0x02;							// Set mode
    //sbuf[2] = 0x60;						// Set mode to 100KHz I2C
    sbuf[2] = 0x70;							// Set mode to 400KHz I2C. This is the default
    //sbuf[2] = 0x80;						// Set mode to 1000KHz I2C
    sbuf[3] = 0x00;							// Spare pins set to output low

    switch (speed) {
    case 1:
        sbuf[2] = 0x60;
        break;
    case 2:
        sbuf[2] = 0x70;
    default:
        sbuf[2] = 0x70;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_i2c_mode. STARTED. I2C Speed: 0x%X\n", line_number++,sbuf[2]);
    bytes_to_write = 4;
    error = WriteFile(USB_hSerial, sbuf, bytes_to_write, &bytes_written, NULL);
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_i2c_mode. WriteFile. FAILED %s\n", line_number++, strerror(errno));
        status = 0;
    }
    com_read_status = ReadFile(USB_hSerial, sbuf, 2, &dwBytesRead, NULL);
    if (com_read_status <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_i2c_mode. ReadFile. FAILED %s\n", line_number++, strerror(errno));
        status = 0;
    }
    if (sbuf[0] != 0xFF)						// If first returned byte is not 0xFF then an error has occured
    {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_i2c_mode. sbuf: %s. FAILED \n", line_number++, (char*)sbuf[0]);
        status = 0;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_i2c_mode. FINISHED \n", line_number++);
    return status;
}

int Get_USB_ISS_Port() {
    FILE* Multus_ini;
    char file_name[MAX_PATH] = { 0 };
    int length = 0;
    //int cmd_value;
    char* parameter;
    char* ptr;
    char temp_serial_number[MAX_PATH];
    char record[MAX_PATH];
    int status = 1;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_USB_ISS_Port -> Default Path: %s\n", line_number++, homedir);
        strcat(file_name, homedir);
        strcat(file_name, "/i2c.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_USB_ISS_Port -> Multus.ini Path: %s\n", line_number++, file_name);
        Multus_ini = fopen(file_name, "r");
        if (Multus_ini != NULL) {
            while (fgets(record, sizeof(record), Multus_ini) != NULL) {
                parameter = strstr(record, "G_I2C_SERIAL_PORT=");
                if (parameter != NULL) {
                    length = strlen("G_I2C_SERIAL_PORT=");
                    strcpy(temp_serial_number, &parameter[length]);
                    ptr = strtok(temp_serial_number, ";");
                    sprintf(USB_ISS_Port_Name, "COM%s", ptr);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Get_USB_ISS_Port -> USB_ISS_Port: %s\n", line_number++, USB_ISS_Port_Name);
                }
            }
            fclose(Multus_ini);
        }
        else {
            status = 0;
        }
    }
    else {
        status = 0;
    }
    return status;
}

char* itoa_my(int value) {
    static char buffer[12] = { 0 }; // 12 bytes is big enough for an INT32
    int original = value; // save original value

    int c = sizeof(buffer) - 1;

    buffer[c] = 0; // write trailing null in last byte of buffer    

    if (value < 0) // if it's negative, note that and take the absolute value
        value = -value;

    do // write least significant digit of value that's left
    {
        buffer[--c] = (value % 10) + '0';
        value /= 10;
    } while (value);

    if (original < 0)
        buffer[--c] = '-';

    return &buffer[c];
}

int Write_Serial_Number(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size, char* caller) {
    int status = 1;
    unsigned char command_buffer[MAX_PATH] = { 0 };
    int write_bytes_written = 0;
    int write_bytes = 0;
    int error = 0;
    int read_count = 0;
    int read_status = 0;

    command_buffer[0] = 0x5A;
    command_buffer[1] = I2C_SET_SER_NUM;
    memcpy(&command_buffer[2], buffer, buffer_size);
    write_bytes = buffer_size + 2;

    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    if (error <= 0) {
        dw = GetLastError();
        status = -1;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        fprintf(stdout, "[%d] Write_Serial_Number. address: 0x%X, WriteFile FAILED %d, %s\n", line_number++, address, dw,
            (char*)lpMsgBuf);
    }
    else {
        Sleep(200);
        read_status = ReadFile(USB_hSerial, command_buffer, 1, &read_count, NULL);
        if (read_status <= 0) {
            read_count = -1;
            status = -1;
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            fprintf(stdout, "[%d] Write_Serial_Number. address: 0x%X, ReadFile. FAILED %d\n", line_number++, dw, address);
        }
        else {
            if (command_buffer[0] != 0) {
                status = command_buffer[0];
                //fprintf(stdout, "[%d] Write_I2C_AD0. address: 0x%X, SUCCESS 0x%X:%X\n", line_number++, address,
                 //   command_buffer[0], command_buffer[1]);
            }
            else {
                fprintf(stdout, "[%d] Write_Serial_Number. address: 0x%X, FAILED 0x%X:%X\n", line_number++, address,
                    command_buffer[0], command_buffer[1]);
                status = -1;
            }
        }
    }
    return status;
}

int set_serial_number(int serial_number) {
    int status = 0;
    char filename[2] = { 0 };
    unsigned char sbuf[20] = { 0 };
    int com_read_status = 0;
    int count = 0;
    char temp_string[8] = { '0' };
    int y = 0;
    char* number;

    memset(temp_string, '0', 8);
    number = itoa_my(serial_number);
    temp_string[7] = number[0];
    for (count = 0; count < 8; count++) {
        fprintf(stdout, "%c", temp_string[count]);
    }
    fprintf(stdout, "\n");
    for (count = 0; count < 8; count++) {
        sbuf[count] = (byte)temp_string[count];
        y += sbuf[count];
    }
    y = (0 - y) & 0xff;
    sbuf[8] = (byte)y;
    for (count = 0; count < 8; count++) {
        fprintf(stdout, "%c", sbuf[count]);
    }
    fprintf(stdout, "\n%d", sbuf[8]);
    status = Write_Serial_Number(6, filename, E_display_addr, 0, sbuf, 9, "DISPLAY_PrintString");
    fprintf(stdout, "\nstatus: %d\n", status);
    return status;
}

int get_serial(void)
{
    unsigned char sbuf[20] = { 0 };
    int com_read_status = 0;

    sbuf[0] = 0x5A; 						// USB_ISS byte
    sbuf[1] = 0x03;							
    int count = 0;
    int serial_number = 0;

    bytes_to_write = 2;
    error = WriteFile(USB_hSerial, sbuf, bytes_to_write, &bytes_written, NULL);
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile,"[%d] get_serial. WriteFile. FAILED. error: % ld\n", line_number++,dw);
    }
    memset(sbuf, 0, sizeof(sbuf));
    com_read_status = ReadFile(USB_hSerial, sbuf, 8, &dwBytesRead, NULL);
    if (com_read_status <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile,"[%d] get_serial. ReadFile. FAILED. error: %ld\n", line_number++,dw);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] USB - ISS Serial Number : ", line_number++);
    for (count = 0; count < 8; count++) {
        fprintf(G_fp_logfile,"%c", sbuf[count]);
    }
    fprintf(G_fp_logfile,"\n");
    serial_number = atoi(sbuf);
    return serial_number;
}

int Open_USB_ISS_1() {
    int status = TRUE;
    DWORD read_timeout = 0;
    char windows_comm_port[20] = { 0 };
    char error_message[80] = { 0 };
    int intialize_status = 1;
    int serial_number = 0;
    COMMTIMEOUTS lpCommTimeouts;

    if (Get_USB_ISS_Port() != 0) {
        sprintf(windows_comm_port, "\\\\.\\%s", USB_ISS_Port_Name);
        USB_hSerial = CreateFileA(windows_comm_port,
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0);

        if (USB_hSerial == INVALID_HANDLE_VALUE) {
            if (dw = GetLastError() == ERROR_FILE_NOT_FOUND) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. COM Port: %s Open Failed \n", line_number++, windows_comm_port);
            }
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. Error Opening Com Port : % s % s\n", 
                                                                    line_number++, windows_comm_port, (char*)lpMsgBuf);
            status = FALSE;
        }
        if (status) {
            USB_dcbSerialParams.BaudRate = 9600;
            USB_dcbSerialParams.DCBlength = sizeof(USB_dcbSerialParams);
            if (!GetCommState(USB_hSerial, &USB_dcbSerialParams)) {
                dw = GetLastError();
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dw,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf,
                    0, NULL);
                status = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. Error Retrieving serial port state: %s %s\n", line_number++,
                    windows_comm_port, (char*)lpMsgBuf);
            }
        }
        if (status == TRUE) {
            if (GetCommTimeouts(USB_hSerial, &lpCommTimeouts)) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. GetCommTimouts. ReadIntervalTimeout: %ld, ReadTotalTimeoutConstant: %ld\n",
                        line_number++, lpCommTimeouts.ReadIntervalTimeout , lpCommTimeouts.ReadTotalTimeoutConstant);
                lpCommTimeouts.ReadIntervalTimeout = 10;
                lpCommTimeouts.ReadTotalTimeoutConstant = 10;
                lpCommTimeouts.ReadTotalTimeoutMultiplier = 10;
                lpCommTimeouts.WriteTotalTimeoutConstant = 10;
                lpCommTimeouts.WriteTotalTimeoutMultiplier = 10;
                if (!SetCommTimeouts(USB_hSerial, &lpCommTimeouts)) {
                    status = FALSE;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. SetCommTimeouts. FAILED\n", line_number++);
                }
            }
            else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. GetCommTimouts. FAILED \n", line_number++);
                status = FALSE;
            }
        }
        if (status == TRUE) {
            USB_dcbSerialParams.BaudRate = CBR_115200;
            USB_dcbSerialParams.StopBits = ONESTOPBIT;
            USB_dcbSerialParams.DCBlength = sizeof(USB_dcbSerialParams);
            if (!SetCommState(USB_hSerial, &USB_dcbSerialParams)) {
                dw = GetLastError();
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dw,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf,
                    0, NULL);
                status = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. SetCommState. FAILED %s %s\n", line_number++,
                    windows_comm_port, (char*)lpMsgBuf);
            }
        }
    }
    else {
        fprintf(G_fp_logfile, "[%d] Open_USB_ISS_1. SERIAL PORT NOT FOUND. \n", line_number++);
    }
    return status;
}

/*int Open_USB_ISS() {
    int status = TRUE;
    DWORD read_timeout = 0;
    char windows_comm_port[20] = { 0 };
    char error_message[80] = { 0 };
    int intialize_status = 1;
    int serial_number = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Open_USB_ISS. STARTED \n", line_number++);
   
    status = Open_USB_ISS_1();
    if (status == FALSE) {
        Stop_all(0, STOP_REGO_INITIALIZATION);
    }
    if (status == TRUE) {
        //USB_Serial_port_open = TRUE;
        serial_number = get_serial();
        if (serial_number == 0) {
            MessageBoxA(NULL, "REGO SERIAL NUMBER NOT SET. RUN SET SERIAL NUMBER UTILITY. \r\nMSCC-PW will now STOP.",
                "MS-SDR", MB_OK);
            Stop_all(0, STOP_NORMAL);
        }
        Get_Version();
        intialize_status = Initialize_i2c_mode(0);
        if (intialize_status == FALSE) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Open_USB_ISS. Initialize_i2c_mode. FAILED\n", line_number++);
            status = FALSE;
            Stop_all(0, STOP_REGO_INITIALIZATION);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Open_USB_ISS. BaudRate %d, StopBits: %d, status: %d.  FINISHED\n", line_number++,
        USB_dcbSerialParams.BaudRate, USB_dcbSerialParams.StopBits, status);
    return status;
}*/

void Close_USB_ISS() {
    char command_buffer[4] = { 0 };
    int read_status = 0;
    int status = 0;
    //int read_count = 0;

    if (USB_hSerial != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Close_USB_ISS. read_operation_count: %d, write_operation_count: %d\n", line_number++,
            read_operation_count, write_operation_count);
        CancelIoEx(USB_hSerial, NULL);
        /*read_status = ReadFile(USB_hSerial, command_buffer, 1, &read_count, NULL);
        if (read_status <= 0) {
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Close_USB_ISS. ReadFile. FAILED %d\n", line_number++,dw);
        }*/
        status = CloseHandle(USB_hSerial);
        if (status == 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Close_USB_ISS. CloseHandle. FAILED\n", line_number++);
        }
    }
}

int Read_I2C_SGL(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size) {
    int status = -1;
    int read_device_address = 0;
    DWORD read_count = 0;
    int read_status = 0;
    int write_bytes = 2;
    int write_bytes_written = 0;
    unsigned char command_buffer[MAX_PATH] = { 0 };
    unsigned char read_data = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_i2c. ReadFile. STARTED\n", line_number++);
    read_device_address = address << 1;
    read_device_address = read_device_address | 1;
    command_buffer[0] = I2C_SGL; //0x53
    command_buffer[1] = read_device_address;
    command_buffer[2] = buffer_size;

    write_operation_count++;
    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    write_operation_count--;
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_I2C_SGL. address: %d, WriteFile FAILED %d\n", line_number++, address, dw);
    }
    else {
        if (write_bytes_written != write_bytes) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_I2C_SGL. WriteFile. write_bytes: %d, write_bytes_written: %d\n",
                line_number++, write_bytes, write_bytes_written);
            Sleep(1);
        }
        memset(command_buffer, 0, sizeof(command_buffer));
        Sleep(5);
        read_operation_count++;
        read_status = ReadFile(USB_hSerial, command_buffer, 1, &read_count, NULL);
        read_operation_count--;
        if (read_status <= 0) {
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_I2C_SGL. address: %d, ReadFile. FAILED %d\n", line_number++, address, dw);
        }
        else {
            status = read_count;
            memcpy(buffer, command_buffer, read_count);
            read_data = command_buffer[0];
        }
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_I2C_SGL. FINISHED. read_count: %d, command_buffer: %d\n", 
    //                           line_number++, status, read_data);
    return status;
}

int Read_i2c(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size) {
    int status = -1;
    int read_device_address = 0;
    DWORD read_count = 0;
    int read_status = 0;
    int write_bytes = 4;
    int write_bytes_written = 0;
    unsigned char command_buffer[MAX_PATH] = { 0 };
    unsigned char error_buffer[MAX_PATH] = { 0 };

    I2C_BUSY = TRUE;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_i2c. ReadFile. STARTED\n", line_number++);
    read_device_address = address << 1;
    read_device_address = read_device_address | 1;
    command_buffer[0] = I2C_AD1;
    command_buffer[1] = read_device_address;
    command_buffer[2] = daddress;
    command_buffer[3] = buffer_size;
   
    write_operation_count++;
    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    write_operation_count--;
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_i2c. address: %d, WriteFile FAILED\n", line_number++,address);
    }
    else {
        if (write_bytes_written != write_bytes) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_i2c. WriteFile. write_bytes: %d, write_bytes_written: %d\n",
                line_number++, write_bytes, write_bytes_written);
            Sleep(1);
        }
        read_operation_count++;
        read_status = ReadFile(USB_hSerial, command_buffer, buffer_size, &read_count, NULL);
        read_operation_count--;
        if (read_status <= 0) {
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_i2c. address: %d, ReadFile. FAILED %s\n", line_number++, 
                address,(char*)lpMsgBuf);
        }
        else {
            status = read_count;
            memcpy(buffer, command_buffer, read_count);
        }
    }
    I2C_BUSY = FALSE;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_i2c. FINISHED.\n", line_number++);
    return status;
}

int Read_i2c_AD0(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size, char *caller) {
    int status = -1;
    int read_device_address = 0;
    DWORD read_count = 0;
    int read_status = 0;
    int write_bytes = 3;
    int write_bytes_written = 0;
    unsigned char command_buffer[MAX_PATH] = { 0 };

    I2C_BUSY = TRUE;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_i2c. ReadFile. STARTED\n", line_number++);
    read_device_address = address << 1;
    read_device_address = read_device_address | 1;
    command_buffer[0] = I2C_AD0;
    command_buffer[1] = read_device_address;
    command_buffer[2] = buffer_size;
 
    write_operation_count++;
    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    write_operation_count--;
    if (error <= 0) {
        dw = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_i2c_AD0. address: %d, WriteFile FAILED %d, caller: %s\n", 
            line_number++,address,dw,caller);
    }
    else {
        if (write_bytes_written != write_bytes) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_i2c_AD0. WriteFile. write_bytes: %d, write_bytes_written: %d\n",
                line_number++, write_bytes, write_bytes_written);
            Sleep(1);
        }
        read_operation_count++;
        read_status = ReadFile(USB_hSerial, command_buffer, buffer_size, &read_count, NULL);
        read_operation_count--;
        if (read_status <= 0) {
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_i2c_AD0. address: %d, ReadFile. FAILED %d, caller: %s\n", 
                line_number++, address,dw,caller);
        }
        else {
            status = read_count;
            memcpy(buffer, command_buffer, read_count);
        }
    }
    if (status == -1 && Failure_Reported == FALSE) {
        Failure_Reported = TRUE;
        //Stop_all(0, STOP_I2C);
    }
    I2C_BUSY = FALSE;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_i2c. FINISHED.\n", line_number++);
    return status;
}

int Write_i2c(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size, char* caller) {
	int status = 1;
    unsigned char command_buffer[MAX_PATH] = { 0 };
    int write_bytes_written = 0;
    int write_bytes = 0;
    int error = 0;
    int read_count = 0;
    int read_status = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Write_i2c. STARTED.  Caller: %s\n", line_number++,caller);
    I2C_BUSY = TRUE;
    command_buffer[0] = I2C_AD1;
    command_buffer[1] = address << 1;
    command_buffer[2] = daddress;
    command_buffer[3] = buffer_size;
    memcpy(&command_buffer[4], buffer, buffer_size);
    write_bytes = buffer_size + 4;
    write_operation_count++;
    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    write_operation_count--;
    if (error <= 0) {
        dw = GetLastError();
        status = -1;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_i2c. address: %d, WriteFile FAILED %s\n", line_number++, address, (char*)lpMsgBuf);
    } else {
        read_operation_count++;
        read_status = ReadFile(USB_hSerial, command_buffer, 1, &read_count, NULL);
        read_operation_count--;
        if (read_status <= 0) {
            read_count = -1;
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Read_i2c. address: %d, ReadFile. FAILED %s\n", line_number++, address, 
                (char*)lpMsgBuf);
        }
        else {
            if (command_buffer[0] != 0) {
                status = command_buffer[0];
            }
            else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Write_i2c. address: %d, FAILED %u\n", line_number++, address, command_buffer[0]);
                status = -1;
            }
        }
    }
    I2C_BUSY = FALSE;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Write_i2c. FINISHED\n", line_number++);
	return status;
}

int Write_I2C_AD0(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size, char* caller) {
    int status = 1;
    unsigned char command_buffer[MAX_PATH] = { 0 };
    int write_bytes_written = 0;
    int write_bytes = 0;
    int error = 0;
    int read_count = 0;
    int read_status = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Write_I2C_AD0. STARTED.  Caller: %s\n", line_number++,caller);
    I2C_BUSY = TRUE;
    command_buffer[0] = I2C_AD0;
    command_buffer[1] = address << 1;
    command_buffer[2] = buffer_size;
    memcpy(&command_buffer[3], buffer, buffer_size);
    write_bytes = buffer_size + 3;
    
    write_operation_count++;
    error = WriteFile(USB_hSerial, command_buffer, write_bytes, &write_bytes_written, NULL);
    write_operation_count--;
    if (error <= 0) {
        dw = GetLastError();
        status = -1;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_I2C_AD0. address: %d, WriteFile FAILED %d, %s, caller: %s\n", line_number++, address, dw,
            (char *)lpMsgBuf,caller);
    }
    else {
        read_operation_count++;
        read_status = ReadFile(USB_hSerial, command_buffer, 1, &read_count, NULL);
        read_operation_count--;
        if (read_status <= 0) {
            read_count = -1;
            status = -1;
            dw = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Write_I2C_AD0. address: %d, ReadFile. FAILED %d\n", line_number++,dw,address);
        }
        else {
            if (command_buffer[0] != 0) {
                status = command_buffer[0];
            }
            else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Write_I2C_AD0. address: 0x%X, FAILED %u, caller: %s\n", 
                    line_number++, address,command_buffer[0],caller);
                status = -1;
            }
        }
    }
    I2C_BUSY = FALSE;
    if (status == -1 && Failure_Reported == FALSE) {
        Failure_Reported = TRUE;
        //Stop_all(0, STOP_I2C);
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Write_I2C_SGL. FINISHED\n", line_number++);
    return status;
}