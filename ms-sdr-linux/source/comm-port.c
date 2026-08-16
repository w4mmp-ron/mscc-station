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

HANDLE hSerial = INVALID_HANDLE_VALUE;
DWORD com_time_out = 20;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };

int comm_name_index = 0;
int baud_rate_index = 0;
int parity_index = 0;
int stop_bit_index = 0;
int data_bit_index = 0;
/* defined in main.c */
extern const char *homedir;

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
    DWORD lpModemStat;
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
        if (hSerial != INVALID_HANDLE_VALUE) {
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
        if (hSerial != INVALID_HANDLE_VALUE) {
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

        // Kenwood RM (this radio manual):
        //   P1 meter: 0=unselected, 1=SWR, 2=COMP, 3=ALC
        //   P2 value: 0000-0030 dots (needle scale), not S-units / not SWR*10
        // Reply: RMpxxxx;
        write_buffer[0] = 'R';
        write_buffer[1] = 'M';

        if (record_start[2] == ';') {
            write_buffer[2] = '1';                    // Default: SWR (WSJT-X)
        }
        else {
            write_buffer[2] = record_start[2];
            // Clamp unknown types into 0-3
            if (write_buffer[2] < '0' || write_buffer[2] > '3') {
                write_buffer[2] = '1';
            }
        }

        {
            int meter_value = 0;                      // dots 0..30
            char meter_type = write_buffer[2];

            switch (meter_type) {
                case '1':
                    // SWR: fixed good match ~1.1 on a 0-30 dot scale
                    // (low dots = good; 0 = flat, 30 = full scale)
                    meter_value = 2;
                    break;
                case '2':                              // COMP — not instrumented
                case '3':                              // ALC — not instrumented
                case '0':                              // unselected
                default:
                    meter_value = 0;
                    break;
            }
            if (meter_value < 0) meter_value = 0;
            if (meter_value > 30) meter_value = 30;

            UintToStr(&write_buffer[3], (uint32_t)meter_value, 4);
        }
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

    /*
     * Kenwood mode SET: MDn;  (n = 1 LSB, 2 USB, 3 CW — matches MD; reply mapping above)
     * WSJT-X and other digi apps send this; was previously commented out so MD2; was discarded.
     * Query MD; is handled above (readback only).
     */
    {
        int mode_number = 0;
        int md_set = 0;

        if (strstr(receive_buffer, "MD1;") != NULL) {
            mode_number = 1; /* LSB */
            md_set = 1;
        } else if (strstr(receive_buffer, "MD2;") != NULL) {
            mode_number = 2; /* USB */
            md_set = 1;
        } else if (strstr(receive_buffer, "MD3;") != NULL) {
            mode_number = 3; /* CW */
            md_set = 1;
        }
        /* Optional common Kenwood/data modes if an app sends them */
        else if (strstr(receive_buffer, "MD4;") != NULL) {
            mode_number = 4;
            md_set = 1;
        } else if (strstr(receive_buffer, "MD5;") != NULL) {
            mode_number = 5; /* often AM in our reply map */
            md_set = 1;
        }

        if (md_set) {
            command = MD;
            G_mode = mode_to_letter(mode_number);
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, mode_number);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, mode_number);
            ModeChanged(G_mode);
            Gui_send_param(CMD_MODE_SET_BY_SERVER, G_mode);
            print_time(0);
            fprintf(G_fp_logfile,
                "[%d] parse_record. MD set received MD%d; -> G_mode=%c cores mode=%d\n",
                line_number++, mode_number, G_mode, mode_number);
        }
    }

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
    hSerial = INVALID_HANDLE_VALUE;
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
    char windows_comm_port[MAX_PATH] = { 0 };
    char error_message[80] = { 0 };
    (void)read_timeout;
    (void)error_message;

    if (G_comm_port[0] == '\0') {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] open_comm_port. G_comm_port empty — refusing open\n", line_number++);
        return FALSE;
    }

#if defined(__linux__) || defined(__APPLE__)
    /* Linux: PTY / absolute /dev path / COM name (CreateFileA maps them) */
    snprintf(windows_comm_port, sizeof(windows_comm_port), "%s", G_comm_port);
#else
    sprintf(windows_comm_port, "\\\\.\\%s", G_comm_port);
#endif
    print_time(0);
    fprintf(G_fp_logfile, "[%d] open_comm_port. Opening: %s (G_comm_port=%s)\n",
        line_number++, windows_comm_port, G_comm_port);

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
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastError,
            1024,
            NULL);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] open_comm_port. Error Opening Com Port: %s %s\n", line_number++, windows_comm_port, lastError);
#if !defined(__linux__) && !defined(__APPLE__)
        MessageBoxA(NULL, "SERIAL PORT OPEN FAILED. \r\n", "MS-SDR",
            MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
#endif
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
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Retrieving serial port state: %s %s\n",
                line_number++, G_comm_port, lastError);
#if defined(__linux__) || defined(__APPLE__)
            /* PTY / some USB serial: continue and apply our DCB defaults */
            memset(&dcbSerialParams, 0, sizeof(dcbSerialParams));
            dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
