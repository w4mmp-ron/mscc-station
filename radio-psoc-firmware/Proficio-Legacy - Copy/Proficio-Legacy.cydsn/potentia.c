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

int32 E_Potentia_temperature = 0;

#define POTENTIA_TEMP_ADDR 0x18 //Temperature sensor on the Potentia

uint8_t Potentia_Temp_Init(void){
    uint8_t msg_buffer[2];
    uint8_t status = 1;
    uint8_t write_status;
    uint8_t buffer_written;
   
    msg_buffer[0] = 0x05;   //IODIR Register
    status = I2C_DISPLAY_MasterStatus();
    if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
        write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_TEMP_ADDR,msg_buffer,1u,
            I2C_DISPLAY_MODE_COMPLETE_XFER);
        while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
        buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    }
    status = 1;
    if(buffer_written != (1u)){status = 0;}
    return status;
}

uint8_t Potentia_Read_Temp(){
    uint8_t status = 0;
    static uint8_t state = 0;
    static uint8_t upper_byte;
    static uint8_t lower_byte;
    uint8_t write_status;
    uint8_t buffer_written;
    static uint8_t msg_buffer[2];
    int32 temperature = 0;
    
    switch(state){
        case 0:
            msg_buffer[0] = 0x05;
            status = I2C_DISPLAY_MasterStatus();
            if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
                write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_TEMP_ADDR,msg_buffer,1u,
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
            status = I2C_DISPLAY_MasterReadBuf(POTENTIA_TEMP_ADDR,msg_buffer,2u,
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
            upper_byte = upper_byte & 0x0F;
            temperature = upper_byte;
            temperature = temperature << 8;
            temperature = temperature + lower_byte;
            E_Potentia_temperature = swap32(temperature);
            state = 0;
            break;
        case 3:
            E_potentia_attached  = FALSE;
            E_Potentia_temperature = 0;
            state = 0;
            break;
    }
    return state;
}
/* [] END OF FILE */
