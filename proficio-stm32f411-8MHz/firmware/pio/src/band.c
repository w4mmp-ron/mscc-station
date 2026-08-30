/**
 * Band LPF selection + TX inhibit windows — port of band.c
 */
#include "band.h"
#include "control.h"
#include "radio_state.h"

uint8_t Band_Main(void)
{
    static uint32_t i = 0;

    if (i != E_current_LO_freq) {
        i = E_current_LO_freq;

        if (i > 24000000UL) {
            Band_Control_Write(CONTROL_BAND_10_12);
        } else if (i > 15000000UL) {
            Band_Control_Write(CONTROL_BAND_15_17);
        } else if (i > 9000000UL) {
            Band_Control_Write(CONTROL_BAND_20_30);
        } else if (i > 4600000UL) {
            Band_Control_Write(CONTROL_BAND_40_60);
        } else if (i > 2800000UL) {
            Band_Control_Write(CONTROL_BAND_80);
        } else {
            Band_Control_Write(CONTROL_BAND_160);
        }

        TX_Inhibit = 1;
        if (i >= 1780000UL && i <= 2010000UL) {
            E_band = BAND_160M;
            TX_Inhibit = 0;
        } else if (i >= 3480000UL && i <= 4010000UL) {
            E_band = BAND_80M;
            TX_Inhibit = 0;
        } else if (i >= 5310500UL && i <= 5413500UL) {
            E_band = BAND_60M;
            TX_Inhibit = 0;
        } else if (i >= 6980000UL && i <= 7310000UL) {
            E_band = BAND_40M;
            TX_Inhibit = 0;
        } else if (i >= 10080000UL && i <= 10160000UL) {
            E_band = BAND_30M;
            TX_Inhibit = 0;
        } else if (i >= 18048000UL && i <= 18170000UL) {
            E_band = BAND_17M;
            TX_Inhibit = 0;
        } else if (i >= 13980000UL && i <= 14360000UL) {
            E_band = BAND_20M;
            TX_Inhibit = 0;
        } else if (i >= 20980000UL && i <= 21460000UL) {
            E_band = BAND_15M;
            TX_Inhibit = 0;
        } else if (i >= 24870000UL && i <= 25000000UL) {
            E_band = BAND_12M;
            TX_Inhibit = 0;
        } else if (i >= 27980000UL && i <= 29710000UL) {
            E_band = BAND_10M;
            TX_Inhibit = 0;
        }
        if (i >= 27980000UL && E_transverter) {
            E_band = BAND_10M;
            TX_Inhibit = 0;
        }
    }
    return 0;
}
