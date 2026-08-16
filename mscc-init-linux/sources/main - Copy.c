#include "extern.h"
#include "ExtIODll.h"
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "port_defines.h"
#include "lusb0_usb.h"
#include <WinBase.h>

#define TRUE 1
#define FALSE 0
#define MULTUS_VID 0x16C0
#define MULTUS_PID 0x05DC
#define PA_SAMPLE_TYPE      paFloat32
#define NO_OUPUT_DEVICE 100
#define MAX_NULL 38
#define MAX_INPUT_DEVICES 50

typedef float SAMPLE;
int modeChanged = FALSE;
int inputchannels = 0;
int myAudioAPItype = 0;
sp_float* inbuffer, * outbuffer;
sp_float wold = 0;
sp_cplx incplx[4097];
sp_cplx outcplx[4096];
float volumeLevel = -0.0f; //Start with the volume muted.
int G_input_device_index = NO_OUPUT_DEVICE;
int G_output_device_index = NO_OUPUT_DEVICE;
uint8_t G_Keyer_Installed = 0;

struct input_devices {
    int record_number;
    int device_index;
    char name[512];
    int num_channels;
    int default_input_device;
    int selected_input_device;
};

struct output_devices {
    int record_number;
    int device_index;
    char name[512];
    int num_channels;
    int default_output_device;
    int selected_output_device;
};

struct input_devices G_input_devices[MAX_INPUT_DEVICES];
struct output_devices G_output_devices[MAX_INPUT_DEVICES];
int num_input_devices_found = 0;

state mystate;
PaStream* stream;

const PaDeviceInfo* lpInfo;
PaStreamParameters inputParameters, outputParameters;

static uint8_t Multus_rig = FALSE;
void* srUsbHandle = NULL;
int srUsbTimeout = 500;
static srUsbInfo_t srUsbInfo;
TCHAR G_serial_number[MAX_PATH] = {0};
static char G_Multus_serial_number[MAX_PATH] = {0};
static char G_Usb_serial_number[MAX_PATH] = {0};
char G_string_mscc_port[16];
char G_string_mscc_IP[MAX_PATH];
char G_mscc_RPI_IP[MAX_PATH];
char G_string_server_port[16];
char G_string_server_IP[MAX_PATH];
int G_mscc_port = 0;
int G_server_port = 0;
char G_Sound_Init_File[] = "/sound-device.ini";
char comm_ports[256] = { 0 };
int line_number = 0;

uint8_t G_PCB_Version;
char Sound_device[MAX_PATH] = {0};
const char *homedir;
char G_l_path[MAX_PATH] = { 0 };

#define usbHandle  ((usb_dev_handle*)srUsbHandle)

/*BOOL srIsOpen()
{
        return srUsbHandle != NULL;
}*/


void srClose() {
    if (usbHandle != NULL) {
        usb_release_interface(usbHandle, 0);
        usb_close(usbHandle);
    }
    srUsbHandle = NULL;
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

    if (pTest != NULL)
    {
        if (wcsncmp(pTest, pStr, iLen) != 0)
        {
            return FALSE;
        }
    }
    return TRUE;
}

