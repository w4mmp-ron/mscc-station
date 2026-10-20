/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "basic-plus.h"

#define TEMP_REFRESH 10000
#define TEMPERATURE_DRIFT_PPM   0.1925
#define STARTUP_DELAY 871000
#define AVERAGE_LIMIT 400

int32 E_temp = 0;
int32 E_delta_drift_int = 0;
uint8 E_temperature_processing = FALSE;
volatile int32 E_transceiver_temp = 0;

/*void average_temps(int32 temp){
    static int32 average_temp[AVERAGE_LIMIT] = {0};
    static uint16 count = 0;
    uint16 average_count = 0;
    int32 temp_delta = 0;
    int32 average = 0;
    
    if(E_PPM_needs_set >= E_PPM_NEEDS_SET_STEP_1){
        memset(average_temp,0,AVERAGE_LIMIT);
        E_delta_drift_int = 0;
        count = 0;
    }
    average_temp[count] = temp;
    for(average_count = 0;average_count < AVERAGE_LIMIT;average_count++){
        average = average + average_temp[average_count];
    }
    E_delta_drift_int = average / average_count;
    temp_delta = E_delta_drift_int;
    count++;
    if(count >= AVERAGE_LIMIT){
        count = 0;
    }
}

uint8 Apply_temp_delta(){
    static uint8 state = 0;
    static int16 previous_temperature = 0;
    static int16 delta_temp = 0;
    static float delta_drift_freq;
    int16 l_drift;
    static uint8 first_pass = 3;
    static int32 l_delta_drift;
    
    if(first_pass != 0){ // The manual says to throw out the first iteration. Do three for good measure
        state = 0;
        first_pass--;
        previous_temperature = E_temp;
        return state;
    }
    switch(state){
        case 0:
            //This determines the freq in Hz adjustment for temperature drift. 
            //It assumes a linear drift based on temperature    
            if(E_temp < previous_temperature){
                delta_temp = (previous_temperature - E_temp) * -1;
            }else {
                delta_temp = (E_temp - previous_temperature) ;
            }
            previous_temperature = E_temp;
            delta_drift_freq = (float)E_l_freq / 1000000;
            delta_drift_freq = delta_drift_freq * 
                                    (float)((TEMPERATURE_DRIFT_PPM * (float)delta_temp));
            //The frequency sent to the Si5351 must be four (4) times 
            //the LO received from the host.
            delta_drift_freq = delta_drift_freq * 4;
            l_delta_drift = (int32)delta_drift_freq;
            state++;
            break;
        case 1:
            //Frequencies must always multiples of 4 or zero
            //The temperature compensation seems to work best if 
            //the frequency set closer to zero verses away from zero
            if(l_delta_drift < 0){
                while((l_delta_drift%4) !=0){
                    l_delta_drift++;
                }
            }else {
                if(l_delta_drift > 0){
                    while((l_delta_drift%4) != 0){
                        l_delta_drift--;
                    }
                }
            }
            l_drift = l_delta_drift;
            state++;
            break;
        case 2:
            average_temps(l_delta_drift);
            state = 0;
    }
    return state;
}*/

void Check_temperature(){
    static uint8_t state = 0;
    //static uint8_t apply_state = 0;
    cystatus status = 0;
    static uint16 temp_refresh = TEMP_REFRESH;
    static int16 real_temp;
    static uint32 startup_delay = STARTUP_DELAY;
   
    if(startup_delay == 0){
        if(temp_refresh == 0){
            switch(state){
                case 0:
                    status = DieTemp_1_Start();
                    switch(status){
                        case CYRET_STARTED:
                            state++;
                            //E_temperature_processing = TRUE;
                            break;
                        case CYRET_TIMEOUT:
                            state = 0;
                            break;
                        case CYRET_LOCKED:
                            status = DieTemp_1_Query(&real_temp);
                            state = 0;
                            break;
                    }
                    break;
                case 1:
                    status = DieTemp_1_Query(&real_temp);
                    switch(status){
                        case CYRET_SUCCESS:
                            E_temp = real_temp;
                            state++;
                            break;
                        case CYRET_STARTED:
                            state = 1;
                            break;
                        case CYRET_TIMEOUT:
                            state = 0;
                            break;
                    }
                    break;
                case 2:
                    /*apply_state = Apply_temp_delta();
                    if(apply_state == 0){
                        E_transceiver_temp = (E_temp);
                        temp_refresh = TEMP_REFRESH;
                        state = 0;
                    }*/
                    E_transceiver_temp = (E_temp);
                    temp_refresh = TEMP_REFRESH;
                    state = 0;
                    break;
            }
        }else{
            temp_refresh--;
        }
    }else{
        startup_delay--;
    }
}

/* [] END OF FILE */
