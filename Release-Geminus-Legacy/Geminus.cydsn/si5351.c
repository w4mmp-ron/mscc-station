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

#include <STDLIB.H>
#include <STDIO.H>
#include<basic-plus.h>
#include <si5351.h>
#include <si5351a.h>
#include <usbvend.h>

#define SI_5351_WRITE 0
#define SI_5351_WRITE_BULK 1
#define SI_5351_READ 2

#define NO_CALIBRATION 127
#define TUNING_PPM 3500
#define MAX_FREQ_DELTA 1000000
#define GET_TEMP_TIMER 43000
#define AM_OFFSET 11860
#define SSB_OFFSET 12000

volatile uint32 Si570_Xtal, Si570_LO = STARTUP_LO;
uint32 Current_LO = STARTUP_LO;
// [0-1] for commands, [2-8] retain registers
uint8 Si570_Buf[8];
// A copy of the factory registers used for cfgsr calibration.
uint8 Si570_Factory[6];
// Emulate old technique of setting of frequency by reg writes
uint8 Si570_OLD[6];

struct Si5351Status dev_status;
struct Si5351IntStatus dev_int_status;
uint8_t queue_status = 0;
volatile uint32 E_freq_from_host;//Set by USB interrupt routine
volatile uint32 E_freq_to_host = 0;//Set by USB interrupt routine
volatile int8 E_calibration_int = 0;//Set by USB interrupt routine
volatile int8 E_calibration_dec = 0;//Set by USB interrupt routine
volatile uint32 E_current_LO_freq = 0;
volatile uint32 E_xtal_freq = SI5351_XTAL_FREQ;
//volatile uint8 E_Band = BAND_20M;
volatile int8 ee_ppm_int = 0;
volatile int8 ee_ppm_dec = 0;
int16 E_temperature_current = 42;
volatile int16 E_temp_at_calibration = 42;
volatile uint8 E_smooth;
volatile uint32 E_tune_freq = 0;
volatile uint32 E_CW_LO_freq;
volatile uint32 l_tune_freq;
volatile uint8 E_cw_pitch = 0;
volatile int8 E_ppm = 0;
int16 E_cw_pitch_freq = 0;

/*void freq_queue_add(uint32 command){
    uint8 l_interrupt_status;
    
    if(E_display_attached == TRUE){
        l_interrupt_status = CyEnterCriticalSection();
        if(E_freq_queue_front == (E_freq_queue_rear +1) %MAX_COMMAND_QUEUE) {
            ERROR ("6 ");
            }else{
                if(E_freq_queue_front == -1) {
                    E_freq_queue_front = E_freq_queue_rear = 0;
                }else{
                    E_freq_queue_rear = (E_freq_queue_rear + 1)%MAX_COMMAND_QUEUE;
                }
            E_freq_queue[E_freq_queue_rear] = command;
        }
        CyExitCriticalSection(l_interrupt_status);
    }
}*/

uint8 smooth_tuning(uint32 freq){
    static uint32 previous_freq = 0;
    uint8 smooth = TRUE;
    uint32 abs_freq;
    int32 freq_diff;
    volatile uint32 max_ppm_freq;
    
    freq_diff = (int32)(freq - previous_freq);   
    abs_freq = labs((freq_diff));
    if(abs_freq >= MAX_FREQ_DELTA){
        smooth = FALSE;
    } else {
        max_ppm_freq = freq / 1000000;
        max_ppm_freq *= TUNING_PPM;
        if (abs_freq > max_ppm_freq){
            smooth = FALSE;
        }
    }
    previous_freq = freq;
    return smooth;
}

