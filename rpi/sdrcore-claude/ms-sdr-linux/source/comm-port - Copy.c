#define _CRT_SECURE_NO_WARNINGS 1
#define __USE_MISC 1
#include "extern.h"
#include "usbavrcmd.h"
#include "version.h"


int open_comm_port();
void *Comms_port_thread(void *myparam);

pthread_t p_Comms_port_thread;
int p_Comms_port_thread_rc;

int Serial_port_open = 0;
int G_comms_process_running = 0;
int G_Comms_port_thread_running = 0;

pthread_t p_Pin_Check_thread;
int p_Pin_Check_thread_rc;
int G_Pin_Check_running;

int tx_mode = 0;
int G_pins = 0;
int G_previous_G_pins = 0;

int comm_port_open = 0;
uint8_t comport_number = 0;
char port_number[80];
char G_comm_port[MAX_PATH] = { 0 };
char comm_port[20];
//int comm_port_size = 0;

char lastError[1024];
int baud_rate = 9600;
int parity = 0;
int stop_bits = 0;
int data_bits = 8;

int baud_rates[8] = { 1200,2400,4800,9600,19200,38400,57600,115200 };
int parity_values[3] = { PARITY_NONE,PARITY_EVEN,PARITY_ODD };
int stop_bit_values[2] = { ONESTOPBIT,TWOSTOPBITS };
int data_bit_values[3] = { 7,8,9 };

HANDLE hSerial = 0;
DWORD com_time_out = 20;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };

int comm_name_index = 0;
int baud_rate_index = 0;
int parity_index = 0;
int stop_bit_index = 0;
int data_bit_index = 0;
const char *homedir;

/*int update_comm_port_ini() {
    FILE *fp_Comm_port_ini;
    char file_name[MAX_PATH] = {0};

    int lenght = 0;
    int record = 0;
    int index = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] update_comm_port_ini -> Called.\n", line_number++);

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] update_comm_port_ini -> Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/comm-port.ini");
        fp_Comm_port_ini = fopen(file_name, "w");
        if (fp_Comm_port_ini != NULL) {
            fprintf(fp_Comm_port_ini, "COMM_PORT_NAME=%s,COMM_PORT_INDEX=%d,BAUD_RATE_INDEX=%d,PARITY_INDEX=%d,DATA_BITS_INDEX=%d,STOP_BITS_INDEX=%d,PIN=%d;\n",
                    G_comm_port, comm_name_index, baud_rate_index, parity_index, data_bit_index, stop_bit_index, G_pins);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] update_comm_port_ini -> Open file failed\n", line_number++);
            return 0;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] update_comm_port_ini -> Finished\n", line_number++);
        fclose(fp_Comm_port_ini);
    }
    return 1;
}*/
void *Pin_Check_Thread(void *param) {
    int status = 0;
    static int previous_tx_mode = 0;
    static int previous_G_pins = 0;
    BOOL fCTS, fDSR, fRING, fRLSD, fDTR, fDCD;
    BOOL fCTS_set = 0, fDCD_set = 0;
    int lpModemStat;
    BOOL reported = 0;


    if (G_Pin_Check_running > 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. Already Running -> Returning \n", line_number++);
        pthread_exit(NULL);
        return (NULL);
    }
    G_Pin_Check_running++;
    Sleep(1000);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. STARTED. \n", line_number++);

    while (G_pins) {//Main loop
        if (previous_G_pins != G_pins) {
            previous_G_pins = G_pins;
        }
        fCTS = 0;
        fDSR = 0;
        fRING = 0;
        fRLSD = 0;
        fDTR = 0;
        fDCD = 0;
        lpModemStat = 0;
        if (hSerial != NULL) {
            status = GetCommModemStatus(hSerial, &lpModemStat);
        }
        fCTS = MS_CTS_ON & lpModemStat;
        fDSR = MS_DSR_ON & lpModemStat;
        fRING = MS_RING_ON & lpModemStat;
        fRLSD = MS_RLSD_ON & lpModemStat;

        switch (G_pins) {
            case 1:
                if (fCTS) {
                    fCTS_set = TRUE;
                    if (!reported) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. fCTS ON\n", line_number++);
                        reported = TRUE;
                    }
                    tx_mode = 1;
                } else {
                    if (fCTS_set == TRUE) {
                        tx_mode = 0;
                        fCTS_set = FALSE;
                        reported = FALSE;
                    }
                }
                break;
            case 2:
                if (fDCD) {
                    fDCD_set = TRUE;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. fDCD ON\n", line_number++);
                    tx_mode = 1;
                } else {
                    if (fDCD_set == TRUE) {
                        tx_mode = 0;
                        fDCD_set = FALSE;
                    }
                }
                break;
        }
        if (previous_tx_mode != tx_mode) {
            Set_PTT(tx_mode,FALSE);
            Gui_send_param(CMD_SET_TX_SET_BY_SERVER, tx_mode);
            previous_tx_mode = tx_mode;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. FINISHED. tx_mode: %d\n",
                    line_number++, tx_mode);
        }
        Sleep(1);
    }
    G_Pin_Check_running = 0;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Pin_Check_Thread. STOPPED \n", line_number++);
    return (NULL);
}