int srOpen(int vid, int pid, const TCHAR* pManufacturer, const TCHAR* pProduct, const TCHAR* pSerialNumber, int iDevNum, int *status) {
    struct usb_bus *bus;
    struct usb_device *dev;
    //char l_converted_buffer[80] = { 0 };
    char l_temp[80] = {0};
    uint8_t i;
    uint8_t l_count = 0;
    int error = 0;
    int return_status = -1;
    int local_status = 0;

    local_status = *status;
    if (pManufacturer != NULL && pManufacturer[0] == '\0') pManufacturer = NULL;
    if (pProduct != NULL && pProduct[0] == '\0') pProduct = NULL;
    if (pSerialNumber != NULL && pSerialNumber[0] == '\0') pSerialNumber = NULL;
    usb_init();
    usb_find_busses(); // find all busses
    usb_find_devices(); // find all connected devices

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vid && dev->descriptor.idProduct == pid) {
                //print_time(0);
                fprintf(stdout, "TRANSCEIVER FOUND\n");
                return_status = 0;
                if ((srUsbHandle = usb_open(dev)) != NULL) {
                    if (iDevNum == -1 || iDevNum == dev->devnum) {
                        if (testUsbString(dev->descriptor.iManufacturer, srUsbInfo.Manufacturer, sizeof (srUsbInfo.Manufacturer), pManufacturer)
                                && testUsbString(dev->descriptor.iProduct, srUsbInfo.Product, sizeof (srUsbInfo.Product), pProduct)
                                && testUsbString(dev->descriptor.iSerialNumber, srUsbInfo.SerialNumber, sizeof (srUsbInfo.SerialNumber), pSerialNumber)) {
                            srUsbInfo.VID = vid;
                            srUsbInfo.PID = pid;
                            if (error = usb_claim_interface(usbHandle, 0) < 0) {
                                srClose();
                                return_status = -1;
                                printf("srOpen -> usb_claim_interface FAILED: %d \n", error);
                            }
                            else {
                                *status = usbGetStringAscii(srUsbHandle, dev->descriptor.iSerialNumber, 0x0409, G_serial_number, 
                                    sizeof(G_serial_number));
                                if (*status > 0) {
                                    //print_time(0);
                                    //printf("srOpen -> G_serial_number: %s, status: %d\n", (char*)G_serial_number, local_status);
                                    memcpy(l_temp, G_serial_number, sizeof(l_temp));
                                    for (i = 0; i < (*status * 2); i++) {
                                        if (l_temp[i] != '\0') {
                                            G_Usb_serial_number[l_count++] = l_temp[i];
                                        }
                                    }
                                    G_Usb_serial_number[l_count] = 0;
                                    printf("TRANSCEIVER SERIAL NUMBER: %s\n",
                                        G_Usb_serial_number);
                                }
                            }
                        }
                    }
                    srClose();
                }
            }
        }
    }
    return return_status;
}

char* My_getenv(char* myenv) {

    //FILE* Multus_ini;
    WCHAR path[MAX_PATH] = { 0 };
    PWSTR lpPath = path;
    int lenght = 0;

    memset(G_l_path, 0, sizeof(G_l_path));

    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
    //fprintf("get_serial_number -> Get folder path \n");
    if (SUCCEEDED(hr)) {
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, G_l_path, sizeof(G_l_path), NULL, NULL);
        strcat(G_l_path, "/MSCC-MKII");
        //printf("getenv: %s\n", G_l_path);
    }
    //$HOME = G_l_path;
    return G_l_path;
}

int init_ini() {
    int status = 01;
    FILE *Multus_ini;
    char l_path[MAX_PATH] = {0};
    //char directry[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/Multus_mscc.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            fprintf(Multus_ini, "PROFICIO_SERIAL_NUMBER=%s;\n", G_Usb_serial_number);
            fprintf(Multus_ini, "MSCC_PORT=%d;\n", G_mscc_port);
            fprintf(Multus_ini, "MSCC_IP=%s;\n", G_string_mscc_IP);
            fprintf(Multus_ini, "PROFICIO_DLL_PORT=%d;\n", G_server_port);
            fprintf(Multus_ini, "PROFICIO_DLL_IP=%s;\n", G_string_server_IP);
            fprintf(Multus_ini, "PCB_VERSION=%d;\n", G_PCB_Version);
            fputs("CW_Restore_Defaults=0;\n", Multus_ini);
            fputs("CW_Iambic_Mode_On_Off=0;\n", Multus_ini);
            fputs("CW_Iambic_Type=0;\n", Multus_ini);
            fputs("CW_Iambic_Calibrate=120;\n", Multus_ini);
            fputs("CW_Memory=3;\n", Multus_ini);
            fputs("CW_Spacing=1;\n", Multus_ini);
            fputs("CW_Paddle=0;\n", Multus_ini);
            fputs("CW_Weight=50;\n", Multus_ini);
            fputs("CW_Tx_Hold=10;\n", Multus_ini);
            fputs("CW_Speed=18;\n", Multus_ini);
            fputs("CW_Semi_Break_In=0;\n", Multus_ini);
            fputs("CW_Semi_Control=0;\n", Multus_ini);
            fputs("CW_Side_Tone=1;\n", Multus_ini);
            fputs("Highlimit=3000;\n", Multus_ini);
            fflush(Multus_ini);
            fclose(Multus_ini);
        } else status = -1;
    } else status = -1;
    return status;
}