void si5351_main(){
    static uint8 state = 0;
    static uint32 l_Si570_LO = 0;
    volatile uint32 freq_in_hz;
    static uint32 previous_lo_freq = 0;
    uint8 critical;
    static uint8 previous_E_cw_pitch = 0;
       
    switch (state){
        case 0:
            if(l_Si570_LO != Si570_LO){
                critical = CyEnterCriticalSection();
                l_Si570_LO = Si570_LO;
                CyExitCriticalSection(critical);
                switch(E_dll_version){
                    //This handles the ExtIO_Si570 version of the data sent by the host
                    case SI570_DLL:
                        freq_in_hz = convert_from_host(l_Si570_LO);
                        freq_in_hz = freq_in_hz / 4;
                        break;
                    //This handles the MSCC version of the data sent by the host
                    case SI5351_DLL:
                        freq_in_hz = swap32(l_Si570_LO);
                        break;
                }
                if(freq_in_hz > 100){
                    if(E_current_LO_freq != freq_in_hz){
                        E_current_LO_freq = freq_in_hz;
                        CW_LO_Freq = E_current_LO_freq;
                        E_smooth = smooth_tuning(E_current_LO_freq);
                    }
                }
            } 
            state++;
            break;
        case 1:
            if(previous_E_cw_pitch != E_cw_pitch){
                switch (E_cw_pitch){
                    case 0:
                        E_cw_pitch_freq = 400;
                        break;
                    case 1:
                        E_cw_pitch_freq = 600;
                        break;
                    case 2:
                        E_cw_pitch_freq = 800;
                        break;
                    case 3:
                        E_cw_pitch_freq = 1000;
                        break;
                }
                previous_E_cw_pitch = E_cw_pitch;
            }
            state++;
        case 2:   //LO change and TX on to happen before the E_tune_freq is set.
          if(previous_lo_freq != E_current_LO_freq){
                E_tune_freq = E_current_LO_freq;
                //first_tx_pass = TRUE;
                switch(E_host_mode){
                    case 'A':
                        E_tune_freq = E_tune_freq + AM_OFFSET - E_ppm;
                        break;
                    case 'U':
                        E_tune_freq = E_tune_freq + SSB_OFFSET - E_ppm;
                        break;
                    case 'L':
                        E_tune_freq = E_tune_freq + SSB_OFFSET - E_ppm;
                        break;
                    case 'C':
                        E_CW_LO_freq = E_tune_freq + SSB_OFFSET - E_cw_pitch_freq;
                        break;
                }
                previous_lo_freq = E_current_LO_freq;   
            }
            //state++;
            state = 0;
            break;
    }//End Switch (state)
}
     

/*
 * si5351_init(uint8_t xtal_load_c, uint32 ref_osc_freq)
 *
 * Setup communications to the Si5351 and set the crystal
 * load capacitance.
 *
 * xtal_load_c - Crystal load capacitance. Use the SI5351_CRYSTAL_LOAD_*PF
 * defines in the header file
 * ref_osc_freq - Crystal/reference oscillator frequency in 1 Hz increments.
 * Defaults to 25000000 if a 0 is used here.
 *
 */
int si5351_init(uint8_t xtal_load_c, uint32 ref_osc_freq)
{
	uint8_t ret = 0;
    
    //Fake Si570_Xtal so ExtIO_Si570 does not complain
    Si570_Xtal = swap32((uint32)(SI570_STARTUP_FREQ * 112 / (float)(56.0) * 0x01000000));		
	/* Set crystal load capacitance */
	ret = si5351_write_init(SI5351_CRYSTAL_LOAD, xtal_load_c);
    
    
    
    if(ret){
	    // Change the ref osc freq if different from default
	    if (ref_osc_freq != 0)
	    {
		    E_xtal_freq = ref_osc_freq;
	    }

	    // Initialize the CLK outputs according to flowchart in datasheet
	    // First, turn them off
	    ret = si5351_write_init(16, 0x80);
        
	    ret = si5351_write_init(17, 0x80);
      
	    ret = si5351_write_init(18, 0x80);
       
	    // Turn the clocks back on...
	    ret = si5351_write_init(16, 0x0c);
        
	    ret = si5351_write_init(17, 0x0c);
        
	    ret = si5351_write_init(18, 0x0c);
        
        si5351_drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA);
                
	    // Then reset the PLLs
	    si5351_pll_reset(SI5351_PLLA);
        
	    si5351_pll_reset(SI5351_PLLB);
       
        //Shutdown clock 1 and clock 2 outputs
        si5351_set_clock_disable(SI5351_CLK1,0);
        
        si5351_set_clock_disable(SI5351_CLK2,0);
        
        si5351_set_clock_pwr(SI5351_CLK1,0);
        
        si5351_set_clock_pwr(SI5351_CLK2,0);
        
        si5351_output_enable(SI5351_CLK1,0);
        
        si5351_output_enable(SI5351_CLK2,0);
        
        
       
    }
	return ret;
}

/*
 * si5351_set_freq(uint64_t freq, enum si5351_clock clk)
 *
 * Uses SI5351_PLL_FIXED (900 MHz) for PLLA.
 * All multisynths are assigned to PLLA using this function.
 * PLLA is set to 900 MHz.
 * Restricted to outputs from 1 to 150 MHz.
 * If you need frequencies outside that range, use set_pll()
 * and set_ms() to set the synth dividers manually.
 *
 * freq - Output frequency in Hz
 * clk - Clock output
 *   (use the si5351_clock enum)
 */