#else
            status = FALSE;
            MessageBoxA(NULL, "MS-SDR -> ERROR Retrieving Comm Port State.  Please forward log files to Multus SDR, LLC ",
                "MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
#endif
        }
    }
    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = (unsigned char)data_bits;
    dcbSerialParams.StopBits = (unsigned char)stop_bits;
    dcbSerialParams.Parity = (unsigned char)parity;
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
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Setting serial port state: %s %s\n",
                line_number++, G_comm_port, lastError);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> BaudRate: %lu, ByteSize: %u, StopBits: %u, Parity: %u\n",
                line_number++,
                (unsigned long)dcbSerialParams.BaudRate,
                (unsigned)dcbSerialParams.ByteSize,
                (unsigned)dcbSerialParams.StopBits,
                (unsigned)dcbSerialParams.Parity);
#if defined(__linux__) || defined(__APPLE__)
            /* Kenwood CAT over PTY still works raw; keep port open */
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> continuing without termios (PTY ok)\n", line_number++);
#else
            status = FALSE;
            MessageBoxA(NULL, "MS-SDR -> ERROR Setting Comm Port Parameters.  Please forward log files to Multus SDR, LLC ",
                "MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
#endif
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
            print_time(0);
            fprintf(G_fp_logfile, "[%d] open_comm_port -> Error Setting serial port timeouts: %s %s\n",
                line_number++, G_comm_port, lastError);
#if !defined(__linux__) && !defined(__APPLE__)
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            status = FALSE;
#endif
        }
    }
    if (status == TRUE) {
        Serial_port_open = TRUE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] open_comm_port -> OK port=%s baud=%d\n",
            line_number++, G_comm_port, baud_rate);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] open_comm_port -> Finished status=%d\n", line_number++, status);
    return status;
}

/* Open configured port and start Kenwood CAT reader thread. */
static int launch_comms_port_thread(void)
{
    long t = 0;
    int open_port_status;

#if defined(__linux__) || defined(__APPLE__)
    open_port_status = open_comm_port();
#else
    /* Windows: COM0 means "no serial adapter" — skip open */
    if (G_comm_port[0] != '\0' &&
        !(G_comm_port[0] == 'C' && G_comm_port[1] == 'O' && G_comm_port[2] == 'M' && G_comm_port[3] == '0' &&
          (G_comm_port[4] == '\0' || G_comm_port[4] == '\r' || G_comm_port[4] == '\n')))
        open_port_status = open_comm_port();
    else
        open_port_status = FALSE;
#endif

    if (open_port_status != TRUE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] launch_comms_port_thread. Open Comm Port Failed / skipped (%s)\n",
            line_number++, G_comm_port);
        return FALSE;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] launch_comms_port_thread. Starting Comms_port_thread\n", line_number++);
    G_comms_process_running = 1;
    p_Comms_port_thread_rc = pthread_create(&p_Comms_port_thread, NULL, Comms_port_thread, (void*)t);
    if (p_Comms_port_thread_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] launch_comms_port_thread. pthread_create FAILED rc=%d\n",
            line_number++, p_Comms_port_thread_rc);
        G_comms_process_running = 0;
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
        Serial_port_open = 0;
        return FALSE;
    }

    Gui_send_param(CMD_GET_SET_COMM_NAME_INDEX, comm_name_index);
    Gui_send_param(CMD_GET_SET_COMM_BAUD_RATE, baud_rate_index);
    Gui_send_param(CMD_GET_SET_COMM_PARITY, parity_index);
    Gui_send_param(CMD_GET_SET_COMM_DATA_BITS, data_bit_index);
    Gui_send_param(CMD_GET_SET_COMM_STOP_BITS, stop_bit_index);
    Gui_send_param(CMD_GET_SET_COMM_PORT_PINS, G_pins);

    if (G_pins != 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] launch_comms_port_thread. Starting Pin_Check_Thread\n", line_number++);
        p_Pin_Check_thread_rc = pthread_create(&p_Pin_Check_thread, NULL, Pin_Check_Thread, (void*)t);
        if (p_Pin_Check_thread_rc) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] launch_comms_port_thread. Pin_Check_Thread failed rc=%d\n",
                line_number++, p_Pin_Check_thread_rc);
        }
    }
    return TRUE;
}

static void apply_default_serial_settings(void)
{
    /* BAUD_RATE_INDEX=3 → 9600, DATA_BITS_INDEX=1 → 8, rest none/1 stop */
    comm_name_index = 0;
    baud_rate_index = 3;
    parity_index = 0;
    data_bit_index = 1;
    stop_bit_index = 0;
    G_pins = 0;
    baud_rate = baud_rates[baud_rate_index];
    data_bits = data_bit_values[data_bit_index];
    stop_bits = stop_bit_values[stop_bit_index];
    parity = parity_values[parity_index];
}

