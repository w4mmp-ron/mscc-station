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
#include <basic-plus.h>
#include <si5351.h>
#include <si5351a.h>

#define MIA_ADDR 0x21
#define MIA_BAND_DATA 0x10
#define MIA_MODE_DATA 0x20
#define MIA_SWR_DATA 0x30
#define MIA_OVERPOWER_DATA 0x40
#define SWR_LIMIT_VALUE 0x06
#define OVER_POWER_LIMIT_VALUE 0x0F

#define DELAY 1000
#define UPDATE_DELAY 200
#define SET_TRANSMIT_ON 15
#define SET_TRANSMIT_OFF 14
#define MIA_IO_OUTPUT 0x00
#define MIA_IO_INPUT 0xFF
#define MIA_READ_COUNT_LIMIT 3
#define MIA_TIMER_RESET 0x02
//#define AMP_TIMER 20
//#define BAND_REFRESH 600
//#define AMP_REFRESH 1000

uint8_t E_amp_power = 0;
uint8_t Mia_Read_Buffer;

uint8_t Mia_Init(void){
    uint8_t msg_buffer[2];
    uint8_t status = 1;
    uint8_t write_status;
   
    uint8_t buffer_written;
   
    msg_buffer[0] = 0x00;   //IODIR Register
    msg_buffer[1] = 0x00;   //Set IODIR to OUTPUT
    write_status = I2C_DISPLAY_MasterWriteBuf(MIA_ADDR,msg_buffer,2u,I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (2u)){status = 0;}
   
    msg_buffer[0] = 0x05;   //IOCON Register
    msg_buffer[1] = 0x20;   //Disable Sequencial Read
    write_status = I2C_DISPLAY_MasterWriteBuf(MIA_ADDR,msg_buffer,2u,I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (2u)){status = 0;}
   
    return status;
}

uint8 Mia_Send(uint8 mia_data,uint8 mia_data_type){
    uint8 status = 1;
    uint8 write_status = 0;
    uint8 buffer_written = 0;
    uint8 mia_data_temp = 0;
    uint8_t msg_buffer[2];
    
    mia_data = mia_data | mia_data_type;
    msg_buffer[0] = 0x09;
    msg_buffer[1] = mia_data;
   
    write_status = I2C_DISPLAY_MasterWriteBuf(MIA_ADDR,msg_buffer,2u,I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (2u)){status = 0;}
    
    if(status == 0){
        E_mia_attached = FALSE;
        I2C_DISPLAY_Stop();
        I2C_DISPLAY_Start();
        I2C_DISPLAY_MasterClearStatus();
    }
    return status;
    }

uint8_t Mia_delay(uint8_t start){
    uint8_t status = FALSE;
    static int16 delay_count = 0;
    
    if(start == TRUE){
        delay_count = DELAY;
    }
    if(--delay_count <= 0){
        status = TRUE;
    }
    return status;
}

uint8 MIA_Refresh(){
    static uint8 state = 0;
    static uint8 timer_status = 0;
    static uint8 previous_E_AMP_Bypass = 100;
    static uint8 previous_E_AMP_VALUE = 100;
    static uint8 previous_E_meter_band = 100;
    static uint8 mia_processing = FALSE;
    static uint8 update_delay = UPDATE_DELAY;
    static uint8 swr_limit = 0;
    static uint8 over_power_limit = 0;
    static uint8 refresh_target = 0;
        
    if(previous_E_AMP_Bypass != E_AMP_Bypass){
        switch(E_AMP_Bypass){
            case 0:
                E_AMP_Value = 11;
                break;
            case 1:
                E_AMP_Value = 12;
                break;
        }
        previous_E_AMP_Bypass = E_AMP_Bypass;
    }
    //mia_processing = FALSE;
    switch (state){
        case 0:
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE && mia_processing == FALSE ){
                if(previous_E_meter_band != E_meter_band){
                    Mia_Send(E_meter_band,MIA_BAND_DATA);
                    previous_E_meter_band = E_meter_band;
                    timer_status = Mia_delay(TRUE);
                    mia_processing = TRUE;
                    update_delay = UPDATE_DELAY;
                }
            }
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE){
                state = 1;
                mia_processing = FALSE;
            }
            break;
        
        case 1:
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE && mia_processing == FALSE ){
                if(previous_E_AMP_VALUE != E_AMP_Value){
                    Mia_Send(E_AMP_Value,MIA_BAND_DATA);
                    previous_E_AMP_VALUE = E_AMP_Value;
                    timer_status = Mia_delay(TRUE);
                    mia_processing = TRUE;
                    update_delay = UPDATE_DELAY;
                }
            }
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE){
                state = 2;
                mia_processing = FALSE;
            }
            break;
            
        case 2:
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE && mia_processing == FALSE ){
                if(swr_limit != SWR_LIMIT_VALUE){
                    swr_limit = SWR_LIMIT_VALUE;
                    Mia_Send(swr_limit,MIA_SWR_DATA);   
                    timer_status = Mia_delay(TRUE);
                    mia_processing = TRUE;
                    update_delay = UPDATE_DELAY;
                }
            }
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE){
                state = 3;
                mia_processing = FALSE;
            }
            break;
        case 3:
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE && mia_processing == FALSE ){
                if(over_power_limit != OVER_POWER_LIMIT_VALUE){
                    over_power_limit = OVER_POWER_LIMIT_VALUE;
                    Mia_Send(over_power_limit,MIA_OVERPOWER_DATA);   
                    timer_status = Mia_delay(TRUE);
                    mia_processing = TRUE;
                    update_delay = UPDATE_DELAY;
                }
            }
            timer_status = Mia_delay(FALSE);
            if(timer_status == TRUE){
                state = 4;
                mia_processing = FALSE;
            }
            break;
        case 4:
            if(mia_processing == FALSE){
                if(--update_delay == 0){
                    switch(refresh_target){
                        case 0:
                            previous_E_AMP_VALUE = 100;
                            refresh_target++;
                            break;
                        case 1:
                            previous_E_meter_band = 100;
                            refresh_target++;
                            break;
                        case 2:
                            swr_limit = 100;
                            refresh_target++;
                            break;
                        case 3:
                            over_power_limit = 100;
                            refresh_target = 0;
                    }
                    update_delay = UPDATE_DELAY;
                }
            }
            state = 0;
            break;
    }
    return mia_processing;
}
/* [] END OF FILE */