/*void si5351_set_freq(uint32 freq, enum si5351_clock clk)
{
	struct Si5351Frac pll_frac, ms_frac;
    int32 ee_ref_correction = 0;
  
	// Lower bounds check
	if(freq < SI5351_MULTISYNTH_MIN_FREQ)
	{
		freq = SI5351_MULTISYNTH_MIN_FREQ;
	}

	// Upper bounds check
	if(freq > SI5351_MULTISYNTH_DIVBY4_FREQ)
	{
		freq = SI5351_MULTISYNTH_DIVBY4_FREQ;
	}


	// Set the PLL
	pll_frac.a = (uint16_t)(SI5351_PLL_FIXED / E_xtal_freq);
	if(ee_ref_correction < 0)
	{
		pll_frac.b = (uint32)((pll_frac.a * (uint32)(ee_ref_correction * -1)) / 10);
	}
	else
	{
		pll_frac.b = 1000000UL - (uint32)((pll_frac.a * (uint32)(ee_ref_correction)) / 10);
		pll_frac.a--;
	}
	pll_frac.c = 1000000UL;
	si5351_set_pll(pll_frac, SI5351_PLLA);

	// Set the MS
	ms_frac.a = (uint16_t)(SI5351_PLL_FIXED / freq);
	ms_frac.b = (uint32)(((SI5351_PLL_FIXED % freq) * 1000000UL) / freq);
	ms_frac.c = 1000000UL;
	si5351_set_ms(clk, ms_frac, 0, SI5351_OUTPUT_CLK_DIV_1, 0);
}*/

/*
 * si5351_set_pll(struct Si5351Frac frac, enum si5351_pll target_pll)
 *
 * Set the specified PLL to a specific oscillation frequency by
 * using the Si5351Frac struct to specify the synth divider ratio.
 *
 * frac - PLL fractional divider values
 * target_pll - Which PLL to set
 *     (use the si5351_pll enum)
 */
/*void si5351_set_pll(struct Si5351Frac frac, enum si5351_pll target_pll)
{
	struct Si5351RegSet pll_reg;
    uint8_t params[20];
  uint8_t i = 0;
  uint8_t temp;

	// Calculate parameters
  pll_reg.p1 = 128 * frac.a + ((128 * frac.b) / frac.c) - 512;
  pll_reg.p2 = 128 * frac.b - frac.c * ((128 * frac.b) / frac.c);
  pll_reg.p3 = frac.c;

  // Derive the register values to write
  // Prepare an array for parameters to be written to
  

  // Registers 26-27 for PLLA
  temp = ((pll_reg.p3 >> 8) & 0xFF);
  params[i++] = temp;

  temp = (uint8_t)(pll_reg.p3 & 0xFF);
  params[i++] = temp;

  // Register 28 for PLLA
  temp = (uint8_t)((pll_reg.p1 >> 16) & 0x03);
  params[i++] = temp;

  // Registers 29-30 for PLLA
  temp = (uint8_t)((pll_reg.p1 >> 8) & 0xFF);
  params[i++] = temp;

  temp = (uint8_t)(pll_reg.p1 & 0xFF);
  params[i++] = temp;

  // Register 31 for PLLA
  temp = (uint8_t)((pll_reg.p3 >> 12) & 0xF0);
  temp += (uint8_t)((pll_reg.p2 >> 16) & 0x0F);
  params[i++] = temp;

  // Registers 32-33 for PLLA
  temp = (uint8_t)((pll_reg.p2 >> 8) & 0xFF);
  params[i++] = temp;

  temp = (uint8_t)(pll_reg.p2 & 0xFF);
  params[i++] = temp;

  // Write the parameters
  if(target_pll == SI5351_PLLA)
  {
    si5351_write_bulk(SI5351_PLLA_PARAMETERS, i, params);
  }
  else if(target_pll == SI5351_PLLB)
  {
    si5351_write_bulk(SI5351_PLLB_PARAMETERS, i, params);
  }
}*/

/*
 * si5351_set_ms(enum si5351_clock clk, struct Si5351Frac frac, uint8_t int_mode, uint8_t r_div, uint8_t div_by_4)
 *
 * Set the specified multisynth parameters.
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * frac - Synth fractional divider values
 * int_mode - Set integer mode
 *  Set to 1 to enable, 0 to disable
 * r_div - Desired r_div ratio
 * div_by_4 - Set Divide By 4 mode
 *   Set to 1 to enable, 0 to disable
 */
