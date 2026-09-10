#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <malloc.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "usbavrcmd.h"
#include <usb.h>
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <wiringPi.h>

#define LOW 0
#define HIGH 1
#define PTT_OFF 0
#define PTT_ON 1
#define HIGH_POWER_COUNT_LIMIT 20

#define OVER_TEMPERATURE 71
#define FAN_OFF_TEMPERATURE 29
#define FAN_ON_TEMPERATURE 30
#define PERIOD 300
#define FAN_PWM 0
#define MAX_PINS 2
//#define PULSE_TIME 100
#define KICK_TIME 1000

//These are Raspberry Pi GPIO Pins
#define POWER_MODE_PIN 4
#define TX_MODE_PIN 5
#define BUTTON_1 13
#define BUTTON_2 19
#define BUTTON_3 26
#define CW_KEY_IN_0 6
#define CW_KEY_IN_1 7
#define CW_TONE_GEN_IN 8
#define CW_KEY_OUT_1B 12
#define PTT 25
#define FAN 1
#define ANTENNA_SENSE 26

//Fortis Pin Configuration MCP23008
#define FORTIS_ANTENNA_SWITCH_OFF 0X01 //INACTIVE (OFF) HIGH
#define FORTIS_FILTER_160 0x0A
#define FORTIS_FILTER_80 0x08
#define FORTIS_FILTER_60_40 0x06
#define FORTIS_FILTER_30_20 0x04
#define FORTIS_FILTER_17_15 0x02
#define FORTIS_FILTER_12_10 0x00
#define FORTIS_FILTER_NO_BAND 0x01


//Solidus Pin Configuration MCP23017
//On GP_A
#define QRP_MODE 0x02
#define QRO_MODE 0x01
#define QRO_TRANSMIT 0x0C
#define FAN_OFF 0x80 //INACTIVE (OFF) HIGH
#define IQBD_ON 0x40;
#define ANTENNA_SWITCH_OFF 0x20 //INACTIVE (OFF) HIGH
#define SET_FAN_ON GP_A & ~FAN_OFF
#define SET_FAN_OFF GP_A | FAN_OFF

//On GP_B
#define TEMPERATURE_OK 0x08 
#define FILTER_160 0x05
#define FILTER_80 0x04
#define FILTER_60_40 0x03
#define FILTER_30_20 0x02
#define FILTER_17_15 0x01
#define FILTER_12_10 0x00
#define FILTER_NO_BAND 0xF8

const char *thread_name;

byte GP_A = {0};
byte GP_B = {0};


byte G_IQBD_allow = FALSE;
byte G_BIAS_Mode = FALSE;
byte G_BIAS_allow = FALSE;
byte G_TEMP_allow = FALSE;
byte G_Display_allow = FALSE;
byte G_current_mode = 0xFF;
byte IQBD_Set = FALSE;
byte G_QRO_Relays_on = FALSE;
byte G_MFC_allow = FALSE;
byte G_FAN_Control = 1;
byte G_FAN_On_Temperature = 30;
byte over_temperature = FALSE;
byte PWM_Run = 0;
int G_kick = 1;
byte G_QRP_TX_ACTIVE = FALSE;
byte G_Solidus_TX_Relay_ON = FALSE;

pthread_t PWM_thread;
int PWM_rc;
static volatile int marks [MAX_PINS];
static volatile int range [MAX_PINS];
//static volatile pthread_t threads [MAX_PINS];
//static volatile int newPin = -1;
pthread_t Solidus_Check_TX_Relay_thread;
int Solidus_Check_TX_Relay_rc;

static void *softPwmThread(void *arg) {
    int pin, mark, space;

    pin = FAN;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s %s . softPwmThread Started. Pin: %d \n",
            line_number++, thread_name, __func__, pin);
    while (PWM_Run) {
        if (G_kick == TRUE) {
            digitalWrite(FAN, HIGH);
            Sleep(KICK_TIME);
            G_kick = FALSE;
        }
        mark = marks [pin];
        space = range [pin] - mark;
        if (mark != 0)
            digitalWrite(pin, HIGH);
        delayMicroseconds(mark * 100);
        if (space != 0)
            digitalWrite(pin, LOW);
        delayMicroseconds(space * 100);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s %s . softPwmThread Finished. Pin: %d \n",
            line_number++, thread_name, __func__, pin);
    pthread_exit(0);
    return NULL;
}

