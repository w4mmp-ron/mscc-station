#ifndef HID_COMM_H
#define HID_COMM_H

#include <stdint.h>
#include "cybtldr_api.h"

/* Proficio / Omnia PSoC USB HID bootloader (from bootloader USBFS_descr.c) */
#define PROFICIO_BL_VID 0x04B4
#define PROFICIO_BL_PID 0xB71D
#define HID_REPORT_SIZE 64

void hid_comm_set_ids(unsigned short vid, unsigned short pid);
void hid_comm_set_path(const char *path); /* optional hidapi path; overrides vid/pid open */

int hid_comm_list(void);

/* CyBtldr_CommunicationsData callbacks */
int OpenConnection(void);
int CloseConnection(void);
int ReadData(uint8_t *rdData, int byteCnt);
int WriteData(uint8_t *wrData, int byteCnt);

CyBtldr_CommunicationsData *hid_comm_get(void);

#endif
