#include "extern.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__) || defined(__APPLE__)
#include <libusb-1.0/libusb.h>
#define _tcsncmp strncmp
#else
#include <conio.h>		// gives _cprintf()
#include <ShlObj.h>
#include <KnownFolders.h>
#include <tchar.h>   // Ensure this is included (for _tcscmp, _tcsncmp, etc.)
#include "lusb0_usb.h"	// LIBUSB-Win32
#endif
//#include <strings.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "usbavrcmd.h"
#include "version.h"

#if defined(MS_SDR_NO_USB)
/* Minimal stubs so UDP / client handshake can run without libusb or hardware. */
int usbControlMsgIN(int cmd, int value, int index, char* buffer, int buflen) {
    (void)cmd; (void)value; (void)index; (void)buffer; (void)buflen;
    return -1;
}
int usbControlMsgOUT(int cmd, int value, int index, char* buffer, int buflen) {
    (void)cmd; (void)value; (void)index; (void)buffer; (void)buflen;
    return buflen > 0 ? buflen : 0;
}
void* srOpen(int vid, int pid, const TCHAR* pManufacturer, const TCHAR* pProduct,
             const TCHAR* pSerialNumber, int iDevNum, int *status) {
    (void)vid; (void)pid; (void)pManufacturer; (void)pProduct; (void)pSerialNumber; (void)iDevNum;
    if (status) *status = 1;
    return (void*)1;
}
BOOL srGetVersion(int* major, int* minor) {
    if (major) *major = 124;
    if (minor) *minor = 92;
    G_firmware_version_packed = ((92 << 8) & 0xff00) | (124 & 0x00ff);
    return TRUE;
}
BOOL srSetPTTGetCWInp(int ptt, int* pCWkey) {
    (void)ptt;
    if (pCWkey) *pCWkey = 0;
    return TRUE;
}
BOOL srSetFreq(double MHz, int i2cAddr) { (void)MHz; (void)i2cAddr; return TRUE; }
BOOL srGetFreq(double* pMHz) { if (pMHz) *pMHz = 14.0; return TRUE; }
void srClose(void) {}
#else
/* Real USB implementation below */

void* srUsbHandle = NULL;
int srUsbTimeout = 500;
static srUsbInfo_t srUsbInfo;

#if defined(__linux__) || defined(__APPLE__)
/* ---------- libusb-1.0 backend (Debian/WSL) ---------- */
static libusb_context *g_lusb_ctx = NULL;
#define usbHandle  ((libusb_device_handle*)srUsbHandle)

int usbControlMsgIN(int cmd, int value, int index, char* buffer, int buflen) {
    int r = -1;
    G_Transceiver_Busy = TRUE;
    if (usbHandle != NULL) {
        r = libusb_control_transfer(usbHandle,
                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                (uint8_t)cmd, (uint16_t)value, (uint16_t)index,
                (unsigned char*)buffer, (uint16_t)buflen, (unsigned)srUsbTimeout);
    }
    G_Transceiver_Busy = FALSE;
    return r;
}

int usbControlMsgOUT(int cmd, int value, int index, char* buffer, int buflen) {
    int r = -1;
    if (usbHandle != NULL) {
        r = libusb_control_transfer(usbHandle,
                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                (uint8_t)cmd, (uint16_t)value, (uint16_t)index,
                (unsigned char*)buffer, (uint16_t)buflen, (unsigned)srUsbTimeout);
    }
    return r;
}

static int usbGetStringAscii_linux(libusb_device_handle *dev, int index, TCHAR *buf, int buflen) {
    unsigned char buffer[256];
    int rval, i;
    if (dev == NULL || buf == NULL || buflen < 1)
        return -1;
    rval = libusb_get_string_descriptor(dev, (uint8_t)index, 0x0409, buffer, sizeof(buffer));
    if (rval < 2)
        return rval;
    if (buffer[1] != LIBUSB_DT_STRING)
        return 0;
    if (buffer[0] < rval)
        rval = buffer[0];
    rval /= 2;
    for (i = 1; i < rval && (i - 1) < buflen - 1; i++) {
        buf[i - 1] = (TCHAR)buffer[2 * i];
        if (buffer[2 * i + 1] != 0)
            buf[i - 1] = '?';
    }
    buf[i - 1] = 0;
    return i - 1;
}