/*void si5351_set_ms(enum si5351_clock clk, struct Si5351Frac frac, uint8_t int_mode, uint8_t r_div, uint8_t div_by_4)
{
	struct Si5351RegSet ms_reg;
	uint8_t params[20];
	uint8_t i = 0;
 	uint8_t temp;
 	uint8_t reg_val;

	// Calculate parameters
	if (div_by_4 == 1)
	{
		ms_reg.p3 = 1;
		ms_reg.p2 = 0;
		ms_reg.p1 = 0;
	}
	else
	{
		ms_reg.p1 = 128 * frac.a + ((128 * frac.b) / frac.c) - 512;
		ms_reg.p2 = 128 * frac.b - frac.c * ((128 * frac.b) / frac.c);
		ms_reg.p3 = frac.c;
	}

	// Registers 42-43 for CLK0
	temp = (uint8_t)((ms_reg.p3 >> 8) & 0xFF);
	params[i++] = temp;

	temp = (uint8_t)(ms_reg.p3 & 0xFF);
	params[i++] = temp;

	// Register 44 for CLK0
	si5351_read((SI5351_CLK0_PARAMETERS + 2) + (clk * 8), &reg_val);
	reg_val &= ~(0x03);
	temp = reg_val | ((uint8_t)((ms_reg.p1 >> 16) & 0x03));
	params[i++] = temp;

	// Registers 45-46 for CLK0
	temp = (uint8_t)((ms_reg.p1 >> 8) & 0xFF);
	params[i++] = temp;

	temp = (uint8_t)(ms_reg.p1 & 0xFF);
	params[i++] = temp;

	// Register 47 for CLK0
	temp = (uint8_t)((ms_reg.p3 >> 12) & 0xF0);
	temp += (uint8_t)((ms_reg.p2 >> 16) & 0x0F);
	params[i++] = temp;

	// Registers 48-49 for CLK0
	temp = (uint8_t)((ms_reg.p2 >> 8) & 0xFF);
	params[i++] = temp;

	temp = (uint8_t)(ms_reg.p2 & 0xFF);
	params[i++] = temp;

	// Write the parameters
	switch(clk)
	{
		case SI5351_CLK0:
			si5351_write_bulk(SI5351_CLK0_PARAMETERS, i, params);
			break;
		case SI5351_CLK1:
			si5351_write_bulk(SI5351_CLK1_PARAMETERS, i, params);
			break;
		case SI5351_CLK2:
			si5351_write_bulk(SI5351_CLK2_PARAMETERS, i, params);
			break;
		case SI5351_CLK3:
			si5351_write_bulk(SI5351_CLK3_PARAMETERS, i, params);
			break;
		case SI5351_CLK4:
			si5351_write_bulk(SI5351_CLK4_PARAMETERS, i, params);
			break;
		case SI5351_CLK5:
			si5351_write_bulk(SI5351_CLK5_PARAMETERS, i, params);
			break;
		case SI5351_CLK6:
			si5351_write_bulk(SI5351_CLK6_PARAMETERS, i, params);
			break;
		case SI5351_CLK7:
			si5351_write_bulk(SI5351_CLK7_PARAMETERS, i, params);
			break;
	}

	si5351_set_int(clk, int_mode);
	si5351_set_ms_div(clk, r_div, div_by_4);
}*/

/*
 * si5351_output_enable(enum si5351_clock clk, uint8_t enable)
 *
 * Enable or disable a chosen clock
 * clk - Clock output
 *   (use the si5351_clock enum)
 * enable - Set to 1 to enable, 0 to disable
 */
void si5351_output_enable(enum si5351_clock clk, uint8_t enable)
{
	uint8_t reg_val;

	if(si5351_read(SI5351_OUTPUT_ENABLE_CTRL, &reg_val) != 0)
	{
		return;
	}

	if(enable == 1)
	{
		reg_val &= ~(1<<(uint8_t)clk);
	}
	else
	{
		reg_val |= (1<<(uint8_t)clk);
	}

	si5351_write_init(SI5351_OUTPUT_ENABLE_CTRL, reg_val);
    
}

/*
 * si5351_drive_strength(enum si5351_clock clk, enum si5351_drive drive)
 *
 * Sets the drive strength of the specified clock output
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * drive - Desired drive level
 *   (use the si5351_drive enum)
 */
void si5351_drive_strength(enum si5351_clock clk, enum si5351_drive drive)
{
	uint8_t reg_val;
	uint8_t mask = 0x03;

	if(si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val) != 0)
	{
		return;
	}

	switch(drive)
	{
	case SI5351_DRIVE_2MA:
		reg_val &= ~(mask);
		reg_val |= 0x00;
		break;
	case SI5351_DRIVE_4MA:
		reg_val &= ~(mask);
		reg_val |= 0x01;
		break;
	case SI5351_DRIVE_6MA:
		reg_val &= ~(mask);
		reg_val |= 0x02;
		break;
	case SI5351_DRIVE_8MA:
		reg_val &= ~(mask);
		reg_val |= 0x03;
		break;
	default:
		break;
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
   
}

/*
 * si5351_update_status(void)
 *
 * Call this to update the status structs, then access them
 * via the dev_status and dev_int_status global variables.
 *
 * See the header file for the struct definitions. These
 * correspond to the flag names for registers 0 and 1 in
 * the Si5351 datasheet.
 */
/*void si5351_update_status(void)
{
	si5351_update_sys_status(&dev_status);
	si5351_update_int_status(&dev_int_status);
}*/

