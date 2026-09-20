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

int32 E_Potentia_Power = 0;
uint8_t E_potentia_power_sensor_attached = 0;

#define POTENTIA_POWER_ADDR 0x40 //Temperature sensor on the Potentia

uint8_t Potentia_Power_Init(void){
    uint8_t msg_buffer[3];
    uint8_t status = 1;
    uint8_t write_status;
    uint8_t buffer_written;
   
    msg_buffer[0] = 0x00;   //Configuration Register
    msg_buffer[1] = 0x1F;
    msg_buffer[2] = 0xFF;
    status = I2C_DISPLAY_MasterStatus();
    if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
        write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_POWER_ADDR,msg_buffer,3u,
            I2C_DISPLAY_MODE_COMPLETE_XFER);
        while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
        buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    }
    status = 1;
    if(buffer_written != (3u)){status = 0;}
    return status;
}

uint8_t Potentia_Read_Power(){
    uint8_t status = 0;
    static uint8_t state = 0;
    static uint8_t upper_byte;
    static uint8_t lower_byte;
    uint8_t write_status;
    uint8_t buffer_written;
    static uint8_t msg_buffer[2];
    int16 power = 0;
    int32 power_temp = 0;
    
    switch(state){
        case 0:
            msg_buffer[0] = 0x01;
            status = I2C_DISPLAY_MasterStatus();
            if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
                write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_POWER_ADDR,msg_buffer,1u,
                    I2C_DISPLAY_MODE_COMPLETE_XFER);
                while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
                buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
                if(write_status == I2C_DISPLAY_MSTR_NO_ERROR){
                    state = 1;
                }else{
                    state = 3;
                }
            }
            break;
        case 1:
            status = I2C_DISPLAY_MasterReadBuf(POTENTIA_POWER_ADDR,msg_buffer,2u,
                                    I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_RD_CMPLT) == 0u){};
            if(status == I2C_DISPLAY_MSTR_NO_ERROR){
                state = 2;
            }else{
                state = 3;
            }
            break;
        case 2:
            upper_byte = msg_buffer[0];
            lower_byte = msg_buffer[1];
            //upper_byte = upper_byte & 0x0F;
            power = upper_byte;
            power = power << 8;
            power = power + lower_byte;
            power_temp = power;
            E_Potentia_Power = 0;
            E_Potentia_Power = swap32(power_temp);
            state = 0;
            break;
        case 3:
            E_potentia_power_sensor_attached  = FALSE;
            E_Potentia_Power = 0;
            state = 0;
            break;
    }
    return state;
}
/* [] END OF FILE */