void srClose(void) {
    if (usbHandle != NULL) {
        libusb_release_interface(usbHandle, 0);
        libusb_close(usbHandle);
        srUsbHandle = NULL;
    }
    if (g_lusb_ctx != NULL) {
        libusb_exit(g_lusb_ctx);
        g_lusb_ctx = NULL;
    }
}

void* srOpen(int vid, int pid, const TCHAR* pManufacturer, const TCHAR* pProduct,
             const TCHAR* pSerialNumber, int iDevNum, int *status) {
    int r;
    libusb_device_handle *h = NULL;
    (void)pManufacturer; (void)pProduct; (void)pSerialNumber; (void)iDevNum;

    if (status) *status = 0;
    if (g_lusb_ctx == NULL) {
        r = libusb_init(&g_lusb_ctx);
        if (r != 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] srOpen . libusb_init FAILED: %d\n", line_number++, r);
            return NULL;
        }
    }
    h = libusb_open_device_with_vid_pid(g_lusb_ctx, (uint16_t)vid, (uint16_t)pid);
    if (h == NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] srOpen . device %04x:%04x not found (attach via usbipd?)\n",
            line_number++, vid, pid);
        return NULL;
    }
    r = libusb_claim_interface(h, 0);
    if (r != 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] srOpen . claim interface FAILED: %d (try sudo or udev rule)\n",
            line_number++, r);
        libusb_close(h);
        return NULL;
    }
    srUsbHandle = h;
    srUsbInfo.VID = vid;
    srUsbInfo.PID = pid;
    {
        int n = usbGetStringAscii_linux(h, 3, G_serial_number, (int)sizeof(G_serial_number));
        if (n > 0) {
            strncpy(G_Usb_serial_number, G_serial_number, sizeof(G_Usb_serial_number) - 1);
            if (status) *status = n;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] srOpen . OK serial=%s\n", line_number++, G_Usb_serial_number);
            return h;
        }
    }
    /* Open OK even without serial string */
    if (status) *status = 1;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] srOpen . OK (no serial string)\n", line_number++);
    return h;
}

BOOL srIsOpen(void) {
    return srUsbHandle != NULL;
}

srUsbInfo_t* srGetUsbInfo(void) {
    return srUsbHandle != NULL ? &srUsbInfo : NULL;
}

int srGetString(int ID, TCHAR* pStr, int iLen) {
    if (usbHandle == NULL)
        return 0;
    return usbGetStringAscii_linux(usbHandle, ID, pStr, iLen);
}

#else
/* ---------- libusb-win32 (0.1 API) backend ---------- */
#define usbHandle  ((usb_dev_handle*)srUsbHandle)

int usbControlMsgIN(int cmd, int value, int index, char* buffer, int buflen) {
    int r = -1;
    G_Transceiver_Busy = TRUE;
    if (usbHandle != NULL) {
        r = usb_control_msg(usbHandle,
                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                cmd,
                value, index, buffer, buflen, srUsbTimeout);
    }
    G_Transceiver_Busy = FALSE;
    return r;
}

int usbControlMsgOUT(int cmd, int value, int index, char* buffer, int buflen) {
    int r = -1;
    if (usbHandle != NULL) {
        r = usb_control_msg(usbHandle,
                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                cmd,
                value, index, buffer, buflen, srUsbTimeout);
    }
    return r;
}


static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, TCHAR *buf, int buflen) {
    char buffer[256];
    int rval, i;

    if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof (buffer), 1000)) < 0)
        return rval;

    if (buffer[1] != USB_DT_STRING)
        return 0;

    if ((unsigned char) buffer[0] < rval)
        rval = (unsigned char) buffer[0];

    rval /= 2;

    //lossy conversion to ISO Latin1 
    for (i = 1; i < rval; i++) {
        if (i > buflen) // destination buffer overflow 
            break;
        buf[i - 1] = buffer[2 * i];
        if (buffer[2 * i + 1] != 0) // outside of ISO Latin1 range 
            buf[i - 1] = '?';
    }

    buf[i - 1] = 0;

    return rval;
}

static BOOL testUsbString(int ID, TCHAR* pStr, int iLen, const TCHAR* pTest) {
    *pStr = '\0';

    if (ID > 0) {
        usbGetStringAscii(usbHandle, ID, 0x0409, pStr, iLen);
    }

    if (pTest != NULL) {
        if (_tcsncmp(pTest, pStr, iLen) != 0) {
            return FALSE;
        }
    }

    return TRUE;
}