/*
 * si5351_set_correction(int32_t corr)
 *
 * Use this to set the oscillator correction factor to
 * EEPROM. This value is a signed 32-bit integer of the
 * parts-per-10 million value that the actual oscillation
 * frequency deviates from the specified frequency.
 *
 * The frequency calibration is done as a one-time procedure.
 * Any desired test frequency within the normal range of the
 * Si5351 should be set, then the actual output frequency
 * should be measured as accurately as possible. The
 * difference between the measured and specified frequencies
 * should be calculated in Hertz, then multiplied by 10 in
 * order to get the parts-per-10 million value.
 *
 * Since the Si5351 itself has an intrinsic 0 PPM error, this
 * correction factor is good across the entire tuning range of
 * the Si5351. Once this calibration is done accurately, it
 * should not have to be done again for the same Si5351 and
 * crystal.
 */
/*void si5351_set_correction(int32 corr)
{
	//xtal_freq = (uint32)(xtal_freq + (corr * (xtal_freq / 10000000UL)));
	ee_ppm_int = (int16)corr;
}*/

/*
 * si5351_set_phase(enum si5351_clock clk, uint8_t phase)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * phase - 7-bit phase word
 *   (in units of VCO/4 period)
 *
 * Write the 7-bit phase register. This must be used
 * with a user-set PLL frequency so that the user can
 * calculate the proper tuning word based on the PLL period.
 */
/*void si5351_set_phase(enum si5351_clock clk, uint8_t phase)
{
	// Mask off the upper bit since it is reserved
	phase = phase & 0xEF;

	si5351_write_init(SI5351_CLK0_PHASE_OFFSET + (uint8_t)clk, phase);
   
}*/

/*
 * si5351_pll_reset(enum si5351_pll target_pll)
 *
 * target_pll - Which PLL to reset
 *     (use the si5351_pll enum)
 *
 * Apply a reset to the indicated PLL.
 */
void si5351_pll_reset(enum si5351_pll target_pll)
{
	if(target_pll == SI5351_PLLA)
 	{
    	si5351_write_init(SI5351_PLL_RESET, SI5351_PLL_RESET_A);
       
	}
	else if(target_pll == SI5351_PLLB)
	{
	    si5351_write_init(SI5351_PLL_RESET, SI5351_PLL_RESET_B);
        
	}
}

/*
 * si5351_set_ms_source(enum si5351_clock clk, enum si5351_pll pll)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * pll - Which PLL to use as the source
 *     (use the si5351_pll enum)
 *
 * Set the desired PLL source for a multisynth.
 */
/*void si5351_set_ms_source(enum si5351_clock clk, enum si5351_pll pll)
{
	uint8_t reg_val;

	si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val);

	if(pll == SI5351_PLLA)
	{
		reg_val &= ~(SI5351_CLK_PLL_SELECT);
	}
	else if(pll == SI5351_PLLB)
	{
		reg_val |= SI5351_CLK_PLL_SELECT;
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
    
}*/

/*
 * si5351_set_int(enum si5351_clock clk, uint8_t int_mode)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * enable - Set to 1 to enable, 0 to disable
 *
 * Set the indicated multisynth into integer mode.
 */
/*void si5351_set_int(enum si5351_clock clk, uint8_t enable)
{
	uint8_t reg_val;
	si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val);

	if(enable == 1)
	{
		reg_val |= (SI5351_CLK_INTEGER_MODE);
	}
	else
	{
		reg_val &= ~(SI5351_CLK_INTEGER_MODE);
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
    
}*/

/*
 * si5351_set_clock_pwr(enum si5351_clock clk, uint8_t pwr)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * pwr - Set to 1 to enable, 0 to disable
 *
 * Enable or disable power to a clock output (a power
 * saving feature).
 */
void si5351_set_clock_pwr(enum si5351_clock clk, uint8_t pwr)
{
	uint8_t reg_val;
	si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val);

	if(pwr == 1)
	{
		//reg_val &= 0b01111111;
        reg_val &= 0x7F;
	}
	else
	{
		//reg_val |= 0b10000000;
        reg_val |= 0x40;
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
 
}

/*
 * si5351_set_clock_invert(enum si5351_clock clk, uint8_t inv)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * inv - Set to 1 to enable, 0 to disable
 *
 * Enable to invert the clock output waveform.
 */
/*void si5351_set_clock_invert(enum si5351_clock clk, uint8_t inv)
{
	uint8_t reg_val;
	si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val);

	if(inv == 1)
	{
		reg_val |= (SI5351_CLK_INVERT);
	}
	else
	{
		reg_val &= ~(SI5351_CLK_INVERT);
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
   
}*/