void UintToStr(uint8_t *s, uint32_t bin, unsigned char n) {
    s += n;
    *s = '\0';

    while (n--) {
        *--s = (bin % 10) + '0';
        bin /= 10;
    }
}
int write_to_comm_port(uint8_t* buffer, int size) {
    int status = 0;

    DWORD bytes_to_write;
    DWORD bytes_written;
    BOOL error;


    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] write_to_comm_port\n", line_number++);
    bytes_to_write = size;
    error = WriteFile(hSerial, buffer, bytes_to_write, &bytes_written, NULL);
    if (error == 0) {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastError,
            1024,
            NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] write_to_comm_port -> FAILED writing to comm port: %s \n", line_number++, lastError);
        MessageBoxA(NULL, " write_to_comm_port FAILED., Please forward log files to Multus SDR, LLC ", "MS-SDR",
            MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
        Serial_port_open = 0;
        if (hSerial != NULL) {
            CloseHandle(hSerial);
        }
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] write_to_comm_port-> bytes written: %d\n", line_number++, bytes_written);
    return status;
}
int parse_record(char *receive_buffer, int size) {
    int status = TRUE;
    char *record_start = NULL;
    char *data_record_end = NULL;
    int index = 0;
    int freq_index = 0;
    int command = NO_COMMAND_FOUND;
    static uint8_t write_buffer[120];
    uint8_t freq[12] = {'0'};
    int freq_size;
    char received_freq[24] = {'0'};
    uint32_t frequency = 0;
    uint8_t mode = 'Z';
    char print_buffer[256] = {0};
    uint16_t band = 0;

    memset(write_buffer, 0, sizeof (write_buffer));
    memcpy(print_buffer, receive_buffer, size);
    //print_time(1);
    //fprintf(G_fp_logfile, "[%d] parse_record -> Start -> size received: %d, record: %s\n", line_number++, size, print_buffer);

    switch (G_mode) {
        case 'L':
            mode = '1';
            break;
        case 'U':
            mode = '2';
            break;
        case 'C':
            mode = '3';
            break;
        case 'A':
            mode = '5';
            break;
        case 'F':
            mode = '4';
            break;
    }

    record_start = strstr(receive_buffer, "AI;");
    if (record_start != NULL) {
        command = AI;
        write_buffer[0] = 'A';
        write_buffer[1] = 'I';
        write_buffer[2] = '0';
        write_buffer[3] = ';';
        write_to_comm_port(write_buffer, 4);
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> AI Received.\n", line_number++);
        memset(print_buffer, 0, sizeof (print_buffer));
        memcpy(print_buffer, write_buffer, 4);
    }

    record_start = strstr(receive_buffer, "ID;");
    if (record_start != NULL) {
        command = ID0;
        write_buffer[0] = 'I';
        write_buffer[1] = 'D';
        write_buffer[2] = '0';
        write_buffer[3] = '1';
        write_buffer[4] = '9';
        write_buffer[5] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> ID Received.\n", line_number++);
        fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 6);
    }

    record_start = strstr(receive_buffer, "IF;");
    if (record_start != NULL) {
        command = IF;
        UintToStr(freq, G_tune_freq, (11));
        write_buffer[0] = 'I';
        write_buffer[1] = 'F';
        index = 0;
        freq_index = 0;
        for (index = 2; index < 13; index++) {
            write_buffer[index ] = freq[freq_index++];
        }
        for (index = 13; index < 28; index++) {
            write_buffer[index] = '0';
        }
        if (tx_mode == 1) {
            write_buffer[28] = '1';
        } else {
            write_buffer[28] = '0';
        }

        write_buffer[29] = mode;
        for (index = 30; index < 37; index++) {
            write_buffer[index] = '0';
        }
        write_buffer[37] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> IF Received. Sending: %s\n", line_number++, freq);
        //fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 38);

    }

    record_start = strstr(receive_buffer, "MD;");
    if (record_start != NULL) {
        command = MD;
        write_buffer[0] = 'M';
        write_buffer[1] = 'D';
        write_buffer[2] = mode;
        write_buffer[3] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> MD Received -> Sending ->  Mode: [%d,%c]\n",
        //        line_number++, write_buffer[2], write_buffer[2]);
        //fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 4);

    }

    record_start = strstr(receive_buffer, "PS;");
    if (record_start != NULL) {
        command = PS;
        write_buffer[0] = 'P';
        write_buffer[1] = 'S';
        write_buffer[2] = '1';
        write_buffer[3] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> MD Received -> Sending ->  Mode: [%d,%c]\n",
        //        line_number++, write_buffer[2], write_buffer[2]);
        //fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 4);
    }

    record_start = strstr(receive_buffer, "RM");
    if (record_start != NULL) {
        command = RM;

        write_buffer[0] = 'R';
        write_buffer[1] = 'M';

        if (record_start[2] == ';') {
            write_buffer[2] = '1';                    // Default to meter 1
        }
        else {
            write_buffer[2] = record_start[2];
        }

        // === Use the same logic as your WPF client ===
        int smeter_value = 0;
        int dbm = G_smeter_dBm;                       // ← Use your actual dBm variable here

        if (dbm <= -130) {
            smeter_value = 0;
        }
        else if (dbm <= -73) {
            // S1 to S9
            smeter_value = 9 + (dbm + 73) / 6;
            if (smeter_value < 1) smeter_value = 1;
        }
        else {
            // Above S9
            int db_over_s9 = dbm + 73;
            int over_value = (db_over_s9 + 5) / 10;

            if (over_value <= 0)
                smeter_value = 9;
            else if (over_value >= 3)
                smeter_value = 12;                    // Firmware limit
            else
                smeter_value = 9 + over_value;
        }

        // Convert to 4-digit ASCII using your existing function
        UintToStr(&write_buffer[3], (uint32_t)smeter_value, 4);
        write_buffer[7] = ';';
        write_to_comm_port(write_buffer, 8);
    }

    record_start = strstr(receive_buffer, "FW;");
    if (record_start != NULL) {
        command = FW;
        write_buffer[0] = 'F';
        write_buffer[1] = 'W';
        write_buffer[2] = '3';
        write_buffer[3] = '0';
        write_buffer[4] = '0';
        write_buffer[5] = '0';
        write_buffer[6] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> MD Received -> Sending ->  Mode: [%d,%c]\n",
        //        line_number++, write_buffer[2], write_buffer[2]);
        //fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 7);
    }

    record_start = strstr(receive_buffer, "KS;");
    if (record_start != NULL) {
        command = KS;
        write_buffer[0] = 'K';
        write_buffer[1] = 'S';
        write_buffer[2] = '0';
        write_buffer[3] = '1';
        write_buffer[4] = '8';
        write_buffer[5] = ';';
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record-> MD Received -> Sending ->  Mode: [%d,%c]\n",
        //        line_number++, write_buffer[2], write_buffer[2]);
        //fflush(G_fp_logfile);
        write_to_comm_port(write_buffer, 6);
    }

    /*record_start = strstr(work_buffer_ptr, "MD1;");
    if (record_start != NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received\n", line_number++);
            mode_number = atoi(&record_start[2]);
            G_mode = mode_to_letter(mode_number);
            command = MD;
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, mode_number);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, mode_number);
            ModeChanged(G_mode);
            Gui_send_param(CMD_MODE_SET_BY_SERVER, G_mode);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received -> MD{%d}, G_mode: %c\n", line_number++, MD_Set_Command, G_mode);
            process_count += strlen("MD3;");
            work_buffer_ptr += strlen("MD3;");
            continue;
    }

    record_start = strstr(work_buffer_ptr, "MD2;");
    if (record_start != NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received\n", line_number++);
            mode_number = atoi(&record_start[2]);
            G_mode = mode_to_letter(mode_number);
            command = MD;
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, mode_number);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, mode_number);
            ModeChanged(G_mode);
            Gui_send_param(CMD_MODE_SET_BY_SERVER, G_mode);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received -> MD{%d}, G_mode: %c\n", line_number++, MD_Set_Command, G_mode);
            process_count += strlen("MD3;");
            work_buffer_ptr += strlen("MD3;");
            continue;
    }

    record_start = strstr(work_buffer_ptr, "MD3;");
    if (record_start != NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received\n", line_number++);
            mode_number = atoi(&record_start[2]);
            G_mode = mode_to_letter(mode_number);
            command = MD;
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, mode_number);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, mode_number);
            ModeChanged(G_mode);
            Gui_send_param(CMD_MODE_SET_BY_SERVER, G_mode);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Parse Record -> Received -> MD{%d}, G_mode: %c\n", line_number++, MD_Set_Command, G_mode);
            process_count += strlen("MD3;");
            work_buffer_ptr += strlen("MD3;");
            continue;
    }*/

    record_start = strstr(receive_buffer, "FA;");
    if (record_start != NULL) {
        ////print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record -> FA Received -> G_tune_freq: %d\n", 
        //                line_number++, G_tune_freq);
        command = FA;
        freq_index = 0;
        UintToStr(freq, G_tune_freq, (11));
        write_buffer[0] = 'F';
        write_buffer[1] = 'A';
        for (index = 2; index < 13; index++) {
            write_buffer[index] = freq[freq_index++];
        }
        write_buffer[13] = ';';
        write_to_comm_port(write_buffer, 14);
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record -> FA Received -> Finished -> Sending: %d\n",
        //       line_number++, G_tune_freq);
    }

    record_start = strstr(receive_buffer, "FA0");
    if (record_start != NULL) {
        command = FA;
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] parse_record -> FA with Freq Received \n", line_number++);
        record_start = strstr(receive_buffer, "FA");
        if (record_start != NULL) {
            data_record_end = strstr(record_start, ";");
            if (data_record_end != NULL) {
                freq_size = (data_record_end - (record_start + 2));
                strncpy(received_freq, (record_start + 2), freq_size);
                frequency = atol(received_freq);
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] parse_record -> FA with Freq Received  -> freq_size: %d, frequency: %d\n",
                //        line_number++, freq_size, frequency);
                G_tune_freq = frequency;
                freq_queue_add(frequency);
                band = Get_band_by_frequency(G_tune_freq, TRUE);
                Gui_send_param(CMD_SET_BAND, band);
                Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq);
            }
        }
    }

    record_start = strstr(receive_buffer, "FB;");
    if (record_start != NULL) {
        command = FB;
    }

    record_start = strstr(receive_buffer, "RX;");
    if (record_start != NULL) {
        command = RX;
        write_buffer[0] = 'R';
        write_buffer[1] = 'X';
        write_buffer[2] = '0';
        write_buffer[3] = ';';
        tx_mode = 0;
        Set_PTT(tx_mode,TRUE);
        Gui_send_param(CMD_SET_TX_SET_BY_SERVER, tx_mode);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] parse_record. RX Received.\n", line_number++);
    }

    record_start = strstr(receive_buffer, "TX;");
    if (record_start != NULL) {
        command = TX;
        write_buffer[0] = 'T';
        write_buffer[1] = 'X';
        write_buffer[2] = '0';
        write_buffer[3] = ';';
        tx_mode = 1;
        Set_PTT(tx_mode,FALSE);
        Gui_send_param(CMD_SET_TX_SET_BY_SERVER, tx_mode);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] parse_record. TX Received.\n", line_number++);
    }
    if (command == NO_COMMAND_FOUND) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] parse_record. Invalid Data Record. Discarding Record: %s\n", line_number++, receive_buffer);
        status = FALSE;
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] parse_record-> Finished \n", line_number++);
    return (status);
}
void* Comms_port_thread(void* myparam) {
    //int n = 8192;
    int n = 132;
    char szBuff[8196] = { 0 };
    DWORD dwBytesRead = 0;
    DWORD lasterror = 0;
    //char lastError[1024];
    static BOOL com_read_status = TRUE;
    int status = FALSE;
    int parse_status = TRUE;
    int error_loop_count = 0;
    LPTSTR error_buffer = { 0 };
    //int pin_status;
    //DWORD lpModemStat;

    if (G_Comms_port_thread_running > 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Comms_port_thread -> Already Running -> Returning \n", line_number++);
        pthread_exit(0);
        return(NULL);
    }
    G_Comms_port_thread_running++;
    Sleep(1000);
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Comms_port_thread. Started \n", line_number++);
    comm_port_open = 1;
    while (G_comms_process_running) {//Main loop
        memset(szBuff, 0, sizeof(szBuff));
        com_read_status = ReadFile(hSerial, szBuff, n, &dwBytesRead, NULL);
        if (com_read_status == 0) {
            lasterror = GetLastError();
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastError,
                1024,
                NULL);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Comms_port_thread.  Error Reading Com Port: %s \n", line_number++, lastError);
            error_loop_count++;
            if (error_loop_count > 100) {
                MessageBoxA(NULL, "MS-SDR -> Comm Port Read FAILED. Closing Comm Port. Please forward log files to Multus SDR, LLC ", "MS-SDR",
                    MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
                G_comms_process_running = 0;
            }
        }
        else {
            if (dwBytesRead > 0) {
                parse_status = parse_record(szBuff, (int)dwBytesRead);
                error_loop_count = 0;
            }

        }
    }//End main loop
    CloseHandle(hSerial);
    hSerial = NULL;
    comm_port_open = 0;
    G_Comms_port_thread_running = 0;
    Sleep(1);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Comms_port_thread. NORMAL EXIT. \n", line_number++);
    pthread_exit(0);
    return(NULL);
}