int Create_Fortis_I2C(int mfc, int meter) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[PATH_MAX] = { 0 };
    char directry[PATH_MAX] = { 0 };

    if ((homedir = getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/i2c.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            fputs("G_MASTER_CONTROLLER_attached=2;\n", Multus_ini);
            fprintf(Multus_ini, "G_MFC_attached=%d;\n", mfc);
            fputs("G_SOLIDUS_TEMP_SENSOR_attached=0;\n", Multus_ini);
            fprintf(Multus_ini, "G_METER_attached=%d;\n", meter);
            fputs("G_IQBD_attached=0;\n", Multus_ini);
            fputs("G_CURRENT_SENSOR_attached=0;\n", Multus_ini);
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else status = -1;
    }
    else status = -1;
    return status;
}

int Update_CW_ini() {
    FILE* fp_cw_ini;
    char file_name[PATH_MAX] = { 0 };
    int status = 0;

    if ((homedir = getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/.local/share/mscc/cw.ini");

        fp_cw_ini = fopen(file_name, "w");
        if (fp_cw_ini != NULL) {
            rewind(fp_cw_ini);
            fprintf(fp_cw_ini, "CW_Keyer_Installed=%d;\n", G_Keyer_Installed);
            fprintf(fp_cw_ini, "CW_Keyer_Mode=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Iambic_Type=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Iambic_Calibrate=%d;\n", 120);
            fprintf(fp_cw_ini, "CW_Memory=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Spacing=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Paddle=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Weight=%d;\n", 50);
            fprintf(fp_cw_ini, "CW_Tx_Hold=%d;\n", 15);
            fprintf(fp_cw_ini, "CW_Speed=%d;\n", 18);
            fprintf(fp_cw_ini, "CW_Semi_Break_In=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Semi_Control=%d;\n", 0);
            fprintf(fp_cw_ini, "CW_Side_Tone_Volume=%d;\n", 0);
            fflush(fp_cw_ini);
            fclose(fp_cw_ini);
        }
        else status = -1;
    }
    else status = -1;
    return status;
}

/*int Create_MSCC_PW_I2C(int transceiver_type,int mfc, int meter, int rego_serial_port,int rego) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    //char comm_port_number[2] = { 0 };


    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/i2c.ini");
        //_itoa(rego_serial_port, comm_port_number, 10);
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            fprintf(Multus_ini,"G_MASTER_CONTROLLER_attached=%d;\n", transceiver_type);
            fprintf(Multus_ini, "G_REGO_attached=%d;\n", rego);
            fprintf(Multus_ini, "G_MFC_attached=%d;\n", mfc);
            fputs("G_SOLIDUS_TEMP_SENSOR_attached=0;\n", Multus_ini);
            fprintf(Multus_ini, "G_METER_attached=%d;\n", meter);
            fputs("G_IQBD_attached=0;\n", Multus_ini);
            fputs("G_CURRENT_SENSOR_attached=0;\n", Multus_ini);
            fprintf(Multus_ini, "G_I2C_SERIAL_PORT=%d;\n", rego_serial_port);
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else status = -1;
    }
    else status = -1;
    return status;
}*/

void build_output_devices(int device_index) {
    static int index = 0;
    int ret = 0;
    char temp_string[MAX_PATH] = { 0 };
    char* name = NULL;
    char* mapper = NULL;

    if (index < MAX_INPUT_DEVICES) {
        /*strcpy(temp_string, lpInfo->name);
        name = strstr(temp_string, "(");
        if (name != NULL) {
            strcpy(G_output_devices[index].name, &name[1]);
        }
        else {
            strcpy(G_output_devices[index].name, temp_string);
        }*/
        strcpy(G_output_devices[index].name, lpInfo->name);
        G_output_devices[index].device_index = device_index;
        G_output_devices[index].num_channels = lpInfo->maxOutputChannels;
        G_output_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultOutputDevice) {
            G_output_devices[index].default_output_device = 1;
        }
        name = strstr(G_output_devices[index].name, "Multus");
        mapper = strstr(G_output_devices[index].name, "Mapper");
        if (name == NULL && mapper == NULL) {
            printf("Speaker Device: Number: %d, name: %s \n",
                index, G_output_devices[index].name);
        }
        index++;
        num_input_devices_found = index;
    }
    else {
        printf("Number of output devices exceed permitted limit\n");
    }
}