int Start_Serial_Port() {
    FILE* fp_Comm_port_ini;
    char file_name[MAX_PATH] = { 0 };
    int mynumber;
    int status = TRUE;
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
            if (fgets(comm_port_record, sizeof(comm_port_record), fp_Comm_port_ini) == NULL) {
                fclose(fp_Comm_port_ini);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Empty comm-port.ini\n", line_number++);
                goto create_default_ini;
            }
            /* Get the name of the comm port */
            comm_port_fields.comm_port_name_start = strstr(comm_port_record, "COMM_PORT_NAME");
            if (comm_port_fields.comm_port_name_start == NULL) {
                fclose(fp_Comm_port_ini);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Malformed ini (no COMM_PORT_NAME)\n", line_number++);
                goto create_default_ini;
            }
            comm_port_fields.comm_port_name_start = comm_port_fields.comm_port_name_start + 15;
            comm_port_fields.comm_port_name_end = strstr(comm_port_fields.comm_port_name_start, ",");
            if (comm_port_fields.comm_port_name_end == NULL) {
                fclose(fp_Comm_port_ini);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Malformed ini (no comma after port name)\n", line_number++);
                goto create_default_ini;
            }
            comm_port_fields.comm_port_name_size =
                (int)(comm_port_fields.comm_port_name_end - comm_port_fields.comm_port_name_start);
            if (comm_port_fields.comm_port_name_size <= 0 ||
                comm_port_fields.comm_port_name_size >= (int)sizeof(G_comm_port)) {
                fclose(fp_Comm_port_ini);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Bad port name length %d\n",
                    line_number++, comm_port_fields.comm_port_name_size);
                goto create_default_ini;
            }
            memset(G_comm_port, 0, sizeof(G_comm_port));
            strncpy(G_comm_port, comm_port_fields.comm_port_name_start,
                (size_t)comm_port_fields.comm_port_name_size);
            G_comm_port[comm_port_fields.comm_port_name_size] = '\0';

            comm_port_fields.comm_port_index = strstr(comm_port_record, "COMM_PORT_INDEX");
            comm_port_fields.baud_rate_index = strstr(comm_port_record, "BAUD_RATE_INDEX");
            comm_port_fields.parity_index = strstr(comm_port_record, "PARITY_INDEX");
            comm_port_fields.data_bits_index = strstr(comm_port_record, "DATA_BITS_INDEX");
            comm_port_fields.stop_bits_index = strstr(comm_port_record, "STOP_BITS_INDEX");
            comm_port_fields.pin = strstr(comm_port_record, "PIN");

            if (comm_port_fields.comm_port_index != NULL) {
                mynumber = atoi(comm_port_fields.comm_port_index + 16);
                comm_name_index = mynumber;
            }
            if (comm_port_fields.baud_rate_index != NULL) {
                mynumber = atoi(comm_port_fields.baud_rate_index + 16);
                if (mynumber >= 0 && mynumber < 8)
                    baud_rate_index = mynumber;
            }
            if (comm_port_fields.parity_index != NULL) {
                mynumber = atoi(comm_port_fields.parity_index + 13);
                if (mynumber >= 0 && mynumber < 3)
                    parity_index = mynumber;
            }
            if (comm_port_fields.data_bits_index != NULL) {
                mynumber = atoi(comm_port_fields.data_bits_index + 16);
                if (mynumber >= 0 && mynumber < 3)
                    data_bit_index = mynumber;
            }
            if (comm_port_fields.stop_bits_index != NULL) {
                mynumber = atoi(comm_port_fields.stop_bits_index + 16);
                if (mynumber >= 0 && mynumber < 2)
                    stop_bit_index = mynumber;
            }
            if (comm_port_fields.pin != NULL) {
                mynumber = atoi(comm_port_fields.pin + 4);
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

            if (!launch_comms_port_thread())
                status = TRUE; /* non-fatal: app continues without CAT */
        }
        else {
create_default_ini:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Start_Serial_Port. No/invalid ini — writing default and opening\n",
                line_number++);
            fp_Comm_port_ini = fopen(file_name, "w");
            if (fp_Comm_port_ini != NULL) {
#if defined(__linux__) || defined(__APPLE__)
                /* PTY Kenwood CAT — digital apps open $HOME/ms-sdr-cat */
                fprintf(fp_Comm_port_ini,
                    "COMM_PORT_NAME=PTY,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN=0;\n");
                strcpy(G_comm_port, "PTY");
#else
                fprintf(fp_Comm_port_ini,
                    "COMM_PORT_NAME=COM0,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN=0;\n");
                strcpy(G_comm_port, "COM0");
#endif
                fclose(fp_Comm_port_ini);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Created %s port=%s\n",
                    line_number++, file_name, G_comm_port);
                apply_default_serial_settings();
                /* Linux: open PTY now (first run). Windows COM0: launch skips open. */
                (void)launch_comms_port_thread();
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Start_Serial_Port. Could not create default comm-port.ini (continuing without serial)\n",
                    line_number++);
#if !defined(__linux__) && !defined(__APPLE__)
                MessageBoxA(NULL, "Start_Serial_Port. Open Initialization file FAILED", "MS-SDR",
                    MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
#endif
            }
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Start_Serial_Port. HOME not set — CAT disabled\n", line_number++);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Start_Serial_Port. FINISHED open=%d port=%s\n",
        line_number++, Serial_port_open, G_comm_port);
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