int Start_Soft_PWM(int pin, int initialValue, int pwmRange) {
    long t;
    int status = 0;

    t = pin;
    print_time(0);
    marks[pin] = initialValue;
    range[pin] = pwmRange;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s %s . Starting Start_Soft_PWM thread \n", line_number++, thread_name, __func__);
    PWM_rc = pthread_create(&PWM_thread, NULL, softPwmThread, (void *) t);
    if (PWM_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s %s . Start Start_Soft_PWM thread failed, return code from pthread_create() is %d\n",
                line_number++, thread_name, __func__, PWM_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s %s . Start Start_Soft_PWM thread Started, return code from pthread_create() is %d\n",
                line_number++, thread_name, __func__, PWM_rc);
    }
    return status;
}

void softPwmWrite(int pin, int value) {
    marks [pin] = value;
}

int Do_Pulse() {
    int status = 0;
    int on_time = 0;
    float ratio = 0.0f;
    //float max = 0;
    int delta = 0;
    float scale = 0.0f;
    static int previous_on_time = 0;

    scale = (float) PERIOD / (float) G_FAN_On_Temperature;
    if (G_Solidus_Temperature < 40) {
        scale = scale / 4.0f;
    } else {
        if (G_Solidus_Temperature >= 40 && G_Solidus_Temperature < 50) {
            scale = scale / 3.0f;
        } else {
            if (G_Solidus_Temperature >= 50 && G_Solidus_Temperature < 60) {
                scale = scale / 2.0f;
            }
        }
    }
    delta = G_Solidus_Temperature - G_FAN_On_Temperature;
    ratio = (float) delta * scale;
    on_time = (int) ratio;
    if (on_time <= 30) {
        on_time = 30;
    }
    if (on_time >= PERIOD) {
        on_time = PERIOD;
    }
    if (previous_on_time != on_time) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s %s . on_time: %d, scale: %f, delta: %d, ratio: %f \n",
                line_number++, thread_name, __func__, on_time, scale, delta, ratio);
        previous_on_time = on_time;
    }
    softPwmWrite(FAN, on_time);
    return status;
}