void build_input_devices(int device_index) {
    static int index = 0;
    int ret = 0;
    char temp_string[MAX_PATH] = { 0 };
    char* name = NULL;
    char* mapper = NULL;

    if (index < MAX_INPUT_DEVICES) {
        strcpy(temp_string, lpInfo->name);
        //name = strstr(temp_string, "(");
        //if (name != NULL) {
        //    strcpy(G_input_devices[index].name, &name[1]);
        //}
        //else {
        //    strcpy(G_input_devices[index].name, temp_string);
        //}
        strcpy(G_input_devices[index].name, temp_string);
        G_input_devices[index].device_index = device_index;
        G_input_devices[index].num_channels = lpInfo->maxInputChannels;
        G_input_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultInputDevice) {
            G_input_devices[index].default_input_device = 1;
        }
        name = strstr(G_input_devices[index].name, "Multus");
        mapper = strstr(G_input_devices[index].name, "Mapper");
        if (name == NULL && mapper == NULL) {
            printf("Microphone Device: Number: %d, name: %s \n",
                index, G_input_devices[index].name);
            index++;
        }
        num_input_devices_found = index;
    }
    else {
        printf("Number of input devices exceed permitted limit\n");
    }
}
void Get_Speaker_Audio_Device() {

    PaDeviceIndex devCount = Pa_GetDeviceCount();
    PaDeviceIndex recordDevice = 0;
    PaDeviceIndex playDevice = 0;
    int j = 0;

    // Get # of host API's supported on THIS machine at THIS time.
    const PaHostApiInfo* lpApiInfo;
    PaHostApiIndex hostApiCount;
    int apiTypeMME = 9999;
    int apiTypeDSOUND = 9999;
    int apiTypeWDMKS = 9999;
    int apiTypeWASAPI = 9999;
    int apiTypeASIO = 9999;
    int apiTypeALSA = 9999;

    hostApiCount = Pa_GetHostApiCount();

    printf("Get_Speaker_Audio_Device \n");
    for (j = 0; j < hostApiCount; j++) {
        lpApiInfo = Pa_GetHostApiInfo(j);
        if (!strncmp("MME", lpApiInfo->name, 3)) {
            apiTypeMME = j;
        }
    }

    for (int j = 0; j < devCount; j++) {
        lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
        if (lpInfo->hostApi == apiTypeMME) {
            if (lpInfo->maxOutputChannels >= 1) {
                build_output_devices((PaDeviceIndex)j);
            }
        }
    }
}

void Get_Microphone_Audio_Device() {

    PaDeviceIndex devCount = Pa_GetDeviceCount();
    PaDeviceIndex recordDevice = 0;
    PaDeviceIndex playDevice = 0;
    int j = 0;


    // Get # of host API's supported on THIS machine at THIS time.
    const PaHostApiInfo* lpApiInfo;
    PaHostApiIndex hostApiCount;
    int apiTypeMME = 9999;
    int apiTypeDSOUND = 9999;
    int apiTypeWDMKS = 9999;
    int apiTypeWASAPI = 9999;
    int apiTypeASIO = 9999;
    int apiTypeALSA = 9999;

    hostApiCount = Pa_GetHostApiCount();

    for (j = 0; j < hostApiCount; j++) {
        lpApiInfo = Pa_GetHostApiInfo(j);
        if (!strncmp("MME", lpApiInfo->name, 3)) {
            apiTypeMME = j;
        }
    }

    for (int j = 0; j < devCount; j++) {
        lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
        if (lpInfo->hostApi == apiTypeMME) {
            if (lpInfo->maxInputChannels >= 1) {
                build_input_devices((PaDeviceIndex)j);
            }
        }
    }
}

