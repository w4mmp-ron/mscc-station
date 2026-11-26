/* cw-carrier (recv 3.190): see cw_carrier_rx.c */
#ifndef CW_CARRIER_RX_H
#define CW_CARRIER_RX_H

#include <stdint.h>
#include <pthread.h>

extern pthread_mutex_t g_stream_lock;   /* held around every receive stream start / stop */

void cw_carrier_rx_set(uint8_t value);
void *CW_Carrier_RX_thread(void *arg);

#endif
