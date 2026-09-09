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

uint8_t E_Potentia_Read_Bias = 0;
uint16_t E_Potentia_Write_Bias = 0;
uint8_t E_potentia_Bias_Sensor_Attached = 0;

#define POTENTIA_BIAS_ADDR 0x2F 

uint8_t Potentia_Bias_Init(void){
    uint8_t msg_buffer[3];
    uint8_t status = 1;
    uint8_t write_status;
    uint8_t buffer_written = 0;
   
    msg_buffer[0] = 0x00;   //Configuration Register
    status = I2C_DISPLAY_MasterStatus();
    if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
        write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_BIAS_ADDR,msg_buffer,1u,
            I2C_DISPLAY_MODE_COMPLETE_XFER);
        while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
        buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    }
    status = 1;
    if(buffer_written != (1u)){status = 0;}
    return status;
}

uint8_t Potentia_Write_Bias(void){
    uint8_t status = 0;
    static uint8_t state = 0;
    static uint8_t msg_buffer[2];
    static uint16_t previous_bias_value = 0;
    uint8_t write_status;
    uint8_t buffer_written;
    uint16_t bias_swapped = 0;
    //static uint16_t wiper_0 = 0;
    //static uint16_t wiper_1 = 0;
    
    if(previous_bias_value != E_Potentia_Write_Bias){
        bias_swapped = swap16(E_Potentia_Write_Bias);
        msg_buffer[0] = bias_swapped >> 8;
        if(msg_buffer[0] == 0){
            //wiper_0 = bias_swapped;
            msg_buffer[1] = (uint8_t)(bias_swapped);
        }else{
            msg_buffer[0] = 0x80;
            msg_buffer[1] = (uint8_t)(bias_swapped);
            //wiper_1 = msg_buffer[0];
            //wiper_1 = wiper_1 << 8;
            //wiper_1 = wiper_1 + msg_buffer[1];
        }
        switch(state){
            case 0:
                status = I2C_DISPLAY_MasterStatus();
                if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
                    write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_BIAS_ADDR,msg_buffer,2u,
                        I2C_DISPLAY_MODE_COMPLETE_XFER);
                    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
                    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
                    if(write_status == I2C_DISPLAY_MSTR_NO_ERROR){
                        if(buffer_written == 2u){
                            state = 0;
                            previous_bias_value = E_Potentia_Write_Bias;
                        }else{
                            state = 1;
                        }
                    }
                }
                break;
            case 1:
                E_potentia_Bias_Sensor_Attached  = FALSE;
                E_Potentia_Write_Bias = 0;
                state = 0;
                break;
        }
    }
    return state;
}

uint8_t Potentia_Read_Bias(void){
    //uint8_t status = 0;
    static uint8_t state = 0;
    //static uint8_t upper_byte;
    //static uint8_t lower_byte;
    //uint8_t write_status;
    //uint8_t buffer_written;
    //static uint8_t msg_buffer[2];
    //int16 bias = 0;
    //int32 bias_temp = 0;
    
    /*switch(state){
        case 0:
            msg_buffer[0] = 0x01;
            status = I2C_DISPLAY_MasterStatus();
            if(status != I2C_DISPLAY_MSTAT_ERR_XFER){
                write_status = I2C_DISPLAY_MasterWriteBuf(POTENTIA_BIAS_ADDR,msg_buffer,1u,
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
            status = I2C_DISPLAY_MasterReadBuf(POTENTIA_BIAS_ADDR,msg_buffer,2u,
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
            bias = upper_byte;
            bias = bias << 8;
            bias = bias + lower_byte;
            bias_temp = bias_temp;
            E_Potentia_Read_Bias = 0;
            E_Potentia_Read_Bias = swap32(bias_temp);
            state = 0;
            break;
        case 3:
            E_potentia_Bias_Sensor_Attached  = FALSE;
            E_Potentia_Read_Bias = 0;
            state = 0;
            break;
    }*/
    return state;
}
/* [] END OF FILE */