int Update_Operator_Speaker_Config(int record) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    char device_number[2] = { 0 };
    char* name = NULL;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/operator-speaker.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            name = strtok(G_output_devices[record].name, "(");
            if (name != NULL) {
                //fprintf(Multus_ini,"%s", G_output_devices[record].name);
                fprintf(Multus_ini, "%s", name);
            }
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else
        {
            status = -1;
            printf("Open FAILED. %s: \n", l_path);
        }
    }
    else {
        status = -1;
        printf("My_getevn FAILED. %s: \n", homedir);
    }
    return status;
}

int Update_Digital_Speaker_Config(int record) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    char device_number[2] = { 0 };
    char* name = NULL;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/digital-speaker.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            name = strtok(G_output_devices[record].name, "(");
            if (name != NULL) {
                //fprintf(Multus_ini,"%s", G_output_devices[record].name);
                fprintf(Multus_ini, "%s", name);
            }
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else
        {
            status = -1;
            printf("Open FAILED. %s: \n", l_path);
        }
    }
    else {
        status = -1;
        printf("My_getevn FAILED. %s: \n", homedir);
    }
    return status;
}

int Update_Operator_Microphone_Config(int record) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    char device_number[2] = { 0 };
    char* name = NULL;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/operator-microphone.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            name = strtok(G_input_devices[record].name, "(");
            if (name != NULL) {
                //fprintf(Multus_ini,"%s", G_output_devices[record].name);
                fprintf(Multus_ini, "%s", name);
            }
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else
        {
            status = -1;
            printf("Open FAILED. %s: \n", l_path);
        }
    }
    else {
        status = -1;
        printf("My_getevn FAILED. %s: \n", homedir);
    }
    return status;
}

int Update_Digital_Microphone_Config(int record) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    char device_number[2] = { 0 };
    char* name = NULL;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/digital-microphone.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            name = strtok(G_input_devices[record].name, "(");
            if (name != NULL) {
                //fprintf(Multus_ini,"%s", G_output_devices[record].name);
                fprintf(Multus_ini, "%s", name);
            }
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else
        {
            status = -1;
            printf("Open FAILED. %s: \n", l_path);
        }
    }
    else {
        status = -1;
        printf("My_getevn FAILED. %s: \n", homedir);
    }
    return status;
}

int init_mscc() {
    int status = 1;
    FILE* Multus_ini;
    char l_path[PATH_MAX] = { 0 };

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/mscc.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            fprintf(Multus_ini, "PROFICIO_SERIAL_NUMBER=%s;\n", G_Usb_serial_number);
            fprintf(Multus_ini, "MSCC_PORT=%d;\n", G_mscc_port);
            fprintf(Multus_ini, "MSCC_IP=%s;\n", G_string_mscc_IP);
            fprintf(Multus_ini, "PROFICIO_DLL_PORT=%d;\n", G_server_port);
            fprintf(Multus_ini, "PROFICIO_DLL_IP=%s;\n", G_string_server_IP);
            fprintf(Multus_ini, "PCB_VERSION=%d;\n", G_PCB_Version);
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else status = -1;
    }
    else status = -1;
    return status;
}

