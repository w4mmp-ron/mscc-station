/*
 * USB HID transport for Cypress PSoC bootloader host (Linux).
 * Matches Multus Windows bootloader.exe / USBBootloaderHost (HID, not UART).
 */
#include "hid_comm.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>

#include "cybtldr_utils.h"

static hid_device *g_dev;
static unsigned short g_vid = PROFICIO_BL_VID;
static unsigned short g_pid = PROFICIO_BL_PID;
static char *g_path;

void hid_comm_set_ids(unsigned short vid, unsigned short pid)
{
    g_vid = vid;
    g_pid = pid;
}

void hid_comm_set_path(const char *path)
{
    free(g_path);
    g_path = path ? strdup(path) : NULL;
}

int hid_comm_list(void)
{
    struct hid_device_info *list, *cur;
    int n = 0;

    if (hid_init() != 0) {
        fprintf(stderr, "hid_init failed\n");
        return -1;
    }
    list = hid_enumerate(0, 0);
    for (cur = list; cur; cur = cur->next) {
        printf("%04x:%04x  if=%d  %ls / %ls\n  path=%s\n",
            cur->vendor_id, cur->product_id, cur->interface_number,
            cur->manufacturer_string ? cur->manufacturer_string : L"(none)",
            cur->product_string ? cur->product_string : L"(none)",
            cur->path ? cur->path : "");
        n++;
    }
    hid_free_enumeration(list);
    hid_exit();
    return n;
}

int OpenConnection(void)
{
    if (hid_init() != 0) {
        fprintf(stderr, "hid_init failed\n");
        return CYRET_ERR_COMM_MASK;
    }

    if (g_path && g_path[0]) {
        g_dev = hid_open_path(g_path);
    } else {
        g_dev = hid_open(g_vid, g_pid, NULL);
    }

    if (!g_dev) {
        fprintf(stderr,
            "No HID bootloader device %04x:%04x (is BOOT jumper set / CMD 0x0E sent?)\n"
            "  Tip: sudo or udev rule; try --list\n",
            g_vid, g_pid);
        hid_exit();
        return CYRET_ERR_COMM_MASK;
    }

    hid_set_nonblocking(g_dev, 0);
    return CYRET_SUCCESS;
}

int CloseConnection(void)
{
    if (g_dev) {
        hid_close(g_dev);
        g_dev = NULL;
    }
    hid_exit();
    return CYRET_SUCCESS;
}

int WriteData(uint8_t *wrData, int byteCnt)
{
    uint8_t buf[1 + HID_REPORT_SIZE];
    int n, to_copy;

    if (!g_dev || byteCnt < 0)
        return CYRET_ERR_COMM_MASK;

    memset(buf, 0, sizeof(buf));
    buf[0] = 0x00; /* report ID */
    to_copy = byteCnt;
    if (to_copy > HID_REPORT_SIZE)
        to_copy = HID_REPORT_SIZE;
    if (to_copy > 0 && wrData)
        memcpy(buf + 1, wrData, (size_t)to_copy);

    n = hid_write(g_dev, buf, sizeof(buf));
    if (n < 0) {
        fprintf(stderr, "hid_write failed: %ls\n", hid_error(g_dev));
        return CYRET_ERR_COMM_MASK;
    }
    return CYRET_SUCCESS;
}

int ReadData(uint8_t *rdData, int byteCnt)
{
    uint8_t buf[1 + HID_REPORT_SIZE];
    int n, to_copy;

    if (!g_dev || !rdData || byteCnt <= 0)
        return CYRET_ERR_COMM_MASK;

    memset(buf, 0, sizeof(buf));
    /* 5 s timeout — flash rows can be slow */
    n = hid_read_timeout(g_dev, buf, sizeof(buf), 5000);
    if (n <= 0) {
        fprintf(stderr, "hid_read failed/timeout: %ls\n",
            g_dev ? hid_error(g_dev) : L"(null)");
        return CYRET_ERR_COMM_MASK;
    }

    /*
     * Some stacks return report-ID as first byte; Cypress utils expect payload only.
     * If first byte is 0 and n == 65, skip ID. If n == 64, use as-is.
     */
    if (n == (int)sizeof(buf) && buf[0] == 0) {
        to_copy = byteCnt < HID_REPORT_SIZE ? byteCnt : HID_REPORT_SIZE;
        memcpy(rdData, buf + 1, (size_t)to_copy);
    } else {
        to_copy = byteCnt < n ? byteCnt : n;
        memcpy(rdData, buf, (size_t)to_copy);
    }
    if (to_copy < byteCnt)
        memset(rdData + to_copy, 0, (size_t)(byteCnt - to_copy));
    return CYRET_SUCCESS;
}

static CyBtldr_CommunicationsData g_comm = {
    OpenConnection,
    CloseConnection,
    ReadData,
    WriteData,
    HID_REPORT_SIZE
};

CyBtldr_CommunicationsData *hid_comm_get(void)
{
    return &g_comm;
}