int open_comm_port() {
    int status = TRUE;
    DWORD read_timeout = 0;
    char windows_comm_port[20] = { 0 };
    char error_message[80] = { 0 };

    sprintf(windows_comm_port, "\\\\.\\%s", G_comm_port);
    hSerial = CreateFileA(windows_comm_port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (hSerial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port. COM Port: %s Open Failed \n", line_number++, windows_comm_port);
        }
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            ERROR_FILE_NOT_FOUND,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastError,
            1024,
            NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] open_comm_port. Error Opening Com Port: %s %s\n", line_number++, windows_comm_port, lastError);
        MessageBoxA(NULL, "SERIAL PORT OPEN FAILED. \r\n", "MS-SDR",
            MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
        status = FALSE;
    }
    if (status) {
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastError,
                1024,
                NULL);
            status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Retrieving serial port state: %s %s\n", line_number++, comm_port, lastError);
            MessageBoxA(NULL, "MS-SDR -> ERROR Retrieving Comm Port State.  Please forward log files to Multus SDR, LLC ",
                "MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);

        }
    }
    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = data_bits;
    dcbSerialParams.StopBits = stop_bits;
    dcbSerialParams.Parity = parity;
    if (status) {
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastError,
                1024,
                NULL);
            status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Setting serial port state: %s %s\n", line_number++, comm_port, lastError);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> BaudRate: %d, ByteSize: %d, StopBits: %d, Parity: %d\n", line_number++,
                dcbSerialParams.BaudRate, dcbSerialParams.ByteSize, dcbSerialParams.StopBits, dcbSerialParams.Parity);
            MessageBoxA(NULL, "MS-SDR -> ERROR Setting Comm Port Parameters.  Please forward log files to Multus SDR, LLC ",
                "MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);

            CloseHandle(hSerial);
        }
    }
    timeouts.ReadTotalTimeoutMultiplier = com_time_out;
    timeouts.ReadIntervalTimeout = com_time_out;
    timeouts.ReadTotalTimeoutConstant = (com_time_out * 80);

    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 15;
    if (status) {
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lastError,
                1024,
                NULL);
            status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Setting serial port timeouts: %s %s\n", line_number++, comm_port, lastError);
            CloseHandle(hSerial);
            status = FALSE;
        }
    }
    if (status == TRUE) {
        Serial_port_open = TRUE;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] open_comm_port -> Finished\n", line_number++);
    return status;
}