int srGetString(int ID, TCHAR* pStr, int iLen) {
    if (usbHandle == NULL)
        return 0;

    return usbGetStringAscii(usbHandle, ID, 0x0409, pStr, iLen);
}

BOOL srIsOpen() {
    return srUsbHandle != NULL;
}

srUsbInfo_t* srGetUsbInfo() {
    return srUsbHandle != NULL ? &srUsbInfo : NULL;
}

void srClose() {
    if (usbHandle != NULL) {
        usb_release_interface(usbHandle, 0);
        usb_close(usbHandle);
    }
    srUsbHandle = NULL;
}

#endif /* !linux — end Win32 open/string helpers; shared commands below */

//Command 0x00

BOOL srGetVersion(int* major, int* minor) {
    int r;
    uint16_t iVersion = 0;
    uint8_t bVersion = 0;

    // Get version of my pe0fko firmware
    print_time(0);
    fprintf(G_fp_logfile, "[%d] srGetVersion. \n",line_number++);
    r = usbControlMsgIN(CMD_GET_VERSION, 0xA55A, 0, (char*) &iVersion, sizeof (iVersion));
    if (r == 2) {
        if (iVersion != 0xA55A) {
            /*
             * Target client/Proficio layout (FIRMWARE_VERSION / mscc-client 0xB2):
             *   packed = (minor << 8) | major   e.g. 3.150 → 0x9603
             *
             * Some paths still present pe0fko order on the wire (major high, minor low),
             * e.g. 0x0397 for 3.151. Detect: model/major is small (≤15), build/minor often large.
             */
            {
                unsigned lo = iVersion & 0xFF;
                unsigned hi = (iVersion >> 8) & 0xFF;
                if (lo > 15 && hi <= 15) {
                    /* Wire looks like pe0fko: high=major, low=minor */
                    *major = (int)hi;
                    *minor = (int)lo;
                } else {
                    /* Wire matches Proficio #define: low=major, high=minor */
                    *major = (int)lo;
                    *minor = (int)hi;
                }
                G_firmware_version_packed =
                    ((*minor << 8) & 0xff00) | (*major & 0x00ff);
            }
        } else {
            // Echo the number, must be old SAQ firmware?
            *major = 1; //vDG8SAQ
            *minor = 4;
            G_firmware_version_packed = ((*minor << 8) & 0xff00) | (*major & 0x00ff);
        }
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] srGetVersion. usb_raw=0x%04X -> major: %d, minor: %d, client_pack=0x%04X\n",
            line_number++, (unsigned)iVersion, *major, *minor,
            (unsigned)G_firmware_version_packed);
        return TRUE;
    }

    // Get version of the new V2.0 SDR-Kit firmware
    r = usbControlMsgIN(0xFF, 0, 0, (char*) &bVersion, sizeof (bVersion));
    if (r == 1 && bVersion != 0xFF) {
        *major = (bVersion >> 4) & 0x0F;
        *minor = bVersion & 0x0F;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] srGetVersion - 0xA55A. major: %d, minor: %d\n", line_number++, *major, *minor);
        return TRUE;
    }

    return FALSE;
}

/*Command 0x3A:
-------------
Return actual frequency of the device.
The frequency is formatted in MHz as a 11.21 bits value.

Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x3A
    value:           Don't care
    index:           Don't care
    bytes:           pointer 32 bits integer
    size:            4

Code sample:
    uint32_t iFreq;
    double   dFreq;
    r = usbCtrlMsgIN(0x3A, 0, 0, (char *)&iFreq, sizeof(iFreq));
    if (r == 4)
        dFreq = (double)iFreq / (1UL<<21);
 */
BOOL srGetFreq(double* pMHz) {
    int r;
    uint32_t iFreq = 0;

    r = usbControlMsgIN(CMD_GET_FREQ, 0, 0, (char*) &iFreq, sizeof (iFreq));
    print_time(0);
    fprintf(G_fp_logfile, "[%d] srGetFreq . Called. iFreq: %ld, r: %d\n", line_number++, iFreq, r);

    /*if (r == sizeof(iFreq))
    {
            if (pMHz != NULL) {
     *pMHz = (double)iFreq / (1UL << 21);
     *pMHz = floor(*pMHz * 1e6 + 0.5) / 1e6;			// Round to 1Hz
            }
    }*/
    *pMHz = iFreq;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] srGetFreq . Finished\n", line_number++);
    return r;
}