void *Fan_PWM(void *param) {
    int status = 0;
    int sleep_time = 10;
    int fan_idle = FALSE;
    byte power_mode = QRP_MODE;
    byte previous_power_mode = QRO_MODE;

    digitalWrite(FAN, LOW);
    Sleep(9000);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . START\n", line_number++, __func__);
    while (G_all_threads_run) {
        if ((G_Solidus_Temperature < G_FAN_On_Temperature)) {
            if (fan_idle == FALSE) {
                digitalWrite(FAN, LOW);
                PWM_Run = 0;
                status = 0;
                G_kick = 1;
                sleep_time = 10;
                fan_idle = TRUE;
            }
        } else {
            power_mode = G_current_mode;
            if (previous_power_mode != power_mode) {
                if (power_mode == QRO_MODE) {
                    G_kick = 1;
                }
                previous_power_mode = power_mode;
            }
            if (G_FAN_Control == FAN_PWM) {
                if (PWM_Run == 0) {
                    PWM_Run = 1;
                    sleep_time = 1;
                    status = Start_Soft_PWM(FAN, 0, PERIOD);
                    fan_idle = FALSE;
                    if (status != 0) {
                        PWM_Run = 0;
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s . PWM did not start: %d\n", line_number++, __func__, status);
                    }
                }
                if (status == 0) {
                    Do_Pulse();
                }
            } else {
                PWM_Run = 0;
                digitalWrite(FAN, HIGH);
                G_kick = 1;
                sleep_time = 10;
                fan_idle = FALSE;
            }
        }
        Sleep(sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT\n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}

int Manage_PTT_Rear_Switch() {
    int status = 0;
    byte PTT_pin_state;
    static byte previous_PTT_pin_state;
    byte PTT_control;

    PTT_pin_state = digitalRead(PTT);
    if (previous_PTT_pin_state != PTT_pin_state) {
        if (PTT_pin_state == HIGH) {
            PTT_control = PTT_OFF;
        } else {
            PTT_control = PTT_ON;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Master_controller.  %s. PTT_control: %d \n",
                line_number++, __func__, PTT_control);
        SDRcore_recv_send_param(CMD_SET_TX_ON, PTT_control);
        SDRcore_trans_send_param(CMD_SET_TX_ON, PTT_control);
        Set_PTT(PTT_control,FALSE);
        Gui_send_param(CMD_SET_TX_SET_BY_SERVER, PTT_control);
        previous_PTT_pin_state = PTT_pin_state;
    }
    return status;
}

int Manage_MCP23017() {
    int status = 0;
    static unsigned char previous_gp_a = 0;
    static unsigned char previous_gp_b = 0;

    if ((previous_gp_a != GP_A) || (previous_gp_b != GP_B)) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_MCP23017 GP_A: 0x%02X, GP_B: 0x%02X\n",
        //        line_number++, GP_A, GP_B);
        status = MCP23017_Controller_main(GP_A, GP_B);
        previous_gp_a = GP_A;
        previous_gp_b = GP_B;
        if (G_QRP_TX_ACTIVE == TRUE) {
            G_QRP_TX_ACTIVE = FALSE;
        }
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_MCP23017 . FINISHED\n", line_number++);
    }
    return status;
}

int Manage_MCP23008() {
    int status = 0;
    static unsigned char previous_gp_a = 0;

    if ((previous_gp_a != GP_A)) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_MCP23008 GP_A: 0x%02X \n",
        //        line_number++, GP_A);
        status = MCP23008_Controller_main(GP_A);
        previous_gp_a = GP_A;
        if (G_QRP_TX_ACTIVE == TRUE) {
            G_QRP_TX_ACTIVE = FALSE;
        }
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_MCP23008 . FINISHED\n", line_number++);
    }
    return status;
}

void Manage_antenna_sense() {
    int antenna_sense = 0;
    static int previous_antenna_sense = 100;

    if (G_MSCC_Initialized == TRUE) {
        antenna_sense = digitalRead(ANTENNA_SENSE);
        if (previous_antenna_sense != antenna_sense) {
            Gui_send_param(CMD_SET_ANTENNA_SENSE, antenna_sense);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . antenna_sense: %d \n",
                    line_number++, thread_name, __func__, antenna_sense);
            previous_antenna_sense = antenna_sense;
        }
    }
}

void Manage_Solidus_antenna_switch() {
    static byte previous_antenna_on_off = 100;

    if (G_TX == FALSE) {
        if (G_Antenna_Switch != previous_antenna_on_off) {
            previous_antenna_on_off = G_Antenna_Switch;
            if (G_Antenna_Switch == 0) {
                GP_A = GP_A | ANTENNA_SWITCH_OFF;
            } else {
                GP_A = GP_A & ~ANTENNA_SWITCH_OFF;
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . G_Antenna_Switch: %d, GP_A: 0x%02X  \n",
                    line_number++, thread_name, __func__, G_Antenna_Switch, GP_A);
        }
    }
}

void Manage_Fortis_antenna_switch() {
    static byte previous_antenna_on_off = 100;

    if (G_TX == FALSE) {
        if (G_Antenna_Switch != previous_antenna_on_off) {
            previous_antenna_on_off = G_Antenna_Switch;
            if (G_Antenna_Switch == 0) {
                GP_A = GP_A | FORTIS_ANTENNA_SWITCH_OFF;
            } else {
                GP_A = GP_A & ~FORTIS_ANTENNA_SWITCH_OFF;
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . G_Antenna_Switch: %d, GP_A: 0x%02X  \n",
                    line_number++, thread_name, __func__, G_Antenna_Switch, GP_A);
        }
    }
}