int Start_Serial_Port() {
    FILE* fp_Comm_port_ini;
    char file_name[MAX_PATH] = { 0 };
    int mynumber;
    int status = TRUE;
    int open_port_status = 0;
    long t = 0;
    int lenght = 0;
    int record = 0;
    int index = 0;
    char comm_port_record[132];

    struct {
        char* comm_port_name_start;
        char* comm_port_name_end;
        int comm_port_name_size;
        char* comm_port_index;
        char* baud_rate_index;
        char* parity_index;
        char* data_bits_index;
        char* stop_bits_index;
        char* pin;
    } comm_port_fields;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Start_Serial_Port. STARTED\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/comm-port.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Start_Serial_Port. serial port INI filename: %s\n", line_number++, file_name);
        fp_Comm_port_ini = fopen(file_name, "r");
        if (fp_Comm_port_ini != NULL) {
            fgets(comm_port_record, sizeof(comm_port_record), fp_Comm_port_ini);
            //Get the name of the comm port
            //Find the starting point
            comm_port_fields.comm_port_name_start = strstr(comm_port_record, "COMM_PORT_NAME");
            comm_port_fields.comm_port_name_start = comm_port_fields.comm_port_name_start + 15;
            //Now find the ending point
            comm_port_fields.comm_port_name_end = strstr(comm_port_fields.comm_port_name_start, ",");
            //Get the size of the comm port name.
            comm_port_fields.comm_port_name_size = (comm_port_fields.comm_port_name_end - (comm_port_fields.comm_port_name_start));
            //Now copy to comm_port
            strncpy(G_comm_port, comm_port_fields.comm_port_name_start, comm_port_fields.comm_port_name_size);
            comm_port_fields.comm_port_index = strstr(comm_port_record, "COMM_PORT_INDEX");
            comm_port_fields.baud_rate_index = strstr(comm_port_record, "BAUD_RATE_INDEX");
            comm_port_fields.parity_index = strstr(comm_port_record, "PARITY_INDEX");
            comm_port_fields.data_bits_index = strstr(comm_port_record, "DATA_BITS_INDEX");
            comm_port_fields.stop_bits_index = strstr(comm_port_record, "STOP_BITS_INDEX");
            comm_port_fields.pin = strstr(comm_port_record, "PIN");
            mynumber = atoi((comm_port_fields.comm_port_index + 16));
            comm_name_index = mynumber;
            mynumber = atoi((comm_port_fields.baud_rate_index + 16));
            baud_rate_index = mynumber;
            if (comm_port_fields.parity_index != NULL) {
                mynumber = atoi((comm_port_fields.parity_index + 13));
                parity_index = mynumber;
            }

            if (comm_port_fields.data_bits_index != NULL) {
                mynumber = atoi((comm_port_fields.data_bits_index + 16));
                data_bit_index = mynumber;
            }
            if (comm_port_fields.stop_bits_index != NULL) {
                mynumber = atoi((comm_port_fields.stop_bits_index + 16));
                stop_bit_index = mynumber;
            }
            if (comm_port_fields.pin != NULL) {
                mynumber = atoi((comm_port_fields.pin + 4));
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Pin: %d \n", line_number++, mynumber);
                G_pins = mynumber;
            }
            else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. commport_fields.pin is NULL \n", line_number++);
            }
            baud_rate = baud_rates[baud_rate_index];
            data_bits = data_bit_values[data_bit_index];
            stop_bits = stop_bit_values[stop_bit_index];
            parity = parity_values[parity_index];
            print_time(0);
            fprintf(G_fp_logfile,
                "[%d] Start_Serial_Port. COMM_PORT_NAME=%s,COMM_PORT_INDEX=%d,BAUD_RATE_INDEX=%d,PARITY_INDEX=%d,DATA_BITS_INDEX=%d,STOP_BITS_INDEX=%d,PIN=%d\n",
                line_number++, G_comm_port, comm_name_index, baud_rate_index, parity_index, data_bit_index, stop_bit_index, G_pins);
            fclose(fp_Comm_port_ini);
            if (G_comm_port[3] != '0') {
                open_port_status = open_comm_port();
                if (open_port_status == TRUE) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Starting Comms_port_thread \n", line_number++);
                    p_Comms_port_thread_rc = pthread_create(&p_Comms_port_thread, NULL, Comms_port_thread, (void*)t);
                    G_comms_process_running = 1;
                    if (p_Comms_port_thread_rc) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Start Comms_port_thread thread FAILED, return code from pthread_create() is %d\n",
                            line_number++, p_Comms_port_thread_rc);
                        G_comms_process_running = 0;
                        status = FALSE;
                    }
                    else {
                        Gui_send_param(CMD_GET_SET_COMM_NAME_INDEX, comm_name_index);
                        Gui_send_param(CMD_GET_SET_COMM_BAUD_RATE, baud_rate_index);
                        Gui_send_param(CMD_GET_SET_COMM_PARITY, parity_index);
                        Gui_send_param(CMD_GET_SET_COMM_DATA_BITS, data_bit_index);
                        Gui_send_param(CMD_GET_SET_COMM_STOP_BITS, stop_bit_index);
                        Gui_send_param(CMD_GET_SET_COMM_PORT_PINS, G_pins);
                    }
                    if (G_pins != 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Starting Pin_Check_Thread \n", line_number++);
                        p_Pin_Check_thread_rc = pthread_create(&p_Pin_Check_thread, NULL, Pin_Check_Thread, (void*)t);
                        if (p_Pin_Check_thread_rc) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Start Pin_Check_Thread failed, return code from pthread_create() is %d\n",
                                line_number++, p_Pin_Check_thread_rc);
                            status = FALSE;
                        }
                    }
                }
                else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Open Comm Port Failed \n", line_number++);

                }
            }
        }
        else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Open Initialization file FAILED \n", line_number++);
            MessageBoxA(NULL, "Start_Serial_Port. Open Initialization file FAILED", "MS-SDR",
                MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Start_Serial_Port. FINISHED\n", line_number++);
    return status;
}
/*int Process_comm_command(int opcode, char *buf, int recv_len) {
    int status = 0;
    uint8_t t_opcode_data;
    short int *opcode_data;
    short int s_opcode_data;
    //int *op_code_data_32;
    //int i_opcode_data;
    long t = 0;
    //static int mask;

    opcode = (uint8_t) buf[0];
    t_opcode_data = (uint8_t) (buf[1]);
    opcode_data = (short int*) &buf[1];
    memcpy(&s_opcode_data, opcode_data, 2);

    switch (opcode) {
        case CMD_GET_SET_COMM_PORT_PINS:
            G_pins = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command. CMD_GET_SET_COMM_PORT_PINS. G_pins: %d\n", line_number++, G_pins);
            if (G_previous_G_pins != G_pins) {
                G_previous_G_pins = G_pins;
                if (G_pins != 0) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Process_comm_command. CMD_GET_SET_COMM_PORT_PINS. Staring Pin_Check_Thread \n", 
                                                                                        line_number++);
                    p_Pin_Check_thread_rc = pthread_create(&p_Pin_Check_thread, NULL, Pin_Check_Thread, (void*)t);
                    if (p_Pin_Check_thread_rc) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Process_comm_command. CMD_GET_SET_COMM_PORT_PINS. Start Pin_Check_Thread failed. p_Pin_Check_thread_rc. %d\n", 
                            line_number++, p_Pin_Check_thread_rc);
                        status = FALSE;
                    }
                }
                else {
                    G_Pin_Check_running = 0;
                    status = pthread_kill(p_Pin_Check_thread, 1);
                    if (status == 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_kill p_Pin_Check_thread Successful %d\n", line_number++, t_opcode_data);
                        status = pthread_join(p_Pin_Check_thread, NULL);
                        if (status == 0) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_join p_Pin_Check_thread Successful %d\n", line_number++, t_opcode_data);
                            comm_port_open = 0;
                        }
                    }
                }
            }
            break;

        case CMD_DELETE_COMM_PORT_INIT:
            G_delete_Comm_port_init_file = 1;
            break;

        case CMD_GET_SET_COMM_PORT:
            memset(G_comm_port, 0, sizeof (G_comm_port));
            strcat(G_comm_port, &buf[1]);
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_PORT -> Portname: %s\n", line_number++,
                    G_comm_port);
            break;
        case CMD_GET_SET_COMM_BAUD_RATE:
            baud_rate = baud_rates[t_opcode_data];
            baud_rate_index = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_BAUD_RATE -> Baud Rate Index: %d,Baud Rate: %d\n", line_number++,
                    baud_rate_index, baud_rates[t_opcode_data]);
            break;
        case CMD_GET_SET_COMM_NAME_INDEX:
            comm_name_index = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_NAME_INDEX -> Comm Name index: %d\n", line_number++, comm_name_index);
            break;
        case CMD_GET_SET_COMM_DATA_BITS:
            data_bits = data_bit_values[t_opcode_data];
            data_bit_index = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_DATA_BITS -> Data Bits: %d\n", line_number++, data_bit_values[t_opcode_data]);
            break;
        case CMD_GET_SET_COMM_PARITY:
            parity = parity_values[t_opcode_data];
            parity_index = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_PARITY -> Parity: %d\n", line_number++, parity_values[t_opcode_data]);
            break;
        case CMD_GET_SET_COMM_STOP_BITS:
            stop_bits = stop_bit_values[t_opcode_data];
            stop_bit_index = t_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_STOP_BITS -> Stop Bits: %d\n", line_number++, stop_bit_values[t_opcode_data]);
            break;

        case CMD_GET_SET_COMM_START:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_START -> Param: %d\n", line_number++, t_opcode_data);
            if (t_opcode_data == 1) {
                if (comm_port_open == 0) {
                    status = FALSE;
                    G_comms_process_running = 1;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Process_comm_command -> Calling -> open_comm_port -> COM Port: %s\n", line_number++, G_comm_port);
                    status = open_comm_port();
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Process_comm_command -> open_comm_port status: %d\n", line_number++, status);
                    if (status == TRUE) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Process_comm_command -> Staring Comms_port_thread \n", line_number++);
                        p_Comms_port_thread_rc = pthread_create(&p_Comms_port_thread, NULL, Comms_port_thread, (void *) t);
                        if (p_Comms_port_thread_rc) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_comm_command -> Start Comms_port thread failed, return code from pthread_create() is %d\n", line_number++,
                                    p_Comms_port_thread_rc);
                            status = FALSE;
                        } else {
                            comm_port_open = 1;
                        }
                    }
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Comm Port is Open. Please wait a few moments and try again \n", line_number++);
                    //MessageBoxA(NULL,"Comm Port is Open. Please wait a few moments and try again", "MS-SDR",
                    //	MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
                }
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Process_comm_command -> Stopping Comms_port_thread -> Param: %d\n",
                        line_number++, t_opcode_data);
                if (G_pins != 0) {
                    status = pthread_kill(p_Pin_Check_thread,1);
                    if (status == 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_kill p_Pin_Check_thread Successful %d\n",
                                line_number++, t_opcode_data);
                        status = pthread_join(p_Pin_Check_thread, NULL);
                        if (status == 0) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_join p_Pin_Check_thread Successful %d\n",
                                    line_number++, t_opcode_data);
                        }
                    }
                }
                if (G_comms_process_running != 0) {
                    status = pthread_kill(p_Comms_port_thread, 1);
                    if (status == 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_kill p_Comms_port_thread Successful %d\n",
                                line_number++, t_opcode_data);
                        status = pthread_join(p_Comms_port_thread, NULL);
                        if (status == 0) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_comm_command -> pthread_join p_Comms_port_thread Successful %d\n",
                                    line_number++, t_opcode_data);
                        }
                    }
                }
                G_pins = 0;
                G_comms_process_running = 0;
                comm_port_open = 0;
                //delete_comm_port_ini();
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_START -> Finished\n", line_number++);
            break;
    }
    return status;
}*/