/*
 * si5351_set_clock_source(enum si5351_clock clk, enum si5351_clock_source src)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * src - Which clock source to use for the multisynth
 *   (use the si5351_clock_source enum)
 *
 * Set the clock source for a multisynth (based on the options
 * presented for Registers 16-23 in the Silicon Labs AN619 document).
 * Choices are XTAL, CLKIN, MS0, or the multisynth associated with
 * the clock output.
 */
/*void si5351_set_clock_source(enum si5351_clock clk, enum si5351_clock_source src)
{
	uint8_t reg_val;
	si5351_read(SI5351_CLK0_CTRL + (uint8_t)clk, &reg_val);

	// Clear the bits first
	reg_val &= ~(SI5351_CLK_INPUT_MASK);

	switch(src)
	{
	case SI5351_CLK_SRC_XTAL:
		reg_val |= (SI5351_CLK_INPUT_XTAL);
		break;
	case SI5351_CLK_SRC_CLKIN:
		reg_val |= (SI5351_CLK_INPUT_CLKIN);
		break;
	case SI5351_CLK_SRC_MS0:
		if(clk == SI5351_CLK0)
		{
			return;
		}

		reg_val |= (SI5351_CLK_INPUT_MULTISYNTH_0_4);
		break;
	case SI5351_CLK_SRC_MS:
		reg_val |= (SI5351_CLK_INPUT_MULTISYNTH_N);
		break;
	default:
		return;
	}

	si5351_write_init(SI5351_CLK0_CTRL + (uint8_t)clk, reg_val);
    
}*/

/*
 * si5351_set_clock_disable(enum si5351_clock clk, enum si5351_clock_disable dis_state)
 *
 * clk - Clock output
 *   (use the si5351_clock enum)
 * dis_state - Desired state of the output upon disable
 *   (use the si5351_clock_disable enum)
 *
 * Set the state of the clock output when it is disabled. Per page 27
 * of AN619 (Registers 24 and 25), there are four possible values: low,
 * high, high impedance, and never disabled.
 */
void si5351_set_clock_disable(enum si5351_clock clk, enum si5351_clock_disable dis_state)
{
	uint8_t reg_val, reg = 0;

	if (clk >= SI5351_CLK0 && clk <= SI5351_CLK3)
	{
		reg = SI5351_CLK3_0_DISABLE_STATE;
	}
	else if (clk >= SI5351_CLK0 && clk <= SI5351_CLK3)
	{
		reg = SI5351_CLK7_4_DISABLE_STATE;
	}

	si5351_read(reg, &reg_val);

	if (clk >= SI5351_CLK0 && clk <= SI5351_CLK3)
	{
		reg_val &= ~(0x03 << (clk * 2));
		reg_val |= dis_state << (clk * 2);
	}
	else if (clk >= SI5351_CLK0 && clk <= SI5351_CLK3)
	{
		reg_val &= ~(0x03 << ((clk - 4) * 2));
		reg_val |= dis_state << ((clk - 4) * 2);
	}

	si5351_write_init(reg, reg_val);
    
}

/*
 * si5351_set_clock_fanout(enum si5351_clock_fanout fanout, uint8_t enable)
 *
 * fanout - Desired clock fanout
 *   (use the si5351_clock_fanout enum)
 * enable - Set to 1 to enable, 0 to disable
 *
 * Use this function to enable or disable the clock fanout options
 * for individual clock outputs. If you intend to output the XO or
 * CLKIN on the clock outputs, enable this first.
 *
 * By default, only the Multisynth fanout is enabled at startup.
 */
/*void si5351_set_clock_fanout(enum si5351_clock_fanout fanout, uint8_t enable)
{
	uint8_t reg_val;
	si5351_read(SI5351_FANOUT_ENABLE, &reg_val);

	switch(fanout)
	{
	case SI5351_FANOUT_CLKIN:
		if(enable)
		{
			reg_val |= SI5351_CLKIN_ENABLE;
		}
		else
		{
			reg_val &= ~(SI5351_CLKIN_ENABLE);
		}
		break;
	case SI5351_FANOUT_XO:
		if(enable)
		{
			reg_val |= SI5351_XTAL_ENABLE;
		}
		else
		{
			reg_val &= ~(SI5351_XTAL_ENABLE);
		}
		break;
	case SI5351_FANOUT_MS:
		if(enable)
		{
			reg_val |= SI5351_MULTISYNTH_ENABLE;
		}
		else
		{
			reg_val &= ~(SI5351_MULTISYNTH_ENABLE);
		}
		break;
	}

	si5351_write_init(SI5351_FANOUT_ENABLE, reg_val);
    
}*/