void Manage_Solidus_filters() {
    static byte previous_band = 0xff;
    uint16_t band = FREQ_OUT_OF_RANGE;

    if (G_band_normal == FREQ_OUT_OF_RANGE) {
        band = G_Amplifier_band;
    } else {
        band = G_band_normal;
    }
    if (previous_band != band) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Solidus_filters - START \n", line_number++);
        GP_B = GP_B & FILTER_NO_BAND;
        switch (band) {
            case 160:
                GP_B = GP_B | FILTER_160;
                break;
            case 80:
                GP_B = GP_B | FILTER_80;
                break;
            case 60:
                GP_B = GP_B | FILTER_60_40;
                break;
            case 40:
                GP_B = GP_B | FILTER_60_40;
                break;
            case 30:
                GP_B = GP_B | FILTER_30_20;
                break;
            case 20:
                GP_B = GP_B | FILTER_30_20;
                break;
            case 17:
                GP_B = GP_B | FILTER_17_15;
                break;
            case 15:
                GP_B = GP_B | FILTER_17_15;
                break;
            case 12:
                GP_B = GP_B | FILTER_12_10;
                break;
            case 10:
                GP_B = GP_B | FILTER_12_10;
                break;
            default:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Solidus_filters. Invalid band\n",
                        line_number++);
                break;
        }
        previous_band = band;
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Solidus_filters . FINISHED. Filter: 0x%02X \n",
        //        line_number++, GP_B);
    }
}

void Manage_Fortis_filters() {
    static byte previous_band = 0xff;
    int16_t band = FREQ_OUT_OF_RANGE;

    if (G_band_normal == FREQ_OUT_OF_RANGE) {
        band = G_Amplifier_band;
    } else {
        band = G_band_normal;
    }
    if (previous_band != band) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Fortis_filters - START \n", line_number++);
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
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Fortis_filters. Invalid band\n",
                        line_number++);
                break;
        }
        previous_band = band;
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Master_controller . Manage_Fortis_filters . FINISHED. Filter: 0x%02X \n",
        //       line_number++, GP_A);
    }
}

int QRP_mode() {
    int status = 0;
    static int safety_counter = 0;
    byte qrp_safety = 0;
    char message[PATH_MAX] = {0};
    static byte message_sent = FALSE;

    if (G_current_mode != QRP_MODE) {
        GP_A = QRP_MODE;
        G_current_mode = QRP_MODE;
        safety_counter = 0;
        G_TX_Inhibit = FALSE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . %s . called\n", line_number++, thread_name, __func__);
    }
    qrp_safety = digitalRead(POWER_MODE_PIN);
    if (qrp_safety != 0) {
        if (safety_counter++ > 700) {
            status = -1;
            G_TX_Inhibit = TRUE;
            if (message_sent == FALSE) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s .  %s .  qrp_safety: %d  \n", line_number++, thread_name, __func__, qrp_safety);
                strcpy(message, "WARNING SOLIDUS NOT IN QRP MODE");
                Gui_Add_Message(message);
                message_sent = TRUE;
            }
        }
    } else {
        safety_counter = 0;
        message_sent = FALSE;
        G_TX_Inhibit = FALSE;
        status = 0;
    }
    if (G_IQBD_Monitor_ON == TRUE) {
        GP_A = GP_A | IQBD_ON;
        IQBD_Set = TRUE;
    } else {
        GP_A = GP_A & ~IQBD_ON;
        IQBD_Set = FALSE;
    }
    status = Manage_PTT_Rear_Switch();
    return status;
}