int Update_Serial_Config(int record) {
    int status = 0;
    FILE* Multus_ini;
    char l_path[MAX_PATH] = { 0 };
    char directry[MAX_PATH] = { 0 };
    char comm_port_number[2] = { 0 };

    _itoa(record, comm_port_number, 10);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/comm-port.ini");
        Multus_ini = fopen(l_path, "w");
        if (Multus_ini != NULL) {
            fprintf(Multus_ini, "COMM_PORT_NAME=COM%s,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN=1;", 
                comm_port_number);
            fflush(Multus_ini);
            fclose(Multus_ini);
        }
        else
        {
            status = -1;
            printf("Open FAILED. %s: \n", l_path);
        }
    }
    else {
        status = -1;
        printf("My_getevn FAILED. %s: \n", homedir);
    }
    return status;
}

int Get_Serial_Ports(int rego_port) {
    int status = -1;
    HANDLE hSerial = 0;
    DWORD read_timeout = 0;
    char windows_comm_port[20] = { 0 };
    char error_message[80] = { 0 };
    char G_comm_port[80] = { 0 };
    char lastError[1024] = { 0 };
    int i = 0;
    int count = 0;

    for (i = 0; i < 256; i++) {
        _itoa(i, G_comm_port, 10);
        sprintf(windows_comm_port, "\\\\.\\COM%s", G_comm_port);
        hSerial = CreateFileA(windows_comm_port,
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0);

        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            if(i != rego_port)
            printf("Serial Port Found: %s -> COM%s\n", G_comm_port,G_comm_port);
            status = 0;
        }
        memset(G_comm_port, 0, sizeof(G_comm_port));
    }
    return status;
}

void Audio_Device_Error(PaError err) {

    Pa_Terminate();
    fprintf(stdout, "[%d] An error occured while using the portaudio stream\n", line_number++);
    fprintf(stdout, "[%d] Error number: %d\n", line_number++, err);
    fprintf(stdout, "[%d] Error message: %s\n", line_number++, Pa_GetErrorText(err));
    MessageBoxA(NULL, "SDRcore-recv initialization FAILED.  Send logs to Multus SDR, LLC", "SDRcore-recv",
        MB_OK | MB_ICONEXCLAMATION);
}