void si5351_update_sys_status(struct Si5351Status *status)
{
	uint8_t reg_val = 0;

	if(si5351_read(SI5351_DEVICE_STATUS, &reg_val) != 0)
	{
		return;
	}

	/* Parse the register */
	status->SYS_INIT = (reg_val >> 7) & 0x01;
	status->LOL_B = (reg_val >> 6) & 0x01;
	status->LOL_A = (reg_val >> 5) & 0x01;
	status->LOS = (reg_val >> 4) & 0x01;
	status->REVID = reg_val & 0x03;
}

void si5351_update_int_status(struct Si5351IntStatus *int_status)
{
	uint8_t reg_val = 0;

	if(si5351_read(SI5351_DEVICE_STATUS, &reg_val) != 0)
	{
		return;
	}

	/* Parse the register */
	int_status->SYS_INIT_STKY = (reg_val >> 7) & 0x01;
	int_status->LOL_B_STKY = (reg_val >> 6) & 0x01;
	int_status->LOL_A_STKY = (reg_val >> 5) & 0x01;
	int_status->LOS_STKY = (reg_val >> 4) & 0x01;
}

/*void si5351_set_ms_div(enum si5351_clock clk, uint8_t r_div, uint8_t div_by_4)
{
	uint8_t reg_val, reg_addr = 0;

	switch(clk)
	{
		case SI5351_CLK0:
			reg_addr = SI5351_CLK0_PARAMETERS + 2;
			break;
		case SI5351_CLK1:
			reg_addr = SI5351_CLK1_PARAMETERS + 2;
			break;
		case SI5351_CLK2:
			reg_addr = SI5351_CLK2_PARAMETERS + 2;
			break;
		case SI5351_CLK3:
			reg_addr = SI5351_CLK3_PARAMETERS + 2;
			break;
		case SI5351_CLK4:
			reg_addr = SI5351_CLK4_PARAMETERS + 2;
			break;
		case SI5351_CLK5:
			reg_addr = SI5351_CLK5_PARAMETERS + 2;
			break;
		case SI5351_CLK6:
			return;
		case SI5351_CLK7:
			return;
	}

	si5351_read(reg_addr, &reg_val);

	// Clear the relevant bits
	reg_val &= ~(0x7c);

	if(div_by_4 == 0)
	{
		reg_val &= ~(SI5351_OUTPUT_CLK_DIVBY4);
	}
	else
	{
		reg_val |= (SI5351_OUTPUT_CLK_DIVBY4);
	}

	reg_val |= (r_div << SI5351_OUTPUT_CLK_DIV_SHIFT);

	si5351_write_init(reg_addr, reg_val);
    
}*/


/*uint8_t si5351_write_bulk(uint8_t addr, uint8_t bytes, uint8_t *command_data)
{
	//uint8_t msg_buffer[bytes + 1];
    uint8_t msg_buffer[50];
	uint8_t ret_status = 0;
    uint8 write_status = 0;
	uint8 buffer_written;
   
    write_status = write_status;//Keep Compiler happy
	msg_buffer[0] = addr;
	memcpy(&msg_buffer[1],command_data,(size_t)bytes);
    write_status = I2C_MasterWriteBuf(SI5351_SLAVE_ADDRESS,msg_buffer,(bytes + 1),I2C_MODE_COMPLETE_XFER);
    while((I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT) == 0u){};
	buffer_written = I2C_MasterGetWriteBufSize();
    if(buffer_written != (bytes +1)){ret_status = 1;}
	return ret_status;
}*/


uint8_t si5351_write_init(uint8_t addr, uint8_t command_data)
{
	uint8_t msg_buffer[2];
	uint8_t ret_status = 1;
	uint8_t write_status = 0;
    uint8 buffer_written;
        
    write_status = write_status;//Keep Compiler Happy
	msg_buffer[0] = addr;
	msg_buffer[1] = command_data;

    
	write_status = I2C_DISPLAY_MasterWriteBuf(SI5351_SLAVE_ADDRESS,msg_buffer,2u,I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (2u)){ret_status = 0;}
    return ret_status;
}