int QRO_mode() {
    int status = 0;
    static byte swr_message_sent = FALSE;
    static byte high_power_message_sent = FALSE;
    static byte over_temp_message_sent = FALSE;

    if (G_current_mode != QRO_MODE) {
        GP_A = QRO_MODE;
        G_current_mode = QRO_MODE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Master_controller . QRO_mode. \n", line_number++);
    }
    if (G_TX == TRUE || G_BIAS_Mode == TRUE) {
        GP_A = GP_A | QRO_TRANSMIT;
        G_QRP_TX_ACTIVE = TRUE;
    } else {
        if (G_TX == FALSE && G_BIAS_Mode == FALSE) {
            GP_A = GP_A & ~QRO_TRANSMIT;
            G_QRP_TX_ACTIVE = TRUE;
        }
    }
    if (G_Solidus_Temperature >= OVER_TEMPERATURE) {
        if (over_temp_message_sent == FALSE) {
            if (G_PTT) {
                Set_PTT(0,FALSE);
            } else if (G_Tune) {
                Tune_button(0, FALSE);
            }
            SDRcore_recv_send_param(CMD_SET_TX_ON, 0);
            SDRcore_trans_send_param(CMD_SET_TX_ON, 0);
            Gui_send_param(CMD_SET_TX_SET_BY_SERVER, 0);
            G_TX_Inhibit = TRUE;
            over_temperature = TRUE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s - : %d\n", line_number++, thread_name, __func__, G_swr_shutdown);
            Gui_Add_Message("PA TEMPERATURE EXCEEDED. \nTX MODE HAS BEEN TURNED OFF UNTIL PA HAS RETURNED TO SAFE OPERATING TEMPERATURE");
            over_temp_message_sent = TRUE;
        }
    } else {
        over_temp_message_sent = FALSE;
        if (over_temperature == TRUE && (G_Solidus_Temperature < (OVER_TEMPERATURE - 10))) {
            G_TX_Inhibit = FALSE;
            over_temperature = FALSE;
            Gui_Add_Message("PA TEMPERATURE NOW IN OPERATING RANGE.\nTX MODE HAS BEEN RESTORED");
        }
    }
    if (G_swr_shutdown == TRUE) {
        if (swr_message_sent == FALSE) {
            //SetModeRxTx(0, 0);
            if (G_PTT) {
                Set_PTT(0,FALSE);
            } else if (G_Tune) {
                Tune_button(0, FALSE);
            }
            SDRcore_recv_send_param(CMD_SET_TX_ON, 0);
            SDRcore_trans_send_param(CMD_SET_TX_ON, 0);
            G_TX_Inhibit = TRUE;
            Gui_send_param(CMD_SET_TX_SET_BY_SERVER, 0);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . G_swr_shutdown: %d\n", line_number++, thread_name, __func__,
                    G_swr_shutdown);
            Gui_Add_Message("SWR FAULT: OVER LIMIT\nCORRECT THE FAULT AND TOGGLE QRP/QRO TO RESTORE POWER");
            swr_message_sent = TRUE;
        }
    } else {
        swr_message_sent = FALSE;
        G_TX_Inhibit = FALSE;
    }
    if (G_high_power_flag == TRUE && G_High_Power_Count >= HIGH_POWER_COUNT_LIMIT) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . %s . G_high_power_flag: %d\n", line_number++, thread_name, __func__,
                G_high_power_flag);
        if (high_power_message_sent == FALSE) {
            Gui_Add_Message("POWER OUTPUT OVER LIMIT:  REDUCE POWER");
            high_power_message_sent = TRUE;
        }
    } else {
        high_power_message_sent = FALSE;
    }
    status = Manage_PTT_Rear_Switch();
    return status;
}

/*int Process_meter() {
    static int state = TRUE;
    static int pass_count = 0;
    int16_t band = FREQ_OUT_OF_RANGE;

    if (G_METER_attached) {
        if (G_check_bias == FALSE) {
            if (G_band_normal == FREQ_OUT_OF_RANGE) {
                band = G_Amplifier_band;
            } else {
                band = G_band_normal;
            }
            if (band != 180 && G_TX == TRUE && G_SetModeRxTX == TRUE && G_IQBD_Monitor_ON == FALSE) {
                if (G_Meter_allow == FALSE && pass_count++ == 0) {
                    G_Meter_allow = TRUE;
                    state = FALSE;
                } else {
                    state = TRUE;
                    pass_count = 0;
                }
            }
        }
    }
    return state;
}*/

/*int Process_temperature() {
    static int state = TRUE;
    static int pass_count = 0;

    if (G_SOLIDUS_TEMP_SENSOR_attached) {
        if (G_TEMP_allow == FALSE && pass_count++ == 0) {
            G_TEMP_allow = TRUE;
            state == FALSE;
        } else {
            state = TRUE;
            pass_count = 0;
        }
    }
    return state;
}*/

