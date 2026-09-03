// Copyright 2013 David Turnbull AE9RB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 09/30/2014 Additions to support low latency CW and Iambic funtionality  Ron Patton / W4MMP
// 01/01/2015 Added Semi Break-in Support Ron Patton / W4MMP
// 10/20/2016 Added Support for Omnia SDR Proficio
// Copyright © 2015-2016 Omnia SDR

// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  ff_<variable> -> Externally defined global stored in flash memory
//                                  ee_<variable> -> Externally defined global to be stored in EEPROM memory
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define

#include<basic-plus.h>
#include <usbvend.h>


//uint8 E_meter_band;
//uint8 volume_buffer[CYDEV_EEPROM_ROW_SIZE];
//uint8 E_volume_band = 0;
//uint8 E_volume_value = 0;


//Profico Real band        160  80  60  40  30  20  17  15  12  10  6 (Proficio does not support 6M)
//Proficio band number:     0    1   2   3   4   5   6   7   8   9  10
//uint8_t Band_Map[11][1] = {{10},{9},{8},{7},{6},{5},{4},{3},{2},{1},{0}}; //Maps Proficio band number to mWattmeter band number

uint8 Band_Main(void) {
    static uint32 i;
   
    if (i != E_current_LO_freq) {
        i = E_current_LO_freq;
        // Watch for special IQ reversal frequencies
        /*switch (i) {
            case 33333333: Audio_IQ_Channels = 0; // 33.333333 MHz
                break; 
            case 33444444: Audio_IQ_Channels = 1; // 33.444444 MHz
                break; 
            case 33555555: Audio_IQ_Channels = 2; // 33.555555 MHz
                break;
            case 33666666: Audio_IQ_Channels = 3; // 33.666666 MHz
        } //End of switch (i)
        */
        if (i > 55000000) T1_Band_Number = 12; // 55.000 MHz (This will not be set. Proficio doesn't go this high anyway)
        else if (i > 31000000) T1_Band_Number = 11; // 31.000 MHz (This will not be set. Proficio doesn't go this high anyway)
        else if (i > 26000000) T1_Band_Number = 10; // 26.000 MHz
        else if (i > 22000000) T1_Band_Number = 9; // 22.000 MHz
        else if (i > 19000000) T1_Band_Number = 8; // 19.000 MHz
        else if (i > 16000000) T1_Band_Number = 7; // 16.000 MHz
        else if (i > 12000000) T1_Band_Number = 6; // 12.000 MHz
        else if (i > 9000000) T1_Band_Number = 5; // 9.000 MHz
        else if (i > 6000000) T1_Band_Number = 4; // 6.000 MHz
        else if (i > 5000000) T1_Band_Number = 3; // 5.000 MHz
        else if (i > 3000000) T1_Band_Number = 2; // 3.000 MHz
        else if (i > 1500000) T1_Band_Number = 1; // 1.500 MHz
        else T1_Band_Number = 0;
                    
        // Set the correct LPF and BPF
         if  (i > 400000)  {
            Band_Control_Write(CONTROL_BAND_630);  // 630M Set filter - relay - Amplifier
        } else  {
            Band_Control_Write(CONTROL_BAND_2200);  //2200M filter - relay - Amplfier
        }
        //Inhibit transmitter as necessary
        //Allow TX only on the ham bands to keep the FCC happy
        //Also Set the Band Indicator and PCM3060 Output Volume
        TX_Inhibit = 1;//Default to disable TX
        if (i >= 120000 && i <= 140000) {
            E_band = BAND_2200M;
            TX_Inhibit = 0;
        }//2200M 
        else if (i >= 400000 && i <= 500000) {
            E_band = BAND_630M;
            TX_Inhibit = 0;
        }//630M
      
        //E_meter_band = Band_Map[E_band][0];
        
        //if(previous_left_offset != E_band_and_volume[E_band].left_offset){
        //    PCM3060_Adj_Output_Volume(output_volume);
          //  previous_left_offset = E_band_and_volume[E_band].left_offset;
        //}
        //if(previous_right_offset != E_band_and_volume[E_band].right_offset){
        //    PCM3060_Adj_Output_Volume(output_volume);
        //    previous_left_offset = E_band_and_volume[E_band].right_offset;
        //}
    }
    return(0);
}