/*
Command 0x3C:
-------------
Return device startup frequency.
The frequency is formatted in MHz as a 11.21 bits value.

Default:    4 * 7.050 MHz

Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x3C
    value:           Don't care
    index:           Don't care
    bytes:           pointer 32 bits integer
    size:            4
 */
BOOL srGetStartupFreq(double* pMHz) {
    int r;
    uint32_t iFreq = 0;

    r = usbControlMsgIN(CMD_GET_STARTUP, 0, 0, (char*) (&iFreq), sizeof (iFreq));

    if (r == sizeof (iFreq)) {
        if (pMHz != NULL) {
            *pMHz = (double) iFreq / (1UL << 21);
            *pMHz = floor(*pMHz * 1e6 + 0.5) / 1e6; // Round to 1Hz
        }
    }

    return r == sizeof (iFreq);
}

/*Command 0x3D:
-------------
Return device crystal frequency.
The frequency is formatted in MHz as a 8.24 bits value.

Default:    114.285 MHz

Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x3D
    value:           Don't care
    index:           Don't care
    bytes:           pointer 32 bits integer
    size:            4
 */
BOOL srGetXtalFreq(double* pMHz) {
    int r;
    uint32_t ixtal = 0;

    // Try the new firmware command
    r = usbControlMsgIN(CMD_GET_XTAL, 0, 0, (char*) &ixtal, sizeof (ixtal));
    if (r == sizeof (ixtal)) {
        if (pMHz != NULL) {
            *pMHz = (double) ixtal / (1UL << 24);
            *pMHz = floor(*pMHz * 0.125e6 + 0.5) / 0.125e6; // Round to 0.125Hz
        }
        return TRUE;
    }

    // Old V14 Firmware DG8SAQ!
    for (int i = 0; i < 4; ++i) {
        r = usbControlMsgIN(CMD_READ_EEPROM, 2 + i, 0, &((char*) &ixtal)[i], 1);
        if (r != 1) {
            return FALSE;
        }
    }

    if (pMHz != NULL) {
        *pMHz = (double) ixtal / (1UL << 24);
        *pMHz = floor(*pMHz * 0.125e6 + 0.5) / 0.125e6; // Round to 0.125Hz
    }

    return TRUE;
}

/*
Command 0x3F:
-------------
Return the Si570 frequency control registers (reg 7 .. 12). If there are I2C errors
the return length is 0.

Default:    None

Parameters:
    requesttype:     USB_ENDPOINT_IN
    request:         0x3F
    value:           Don't care
    index:           Don't care
    bytes:           pointer 6 byte register array
    size:            6
 */
// i2cAddr only needed with SAQ version firmware.

BOOL srGetFreqReg(unsigned char reg[6], int i2cAddr, int index) {
    int r;

    r = usbControlMsgIN(CMD_GET_SI570, 0x0700 + i2cAddr, index, (char*) &reg[0], 6); //sizeof(reg));

    return r == 6; //sizeof(reg);
}

/*
Command 0x50:
-------------
Set PTT (PB4) I/O line and read CW key level from the PB5 (CW Key_1) and PB1 (CW Key_2).
In case of the enabled ABPF no change of PTT I/O line will be done and no read of the CW key's are
done. The command will return (in case of enabled ABPF) for both CW key's a open status (bits are 1).
The returnd bit value is bit 5 (0x20) for CW key_1 and bit 1 (0x02) for CW key_2, the other bits are zero.


Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x50
    value:           Output BOOL to user output PTT
    index:           0
    bytes:           pointer to 1 byte variable CW Key's
    size:            1
 */
BOOL srSetPTTGetCWInp(int ptt, int* pCWkey) {
    int r;
    uint8_t key = 0;

    r = usbControlMsgIN(CMD_SET_PTT, ptt, 0, (char*) &key, sizeof (key));
    if (r < 0)
        return FALSE;

    if (r == sizeof (key) && pCWkey != NULL)
        *pCWkey = key;

    return TRUE;
}

/*

Command 0x51:
-------------
Read CW key level from the PB5 (CW Key_1) and PB1 (CW Key_2).
In case of the enabled ABPF no read of the CW key's are done. The command will return for both CW key's 
a open status (bits are 1).
The returnd bit value is bit 5 (0x20) for CW key_1 and bit 1 (0x02) for CW key_2, the other bits are zero.


Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x51
    value:           0
    index:           0
    bytes:           pointer to 1 byte variable CW Key's
    size:            1
 */