/*int Process_bias() {
    static int state = TRUE;
    static int pass_count = 0;

    if (G_CURRENT_SENSOR_attached) {
        if (G_BIAS_allow == FALSE && pass_count++ == 0) {
            G_BIAS_allow = TRUE;
            state == FALSE;
        } else {
            state = TRUE;
            pass_count = 0;
        }
    }
    return state;
}*/

/*int Process_iqbd() {
    static int state = TRUE;

    if (G_IQBD_attached) {
        if (G_IQBD_Monitor_ON == TRUE && IQBD_Set == TRUE) {
            state == FALSE;
        } else {
            state = TRUE;
        }
    }
    return state;
}*/

/*int Process_MFC() {
    static int state = TRUE;
    static int pass_count = 0;

    if (G_MFC_attached) {
        if (G_MFC_allow == FALSE && pass_count++ == 0) {
            G_MFC_allow = TRUE;
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Process_MFC . %s .  G_MFC_allow: %d\n", line_number++, __func__, G_MFC_allow);
            state == FALSE;
        } else {
            state = TRUE;
            pass_count = 0;
        }
    }
    return state;
}*/

/*int Process_Display() {
    int state = TRUE;
    static int pass_count = 0;

    if (G_Display_attached) {
        if (G_Display_allow == FALSE && pass_count++ == 0) {
            G_Display_allow = TRUE;
            state == FALSE;
        } else {
            state = TRUE;
            pass_count = 0;
        }
    }
    return state;
}*/

/*int Check_TX_Relay() {
    int state = FALSE;
    int tx_relay_sense = 100;

    tx_relay_sense = digitalRead(TX_MODE_PIN);
    if (tx_relay_sense == 1) {
        state = TRUE;
    } else {
        state = FALSE;
    }
    return state;
}*/

/*long Get_micro_sec() {
    struct timeval tv;
    long micro_seconds = 0;

    gettimeofday(&tv, NULL);
    micro_seconds = 1000000 * tv.tv_sec + tv.tv_usec;
    return micro_seconds;
}*/

/*long micro_sec_key_1() {
    struct timespec tv;
    long micro_seconds = 0;
    long temp = 0;

    clock_gettime(CLOCK_MONOTONIC, &tv);
    temp = tv.tv_nsec / 1000;
    micro_seconds = (1000000 * tv.tv_sec) + temp;
    return micro_seconds;
}*/

/*int TX_Relay_Switched() {
    int status = TRUE;

    if (G_TX == TRUE && G_Solidus_TX_Relay_ON == FALSE) {
        status = FALSE;
    } else if (G_TX == FALSE && G_Solidus_TX_Relay_ON == TRUE) {
        status = FALSE;
    }
    return status;
}*/

/*void *Solidus_Check_TX_Relay(void *param) {
    int sleep_time = 300;
    int tx_relay_state = 0;
    int tx_relay_toggle = 0;
    int rx_relay_toggle = 0;
    int tx_delay_count = 0;
    int rx_delay_count = 0;
    int tx_report = 0;
    int rx_report = 0;

    while (G_all_threads_run) {
        if (G_TX == TRUE && G_current_mode == QRO_MODE && tx_relay_toggle == 0) {
            tx_relay_state = Check_TX_Relay();
            if (tx_relay_state == 1) {
                G_Solidus_TX_Relay_ON = TRUE;
                if (tx_delay_count > 0) {
                    tx_report = 1;
                }
                tx_relay_toggle = 1;
                rx_relay_toggle = 0;
            } else {
                ++tx_delay_count;
            }
        } else {
            if (G_TX == FALSE && G_current_mode == QRO_MODE && rx_relay_toggle == 0) {
                tx_relay_state = Check_TX_Relay();
                if (tx_relay_state == 0) {
                    G_Solidus_TX_Relay_ON = FALSE;
                    if (rx_delay_count > 0) {
                        rx_report = 1;
                    }
                    tx_relay_toggle = 0;
                    rx_relay_toggle = 1;
                } else {
                    ++rx_delay_count;
                }

            }
        }
        if (tx_report == 1) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. G_TX: %d, tx_delay_count: %d, delay_time: %d, rx_relay_toggle: %d, tx_relay_state: %d \n",
                    line_number++, __func__, G_TX, ++tx_delay_count, (tx_delay_count * sleep_time), rx_relay_toggle,
                    tx_relay_state);
            tx_report = 0;
            tx_delay_count = 0;
            rx_delay_count = 0;
        }
        if (rx_report == 1) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. G_TX: %d, rx_delay_count: %d, delay_time: %d, rx_relay_toggle: %d, tx_relay_state: %d \n",
                    line_number++, __func__, G_TX, ++rx_delay_count, (rx_delay_count * sleep_time), rx_relay_toggle, tx_relay_state);
            rx_report = 0;
            tx_delay_count = 0;
            rx_delay_count = 0;
        }
        delayMicroseconds(sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT  \n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}*/

