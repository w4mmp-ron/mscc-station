/* cw-carrier (trans 3.190): see cw_carrier.c */
#ifndef CW_CARRIER_H
#define CW_CARRIER_H

#include <stdint.h>
#include <pthread.h>

extern pthread_mutex_t g_stream_lock;   /* held around every I/Q stream start / stop */

int  cw_carrier_enabled(void);
int  trans_null_flush_done(void);   /* main.c */
void cw_carrier_capture(const float *out, unsigned long frames);
void cw_carrier_command_seen(uint8_t opcode);
void cw_carrier_set(uint8_t value);
void *CW_Carrier_thread(void *arg);

#endif