int main(int argc, char **argv) {
    int status = 0;
    int usb_status;
    int exit_status = 0;
    int ret = 0;
    int scan_status = 0;
    uint8_t rego = 0;
    uint8_t mfc = 0;
    uint8_t meter = 0;
    uint8_t rego_serial_port = 254;
    WSADATA dll_wsa;
    uint8_t selected_sound_device = 0;
    uint8_t selected_comm_port = 0;
    int transceiver_type = 7;
    PaError err;
    uint8_t speaker_device = 0;
    uint8_t microphone_device = 0;

    printf("mscc-pw-cfg: THIS UTILITY CONFIGURES THE REQUIRED BASE CONFIGURATION NEEDED FOR MSCC-MKII\n");
    status = srOpen(MULTUS_VID, MULTUS_PID,L"", L"", L"", -1, &usb_status);
    //printf("srOpen status: %d \n", status);
    if (status >= 0) {
        G_mscc_port = MSCC_PORT;
        G_server_port = MS_SDR_PORT;
        G_string_mscc_IP[0] = 'N';
        if (WSAStartup(MAKEWORD(2, 2), &dll_wsa) != NO_ERROR)
        {
            printf("mscc-pw-cfg: Initializing Winsock: Failed. Error Code : %d\n",WSAGetLastError());
            exit(1);
        }
        ret = gethostname(G_string_server_IP, sizeof (G_string_server_IP));
        strcpy(G_mscc_RPI_IP, G_string_server_IP);
        strcpy(G_string_mscc_IP, G_string_server_IP);
        printf("HOSTNAME: %s\n", G_string_server_IP);
        G_PCB_Version = 0;
        printf("IS THE PROFICIO MKII KEYER INSTALLED? ENTER 0 FOR NO, 1 FOR YES ");
        scan_status = scanf("%hhd", &G_Keyer_Installed);

        printf("Enter the number that corresponds to the transceiver model that is attached to the PC\n");
        printf("Proficio PCB VERSION: Original -> 1, Rev C -> 2,  4.x -> 4, 5.x -> 5, AFTER 6/2021 -> 9, MARK II -> 10: \n");
        printf("Geminus: 6:\n");
        printf("Magnis Duo: 7:\n");
        printf("Ultimus: 8:\n");
        printf("ENTER MODEL NUMBER: ");
        scan_status = scanf("%hhd", &G_PCB_Version);
        ret = getchar();
        switch (G_PCB_Version) {
            case 1:
                printf("Proficio: Original PCB Selected\n");
                break;
            case 2:
                printf("Proficio: PCB Rev C Selected \n");
                break;
            case 4:
                printf("Proficio: PCB Version 4.x Selected \n");
                break;
            case 5:
                printf("Proficio: PCB Version 5.x Selected \n");
                break;
            case 6:
                printf("Geminus Selected \n");
                transceiver_type = G_PCB_Version;
                break;
            case 7:
                printf("Magnus Duo Selected \n");
                break;
            case 8:
                printf("Ultimus Selected \n");
                break;
            case 9:
                printf("Proficio After 6/2021 Selected \n");
                break;
            case 10:
                printf("Proficio MARK II Selected \n");
                break;
            default:
                printf("Invalid response:  Run initialize again with correct transceiver model \n");
                status = -1;
        }
        if (status >= 0) {
            status = Create_Fortis_I2C(mfc,meter);
            if (status >= 0) {
                status = init_mscc();
                if (status < 0) {
                    printf("mscc.ini Creation Failed\n");
                    exit_status = 5;
                } else {
                    status = Update_CW_ini();
                    if (status < 0) {
                        printf("cw.ini Creation Failed\n");
                        exit_status = 5;
                    }
                }
            }
        } else {
            exit_status = 5;
        }
    } else {
        printf("TRANSCEIVER INITIALIZATION FAILED.  TRANSCEIVER NOT ATTACHED TO USB PORT OR THE TRANSCEIVER IS IN USE\n");
        exit_status = 5;
    }
    if (exit_status != 5) {
        status = Get_Serial_Ports(rego_serial_port);
        printf("SELECT A SERIAL PORT FOR CAT CONTROL OR ZERO (0) FOR NO CAT CONTROL: ");
        scan_status = scanf("%hhd", &selected_comm_port);
        if (selected_comm_port == 0) {
            Update_Serial_Config(0);
            printf("NO SERIAL PORT FOR CAT CONTROL CONFIGURED\n");
            printf("Enter any key to quit\n");
            ret = getchar();
            ret = getchar();
        }
        else {
            status = Update_Serial_Config(selected_comm_port);
            if (status >= 0) {
                printf("SERIAL PORT FOR CAT CONTROL SETUP SUCCESSFUL FOR COM%d \n", selected_comm_port);
                //printf("Enter any key to quit\n");
                //ret = getchar();
                //ret = getchar();
            }
        }
        err = Pa_Initialize();
        if (err != paNoError) Audio_Device_Error(err);
        printf("main. PortAudio Version: 0x%8X\n", Pa_GetVersion());
        printf("main. PortAudio Version text: '%s'\n",
            Pa_GetVersionInfo()->versionText);

        Get_Speaker_Audio_Device();
        printf("Enter Operator Speaker device number: ");
        scan_status = scanf("%hhd", &speaker_device);
        Update_Operator_Speaker_Config(speaker_device);
        printf("Enter Digital Speaker device number: ");
        scan_status = scanf("%hhd", &speaker_device);
        Update_Digital_Speaker_Config(speaker_device);

        Get_Microphone_Audio_Device();
        printf("Enter Operator Microphone device number: ");
        scan_status = scanf("%hhd", &microphone_device);
        Update_Operator_Microphone_Config(microphone_device);
        printf("Enter Digital Microphone device number: ");
        scan_status = scanf("%hhd", &microphone_device);
        Update_Digital_Microphone_Config(microphone_device);
    }
    else {
        printf("mscc-pw-cfg. FAILED. \n");
        printf("Enter any key to quit ");
        ret = getchar();
    }
    return exit_status;
}