void *Master_controller(void *param) {
    int status = 0;
    int MCP23017_status = 0;
    int QRP_status = 0;
    int QRO_status = 0;
    static int master_state = 0;
    int ptt_status = 0;
    int sleep_time = 107;
    int relay_master = 0;
    long t = 0;
    int token_status = 0;
    long long previous_lock_failed = 0;
    long long lock_failed_diff = 0;
    long long previous_lock_failed_diff = 0;
    long long lock_failed_limit = 350;
    long long lock_failed = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Master_controller Started. \n", line_number++);
    thread_name = __func__;

    digitalWrite(CW_KEY_OUT_1B, HIGH);
    status = MCP_Init();
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s->  MCP23017_Init FAILED \n", line_number++, thread_name);
        Gui_Add_Message("MASTER CONTROLLER INITIALIZATION FAILED");
    } else {
        G_subsys_initialized = G_subsys_initialized | MASTER_CONTROLLER_INITIALIZED;
    }
    while (G_all_threads_run && MCP23017_status >= 0) {
        token_status = pthread_mutex_trylock(&Master_Device_Token_available);
        if (token_status == 0) {
            switch (master_state) {
                case 0:
                    if (G_Transceiver_type == SOLIDUS) {
                        if (!G_QRP) {
                            QRP_status = QRP_mode();
                        }
                    } else {
                        ptt_status = Manage_PTT_Rear_Switch();
                    }
                    master_state++;
                    break;
                case 1:
                    if (G_Transceiver_type == SOLIDUS) {
                        if (G_QRP) {
                            QRO_status = QRO_mode();
                        }
                    } else {
                        ptt_status = Manage_PTT_Rear_Switch();
                    }
                    master_state++;
                    break;
                case 2:
                    switch (G_Transceiver_type) {
                        case SOLIDUS:
                            MCP23017_status = Manage_MCP23017();
                            break;
                        case FORTIS:
                            MCP23017_status = Manage_MCP23008();
                            break;
                    }
                    master_state++;
                    break;
                case 3:
                    switch (G_Transceiver_type) {
                        case SOLIDUS:
                            Manage_Solidus_filters();
                            break;
                        case FORTIS:
                            Manage_Fortis_filters();
                            break;
                    }
                    master_state++;
                    break;
                case 4:
                    switch (G_Transceiver_type) {
                        case SOLIDUS:
                            Manage_Solidus_antenna_switch();
                            break;
                        case FORTIS:
                            Manage_Fortis_antenna_switch();
                            break;
                    }
                    master_state++;
                    break;
                case 5:
                    Manage_antenna_sense();
                    master_state = 0;
                    break;
            }
            pthread_mutex_unlock(&Master_Device_Token_available);
        } else {
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
    if (MCP23017_status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . Abnormal Exit .   Manage_MCP23017 FAILED. \n", line_number++, __func__);
        Gui_Add_Message("MASTER CONTROLLER FAILED");
        Stop_all(1, STOP_MASTER_CONTROLLER);
    } else {
        GP_A = QRP_MODE | FAN_OFF;
        switch (G_Transceiver_type) {
            case SOLIDUS:
                Manage_MCP23017();
                break;
            case FORTIS:
                Manage_MCP23008();
                break;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT  \n", line_number++, __func__);
    }
    pthread_exit(0);
    return NULL;
}