BOOL srGetCWInp(int* pCWkey) {
    int r;
    uint8_t key = 0;

    r = usbControlMsgIN(CMD_GET_CW_KEY, 0, 0, (char*) &key, sizeof (key));

    if (r == sizeof (key)) {
        if (pCWkey != NULL)
            *pCWkey = key;
    }

    return r == sizeof (key);
}

/*
Command 0x20:
-------------
Write one byte to a Si570 register. Return value is the i2c error BOOLean in the buffer array.

Code sample:
        // Si570 RECALL function
        uint8_t i2cError;
    r = usbCtrlMsgIN(0x20, 0x55 | (135<<8), 0x01, &i2cError, 1);
        if (r == 1 && i2cError == 0)
                // OK

Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x20
    value:           I2C Address low byte (only for the DG8SAQ firmware)
                     Si570 register high byte
    index:           Register value low byte
    bytes:           NULL
    size:            0
 */
BOOL srSetRegSi570(int reg, int value, int i2cAddr, BOOL* pI2CError) {
    int r;
    uint8_t iI2CError = 0;

    r = usbControlMsgIN(CMD_SET_SI570, (reg << 8) + (i2cAddr & 0xFF), value, (char*) &iI2CError, sizeof (iI2CError));

    if (pI2CError != NULL)
        *pI2CError = iI2CError != 0;

    return r == sizeof (iI2CError);
}

/*
Command 0x30:
-------------
Set the oscillator frequency by Si570 register. The real frequency will be 
calculated by the firmware and the called command 0x32

Default:    None

Parameters:
    requesttype:    USB_ENDPOINT_OUT
    request:         0x30
    value:           I2C Address (only for the DG8SAQ firmware), Don't care
    index:           7 (only for the DG8SAQ firmware), Don't care
    bytes:           pointer 48 bits register
    size:            6
 */
// i2cAddr only needed with SAQ version firmware.

BOOL srSetFreqReg(unsigned char reg[6], int i2cAddr) {
    int r;

    r = usbControlMsgOUT(CMD_SET_FREQ_REG, 0x0700 + i2cAddr, 0, (char*) &reg[0], 6); //sizeof(reg));

    return r == 6; //sizeof(reg);
}

/*
Command 0x32:
-------------
Set the oscillator frequency by value. The frequency is formatted in MHz
as 11.21 bits value. 
The "automatic band pass filter selection", "smooth tune", "one side calibration" and
the "frequency subtract multiply" are all done in this function. (if anabled in the firmware)

Default:    None

Parameters:
    requesttype:    USB_ENDPOINT_OUT
    request:         0x32
    value:           Don't care
    index:           Don't care
    bytes:           pointer 32 bits integer
    size:            4

Code sample:
    uint32_t iFreq;
    double   dFreq;

    dFreq = 30.123456; // MHz
    iFreq = (uint32_t)( dFreq * (1UL << 21) )
    r = usbCtrlMsgOUT(0x32, 0, 0, (char *)&iFreq, sizeof(iFreq));
    if (r < 0) Error
 */
// i2cAddr only needed with SAQ version firmware.

BOOL srSetFreq(double MHz, int i2cAddr) {
    int r = 0;
    uint32_t iFreq;
    uint32_t Si570_freq;
    uint8_t retry_count = 4;
    int status = TRUE;

    Si570_freq = (uint32_t) (MHz * (double) (1UL << 21) + 0.5);
    iFreq = (uint32_t) (MHz);
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] srSetFreq Called . MHz: %f, iFreq: %lu, i2cAddr: %d, Si570_format: %08lX \n",
    //        line_number++, MHz, iFreq, i2cAddr, Si570_freq);
    while (retry_count-- > 0 && r != 4) {
        r = usbControlMsgOUT(CMD_SET_FREQ, 0x0700 + i2cAddr, 0, (char*) (&iFreq), sizeof (iFreq));
        Sleep(10);
    }
    if (retry_count == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] srSetFreq . retry_count EXCEEDED: %d, status: %d \n",
                line_number++, retry_count, r);
    }
    if (r != 4) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] srSetFreq . usbControlMsgOUT Failed. r: %d, \n", line_number++, r);
        //MessageBoxA(NULL, "srSetFreq . Communications with the Transceiver FAILED. \r\n Has the Transceiver been power cycled? \r\n Check the USB Connection \r\n Then restart MSCC-Core \r\n",
        //	"MS-SDR", MB_OK | MB_ICONEXCLAMATION
        status = FALSE;
    }
    return status;
}