uint8_t si5351_read(uint8_t addr, uint8_t *command_data)
{
	uint8_t ret_status = 0;
	uint8_t status = 0;
    uint8_t *l_addr;
       
    l_addr = &addr; //For debugging purposes.  Remove from production code
    status = I2C_DISPLAY_MasterStatus();
    if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
        status = I2C_DISPLAY_MasterWriteBuf(SI5351_SLAVE_ADDRESS,l_addr,1u,I2C_DISPLAY_MODE_COMPLETE_XFER);
        while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
        if(status == I2C_DISPLAY_MSTR_NO_ERROR){
            status = I2C_DISPLAY_MasterReadBuf(SI5351_SLAVE_ADDRESS,command_data,1u,I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_RD_CMPLT) == 0u){};
            if(status != I2C_DISPLAY_MSTR_NO_ERROR){ret_status = 1;}
        }
    }
    return ret_status;
}
/*void si5351_reset_queue(void){
    E_si5351_queue_front = -1;
    E_si5351_queue_rear = -1;
}

uint8 si5351_write_queue(uint8 addr,uint8 command_data){
    uint8 ret_status = TRUE;
    int8_t front;//For debugging 
    int8_t rear;//For debugging 
   
    
    front = E_si5351_queue_front;
    rear = E_si5351_queue_rear;
    if(E_si5351_queue_front == (E_si5351_queue_rear +1) %MAX_SI5351_QUEUE) {
        ERROR ("S I 5 3 5 1  Q U E U E   ");
    }else{
        if(E_si5351_queue_front == -1) {
            E_si5351_queue_front = E_si5351_queue_rear = 0;
        }else{
            E_si5351_queue_rear = (E_si5351_queue_rear + 1)%MAX_SI5351_QUEUE;
        }
        E_si5351_queue[E_si5351_queue_rear][0] = addr;
        E_si5351_queue[E_si5351_queue_rear][1] = command_data;
    }
    return ret_status;                
}

uint8_t si5351_get_command_from_queue(uint8 *addr,uint8 *my_data){
    uint8_t ret = TRUE;
    uint8_t l_addr;//For debugging
    uint8_t l_data;//For debugging
    if(E_si5351_queue_front == -1){
        ret = 0;
    }else{
        l_addr = E_si5351_queue[E_si5351_queue_front][0];
        l_data = E_si5351_queue[E_si5351_queue_front][1];
        *addr = E_si5351_queue[E_si5351_queue_front][0];
        *my_data = E_si5351_queue[E_si5351_queue_front][1];
        ret = 2;
        E_si5351_queue[E_si5351_queue_front][0] = 0;
        E_si5351_queue[E_si5351_queue_front][1] = 0;
        if(E_si5351_queue_front == E_si5351_queue_rear){
            E_si5351_queue_front = E_si5351_queue_rear = -1;
        }else{
            E_si5351_queue_front = (E_si5351_queue_front + 1)%MAX_SI5351_QUEUE;
        }
    }
    return ret;
}

int8_t si5351_process_queue()
{
    static uint8_t addr = 0,command_data = 0;
	//static uint8_t msg_buffer[2];
	static int8_t queue_status = 0;
	uint8_t i2c_write_status = 0;
    //uint8_t i2c_completion_status;
    static uint8 state = 0;
    //static uint8 write_counter = 0;
       
    switch(state){
        case 0:
            queue_status = si5351_get_command_from_queue(&addr,&command_data);
            if(queue_status != 0){
                //msg_buffer[0] = addr;
	            //msg_buffer[1] = command_data;
                state = 1;
            }
            break;
        case 1:
            i2c_write_status = si5351_write_init(addr,command_data);
            if(i2c_write_status){
                state = 2;
            }else{
                si5351_reset_queue();
                state = 0;
            }
            break;
        case 2:
            state = 0;
            queue_status = 0;
            break;
    }
    return queue_status;
}*/

/*int8_t si5351_process_queue()
{
    static uint8_t addr = 0,command_data = 0;
	static uint8_t msg_buffer[2];
	static int8_t queue_status = 0;
	uint8_t i2c_write_status = 0;
    uint8_t i2c_completion_status;
    static uint8 state = 0;
    static uint8 write_counter = 0;
       
    switch(state){
        case 0:
            queue_status = si5351_get_command_from_queue(&addr,&command_data);
            if(queue_status != 0){
                msg_buffer[0] = addr;
	            msg_buffer[1] = command_data;
                state = 1;
            }
            break;
        case 1:
            i2c_write_status = I2C_MasterWriteBuf(SI5351_SLAVE_ADDRESS,msg_buffer,2u,I2C_MODE_COMPLETE_XFER);
            if(i2c_write_status == I2C_MSTR_NO_ERROR){
                state = 2;
            }else {
                //Through away this iteration.  The I2C buss can't keep up
                I2C_Stop();
                I2C_Start();
                state = 0;
                queue_status = SI5351_RESET;
                si5351_reset_queue();
            }
        case 2:
            i2c_completion_status = I2C_MasterStatus();
            if (i2c_completion_status & I2C_MSTAT_WR_CMPLT) {
                state = 3;
            }
            else {
                if(i2c_completion_status & I2C_MSTAT_ERR_XFER){
                    if(write_counter++ > 3){
                        //Allow the I2C buss to retry 3 times.  If not, through away this iteration.
                        I2C_Stop();
                        I2C_Start();
                        write_counter = 0;
                        state = 0;
                        queue_status = SI5351_RESET;
                        si5351_reset_queue();
                    }
                }
            }
            break;
        case 3:
            state = 0;
            queue_status = 0;
            break;
    }
    return queue_status;
}*/



/* [] END OF FILE */
