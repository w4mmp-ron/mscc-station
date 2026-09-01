#include "extern.h"
//#include "commands.h"
//#include "version.h"

#define NORMAL_MODE ('N')

char over_driven_buffer[10];

void Send_Overdriven(int overdriven)
{
        int send_size = 2;

        memset(over_driven_buffer, 0, sizeof(over_driven_buffer));
        over_driven_buffer[0] = CMD_SET_OVERDRIVEN;
        over_driven_buffer[1] = overdriven;
        print_time();
        fprintf(G_fp_logfile, "[%d] Send_Overdriven. Over Driven State: %d\n", line_number++, overdriven);
        if (sendto(ms_sdr_s, (char *) &over_driven_buffer, send_size, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Send_Overdriven. sentto failed with error code : %s\n", line_number++, strerror(errno));
        }
}

void *Overdriven_thread(void *t)
{
        int send_size = 2;
        int overdriven_count = 0;
        int overdriven_sent_flag = 0;
        int normal_flag = 1;

        Sleep(5); //Let the subsystem initialize before processing the overdrive state
        print_time();
        fprintf(G_fp_logfile, "[%d] Overdriven_thread - Thread has started\n", line_number++);
        while (G_all_threads_run) {
                Sleep(10);
                if (mystate.overdriven == 1) {
                        overdriven_count++;
                        if (G_mode != 'T' && G_mode != 'C') { //The DSP Engine sets the overdriven flag in C or T modes (?)
                                if (overdriven_count > 2 && overdriven_sent_flag == 0) {
                                        overdriven_sent_flag = 1;
                                        normal_flag = 0;
                                        print_time();
                                        fprintf(G_fp_logfile, "[%d] Overdriven_thread. Calling Send_Overdriven. overdriven_flag: %d, G_mode: %c\n",
                                                line_number++, overdriven_sent_flag, G_mode);
                                        Send_Overdriven(1);
                                        Sleep(2000);
                                }
                        }
                } else {
                        if (overdriven_sent_flag == 1 && normal_flag == 0) {
                                print_time();
                                fprintf(G_fp_logfile, "[%d] Overdriven_thread. Calling Send_Overdriven. normal_flag: %d\n",
                                        line_number++, normal_flag);
                                Send_Overdriven(0);
                                normal_flag = 1;
                                overdriven_sent_flag = 0;
                                overdriven_count = 0;
                        }
                }
        }
        pthread_exit(0);
        return(NULL);
}