/*
Command 0x33:
-------------
Write new crystal frequency to EEPROM and use it. It can be changed to calibrate the device.
The frequency is formatted in MHz as a 8.24 bits value.

Default:    114.285 MHz

Parameters:
    requesttype:    USB_ENDPOINT_OUT
    request:         0x33
    value:           Don't care
    index:           Don't care
    bytes:           pointer 32 bits integer
    size:            4
 */
BOOL srSetXtalFreq(double MHz) {
    int r;
    uint32_t iFreq;

    iFreq = (uint32_t) (MHz * (1UL << 24) + 0.5);

    r = usbControlMsgOUT(CMD_SET_XTAL_INT, 0, 0, (char*) &iFreq, sizeof (iFreq));

    return r == sizeof (iFreq);
}

#if !defined(__linux__) && !defined(__APPLE__)
void* srOpen(int vid, int pid, const TCHAR* pManufacturer, const TCHAR* pProduct, const TCHAR* pSerialNumber, int iDevNum, int *status) {
    struct usb_bus *bus;
    struct usb_device *dev;
    //char l_converted_buffer[80] = { 0 };
    char l_temp[80] = {0};
    uint8_t i;
    uint8_t l_count = 0;
    int error = 0;


    if (pManufacturer != NULL && pManufacturer[0] == '\0') pManufacturer = NULL;
    if (pProduct != NULL && pProduct[0] == '\0') pProduct = NULL;
    if (pSerialNumber != NULL && pSerialNumber[0] == '\0') pSerialNumber = NULL;
    usb_init();
    usb_find_busses(); // find all busses
    usb_find_devices(); // find all connected devices

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vid && dev->descriptor.idProduct == pid) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] srOpen . Proficio found\n", line_number++);
                if ((srUsbHandle = usb_open(dev)) != NULL) {
                    if (iDevNum == -1 || iDevNum == dev->devnum) {
                        if (testUsbString(dev->descriptor.iManufacturer, srUsbInfo.Manufacturer, sizeof (srUsbInfo.Manufacturer), pManufacturer)
                                && testUsbString(dev->descriptor.iProduct, srUsbInfo.Product, sizeof (srUsbInfo.Product), pProduct)
                                && testUsbString(dev->descriptor.iSerialNumber, srUsbInfo.SerialNumber, sizeof (srUsbInfo.SerialNumber), pSerialNumber)) {
                            srUsbInfo.VID = vid;
                            srUsbInfo.PID = pid;

                            /*if(error = usb_set_configuration(usbHandle, 1) )
                                                                    {
                                                                            if(error < 0){
                                                                                srClose();
                             *status = 0;
                                                                                 print_time(0);
                                                                                fprintf(G_fp_logfile,"[%d] srOpen . usb_set_configuration FAILED: %d \n",line_number++,error);
                                                                                return NULL;
                                                                            }
                            }*/

                            if (error = usb_claim_interface(usbHandle, 0) < 0) {
                                srClose();
                                *status = 0;
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] srOpen . usb_claim_interface FAILED: %d \n", line_number++, error);
                                return NULL;
                            }
                            *status = usbGetStringAscii(srUsbHandle, dev->descriptor.iSerialNumber, 0x0409, G_serial_number, sizeof (G_serial_number));
                            if (*status > 0) {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] srOpen. G_serial_number: %ws, status: %d\n", line_number, G_serial_number, *status);
                                memcpy(l_temp, G_serial_number, sizeof (l_temp));
                                for (i = 0; i < (*status * 2); i++) {
                                    if (l_temp[i] != '\0') {
                                        G_Usb_serial_number[l_count++] = l_temp[i];
                                    }
                                }
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] srOpen . G_Usb_serial_number: %s, status: %d\n",
                                        line_number, G_Usb_serial_number, *status);
                                return usbHandle;
                            }
                        }
                    }
                    srClose();
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d]  srOpen . usb_open FAILED \n", line_number++);
                }
            }
        }
    }
    *status = 0;
    return NULL;
}
#endif /* !linux win32 srOpen */

#endif /* !MS_SDR_NO_USB */

