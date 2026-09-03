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
// 10/01/2016 Added support for Proficio
// 02/18/2017 Added support for native CW
// Copyright © 2015-2017 Omnia SDR

// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  ff_<variable> -> Externally defined global stored in flash memory
//                                  ee_<variable> -> Externally defined global to be stored in EEPROM memory
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
//
// A very very simple Si5351a demonstration
// using the Si5351a module kit http://www.hanssummers.com/synth
// Please also refer to SiLabs AN619 which describes all the registers to use
//
//#include <inttypes.h>
#include <basic-plus.h>
#include "si5351.h"
#include "si5351a.h"
#include "MATH.H"


//#define TEMPERATURE_DRIFT_PPM   0.1925


//
// Set up specified PLL with mult, num and denom
// mult is 15..90
// num is 0..1,048,575 (0xFFFFF)
// denom is 0..1,048,575 (0xFFFFF)
//
uint8 setupPLL_CW(uint8_t pll, uint8_t mult, uint32 num, uint32 denom)
{
	static uint32 P1;					// PLL config register P1
	static uint32 P2;					// PLL config register P2
	static uint32 P3;					// PLL config register P3
    static uint8 state = 0;
    
            P1 = (uint32)(128 * ((float)num / (float)denom));
	        P1 = (uint32)(128 * (uint32)(mult) + P1 - 512);
	        P2 = (uint32)(128 * ((float)num / (float)denom));
	        P2 = (uint32)(128 * num - denom * P2);
	        P3 = denom;
            si5351_write_init(pll + 0, (P3 & 0x0000FF00) >> 8);
	        si5351_write_init(pll + 1, (P3 & 0x000000FF));
            //si5351_write_queue(pll + 1, (P3 & 0x000000FF));
	        si5351_write_init(pll + 2, (P1 & 0x00030000) >> 16);
            //si5351_write_queue(pll + 2, (P1 & 0x00030000) >> 16);
            state++;
	        si5351_write_init(pll + 3, (P1 & 0x0000FF00) >> 8);
            //si5351_write_queue(pll + 3, (P1 & 0x0000FF00) >> 8);
	        si5351_write_init(pll + 4, (P1 & 0x000000FF));
            //si5351_write_queue(pll + 4, (P1 & 0x000000FF));
	        si5351_write_init(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
            //si5351_write_queue(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	        si5351_write_init(pll + 6, (P2 & 0x0000FF00) >> 8);
            //si5351_write_queue(pll + 6, (P2 & 0x0000FF00) >> 8);
	        si5351_write_init(pll + 7, (P2 & 0x000000FF));
            //si5351_write_queue(pll + 7, (P2 & 0x000000FF));
    return state;
}

//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it is a #define in si5351a.h
//
uint8 setupMultisynth_CW(uint8_t synth, uint32 divider, uint8_t rDiv)
{
	static uint32 P1;					// Synth config register P1
	static uint32 P2;					// Synth config register P2
	static uint32 P3;					// Synth config register P3
    static uint8 state = 0;

            P1 = 128 * divider - 512;
	        P2 = 0;							// P2 = 0, P3 = 1 forces an integer value for the divider
	        P3 = 1;
            si5351_write_init(synth + 0,   (P3 & 0x0000FF00) >> 8);
	        si5351_write_init(synth + 1,   (P3 & 0x000000FF));
            //si5351_write_queue(synth + 1,   (P3 & 0x000000FF));
	        si5351_write_init(synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
            //si5351_write_queue(synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
	        si5351_write_init(synth + 3,   (P1 & 0x0000FF00) >> 8);
            //si5351_write_queue(synth + 3,   (P1 & 0x0000FF00) >> 8);
	        si5351_write_init(synth + 4,   (P1 & 0x000000FF));
            //si5351_write_queue(synth + 4,   (P1 & 0x000000FF));
	        si5351_write_init(synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
            //si5351_write_queue(synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
            state++;   
	        si5351_write_init(synth + 6,   (P2 & 0x0000FF00) >> 8);
            //si5351_write_queue(synth + 6,   (P2 & 0x0000FF00) >> 8);
	        si5351_write_init(synth + 7,   (P2 & 0x000000FF));
            //si5351_write_queue(synth + 7,   (P2 & 0x000000FF));
    return state;
}

//
// Switches off Si5351a output
// Example: si5351aOutputOff(SI_CLK0_CONTROL);
// will switch off output CLK0
//
void si5351aOutputOff_CW(uint8_t clk)
{
	//si5351_write_queue(clk, 0x80);		// Refer to SiLabs AN619 to see bit values - 0x80 turns off the output stage
    si5351_write_init(clk, 0x80);
}

// 
// Set CLK0 output ON and to the specified frequency
// Frequency is in the range 1MHz to 150MHz
// Example: si5351aSetFrequency(10000000);
// will set output CLK0 to 10MHz
//
// This example sets up PLL A
// and MultiSynth 0
// and produces the output on CLK0
//
/* VCO bounds / hard blank for MS-hold soft-tune (same policy as si5351a.c) */
#ifndef SI5351_VCO_MIN
#define SI5351_VCO_MIN     600000000UL
#define SI5351_VCO_MAX     900000000UL
#define SI5351_VCO_CENTER  750000000UL
#define SI5351_MS_MIN      6UL
#define SI5351_MS_MAX      1800UL
#endif
#ifndef SI5351_CLK0_OFF
#define SI5351_CLK0_OFF    0x80u
#define SI5351_CLK0_ON     (0x4Fu | SI_CLK_SRC_PLL_A)
#endif

//void si5351aSetFrequency(uint32 frequency,int8_t ppm_int,int8_t ppm_dec, uint8_t smooth)
uint8 si5351aSetFrequency_CW(uint8_t transmit)
{
	static uint32 pllFreq;
	uint32 xtalFreq = SI5351_XTAL_FREQ;
	static uint32 l;
	static uint8_t mult;
	static uint32 num,denom,divider;
    static uint32 prev_ms_divider = 0; /* held Multisynth divider for soft path */
    //static uint32 freq_previous = 0;
    float delta_freq ,ppm, f;
    int32 delta_freq_int;
    static uint8 state = 0;
    uint8 pll_status;
    uint8 multi_status;
    static int8 l_ppm_int;
    static int8 l_ppm_dec;
    uint32 E_l_freq_CW = 0;
    uint8 try_soft;
    uint8 soft_ms_hold;
   
            if(!transmit){
                E_l_freq_CW = (E_current_LO_freq + E_current_rit_freq);
            }else{
                E_l_freq_CW = E_current_LO_freq + E_cw_pitch_freq;
            }
            //This applies both the main PPM adjustment and drift frequency and then calculates the frequency adjustment 
            l_ppm_int = ee_ppm_int;
            l_ppm_dec = ee_ppm_dec;
            ppm = (float)((float)l_ppm_dec / 100);
            ppm = ppm + (float) l_ppm_int;
            delta_freq = (float)E_l_freq_CW / 1000000; 
            delta_freq = ((delta_freq * ppm) * 4) * -1;
            delta_freq_int = (int32)delta_freq;
            //Frequencies must always multiples of 4
            //while((delta_freq_int%4) != 0){
            //    delta_freq_int++;
            //}
            if(delta_freq_int < 0){
                while((delta_freq_int%4) !=0){
                    delta_freq_int++;
                }
            }else {
                if(delta_freq_int > 0){
                    while((delta_freq_int%4) != 0){
                        delta_freq_int--;
                    }
                }
            }
            //Now set the frequency for the Si5351 which four (4) times the LO sent by the host
            E_l_freq_CW = E_l_freq_CW * 4;
            //Now add the PPM correction
            E_l_freq_CW = E_l_freq_CW + delta_freq_int;

            soft_ms_hold = 0;
            try_soft = 0;
            if (prev_ms_divider >= SI5351_MS_MIN && E_l_freq_CW > 0UL) {
                if (E_l_freq_CW <= (SI5351_VCO_MAX / prev_ms_divider)) {
                    pllFreq = prev_ms_divider * E_l_freq_CW;
                    if (pllFreq >= SI5351_VCO_MIN) {
                        mult = (uint8_t)(pllFreq / xtalFreq);
                        if (mult >= 15u && mult <= 90u) {
                            try_soft = 1;
                        }
                    }
                }
            }

            if (try_soft) {
                divider = prev_ms_divider;
                soft_ms_hold = 1;
                E_smooth = TRUE;
            } else {
                divider = SI5351_VCO_CENTER / E_l_freq_CW;
                if (divider < SI5351_MS_MIN) {
                    divider = SI5351_MS_MIN;
                }
                if (divider > SI5351_MS_MAX) {
                    divider = SI5351_MS_MAX;
                }
                if (divider % 2UL) {
                    divider--;
                }
                if (divider < SI5351_MS_MIN) {
                    divider = SI5351_MS_MIN;
                }
                pllFreq = divider * E_l_freq_CW;
                if (pllFreq > SI5351_VCO_MAX || pllFreq < SI5351_VCO_MIN) {
                    divider = SI5351_VCO_MAX / E_l_freq_CW;
                    if (divider % 2UL) {
                        divider--;
                    }
                    if (divider < SI5351_MS_MIN) {
                        divider = SI5351_MS_MIN;
                    }
                    pllFreq = divider * E_l_freq_CW;
                }
                soft_ms_hold = 0;
                E_smooth = FALSE;
            }

	        mult = (uint8_t)(pllFreq / xtalFreq);
	        l = pllFreq % xtalFreq;			// It has three parts:
	        f = l;							// mult is an integer that must be in the range 15..90
	        f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	        f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	        num = f;						// the actual multiplier is  mult + num / denom
	        denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575
            /* Hard only: blank CLK0 around MS rewrite + PLL_RESET (no CW timer/PTT). */
            if (!soft_ms_hold) {
                si5351_write_init(SI_CLK0_CONTROL, SI5351_CLK0_OFF);
            }
	        pll_status = setupPLL_CW(SI_SYNTH_PLL_A, mult, num, denom);
            // Set up PLL A with the calculated multiplication ratio
            if (!soft_ms_hold) {
                multi_status = setupMultisynth_CW(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
            }
            // Soft: Multisynth held — PLL fractional only, no PLL_RESET
	        //if(!smooth) si5351_write_queue(SI_PLL_RESET, 0xA0);	
            if(!E_smooth) {
                si5351_write_init(SI_PLL_RESET, 0xA0);	
                E_smooth = TRUE;
            }
            prev_ms_divider = divider;
            // Resets the PLL. This causes a glitch in the output. For small changes to 
			// the parameters, you don't need to reset the PLL, and there is no glitch
            //si5351_write_queue(SI_CLK0_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);
            si5351_write_init(SI_CLK0_CONTROL, SI5351_CLK0_ON);
            // Finally switch on the CLK0 output (0x4F)
		    // and set the MultiSynth0 input to be PLL A
    return state;
}

