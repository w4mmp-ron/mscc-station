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
#include "cytypes.h"
#include <basic-plus.h>

#define PCM3060_I2C_ADDR 0x46
#define USB_AUDIO_BUFS 3
//extern struct band_volume E_band_and_volume[10];

volatile uint8 RxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE], RxI2S_Stage;
volatile uint8 TxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE], TxI2S_Stage, TxI2S_Zero = 0;

uint8 RxI2S_Stage_TD[3], RxI2S_Buff_TD[USB_AUDIO_BUFS];

void DmaRxInit() {
    uint8 i;
    RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 3; i++) RxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) RxI2S_Buff_TD[i]=CyDmaTdAllocate();
}

void DmaRxStart(void) {
    uint8 i, n;

    for (i=0; i < 3; i++) {
        if (i==2) {
            CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[0], 0 );
        } else {
            CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[i+1], RxI2S_Stage__TD_TERMOUT_EN );
        }
        CyDmaTdSetAddress(RxI2S_Stage_TD[i], LO16((uint32)I2S_RX_FIFO_0_PTR), LO16((uint32)&RxI2S_Stage));
    }
    CyDmaClearPendingDrq(RxI2S_Stage_DmaHandle);
    CyDmaChSetInitialTd(RxI2S_Stage_DmaHandle, RxI2S_Stage_TD[0]);

    for (i=0; i < USB_AUDIO_BUFS; i++) {
        n = i + 1;
        if (n >= USB_AUDIO_BUFS) n=0;
        CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE, RxI2S_Buff_TD[n], TD_INC_DST_ADR);
        CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16((uint32)&RxI2S_Stage), LO16((uint32)RxI2S[i]));
    }
    CyDmaClearPendingDrq(RxI2S_Buff_DmaHandle);
    CyDmaChSetInitialTd(RxI2S_Buff_DmaHandle, RxI2S_Buff_TD[0]);

    CyDmaChEnable(RxI2S_Buff_DmaHandle, 1u);
    CyDmaChEnable(RxI2S_Stage_DmaHandle, 1u);
}


uint8 TxI2S_Stage_TD[3], TxI2S_Buff_TD[USB_AUDIO_BUFS], TxI2S_Zero_TD[USB_AUDIO_BUFS];

void DmaTxInit() {
    uint8 i;
    TxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    TxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    TxI2S_Zero_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 3; i++) TxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) TxI2S_Buff_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) TxI2S_Zero_TD[i]=CyDmaTdAllocate();
}

void DmaTxStart(void) {
    uint8 i, n;
    
    for (i=0; i < 3; i++) {
        if (i==2) {
            CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[0], 0 );
            CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16((uint32)&TxI2S_Zero), LO16((uint32)I2S_TX_FIFO_0_PTR));
        } else {
            CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[i+1], TxI2S_Stage__TD_TERMOUT_EN );
            CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16((uint32)&TxI2S_Stage), LO16((uint32)I2S_TX_FIFO_0_PTR));
        }
    }
    CyDmaClearPendingDrq(TxI2S_Stage_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Stage_DmaHandle, TxI2S_Stage_TD[0]);
    
    for (i=0; i < USB_AUDIO_BUFS; i++) {
        n = i + 1;
        if (n >= USB_AUDIO_BUFS) {
            n=0;
            CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN);    
        } else {
            CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], TD_INC_SRC_ADR);    
        }
        CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16((uint32)TxI2S[i]), LO16((uint32)&TxI2S_Stage));
        CyDmaTdSetConfiguration(TxI2S_Zero_TD[i], I2S_BUF_SIZE, TxI2S_Zero_TD[n], TD_INC_DST_ADR );
        CyDmaTdSetAddress(TxI2S_Zero_TD[i], LO16((uint32)&TxI2S_Zero), LO16((uint32)TxI2S[i]));
    }
    CyDmaClearPendingDrq(TxI2S_Buff_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Buff_DmaHandle, TxI2S_Buff_TD[0]);
    CyDmaClearPendingDrq(TxI2S_Zero_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Zero_DmaHandle, TxI2S_Zero_TD[0]);

    CyDmaChEnable(TxI2S_Zero_DmaHandle, 1u);
    CyDmaChSetRequest(TxI2S_Buff_DmaHandle, CPU_REQ);
    CyDmaChEnable(TxI2S_Buff_DmaHandle, 1u);
    CyDmaChEnable(TxI2S_Stage_DmaHandle, 1u);
}

uint8* PCM3060_TxBuf(void) {
    return TxI2S[SyncSOF_USB_Buffer()];
}

uint8* PCM3060_RxBuf(void) {
    return RxI2S[SyncSOF_USB_Buffer()];
}

void PCM3060_SetTxBufAddress(uint16 source) 
{
    uint8 buffer_idx = SyncSOF_USB_Buffer();
    // DMA register of the buffer source address.
    CY_SET_REG16((reg16*)&CY_DMA_TDMEM_STRUCT_PTR[TxI2S_Buff_TD[buffer_idx]].TD1[0u], source);
}

// Set the address of the TX buffer to be transmitted by the DMA to a default buffer address.
void PCM3060_SetTxBufAddressDefault()
{
    uint8 buffer_idx = SyncSOF_USB_Buffer();
    // DMA register of the buffer source address.
    CY_SET_REG16(
        (reg16*)&CY_DMA_TDMEM_STRUCT_PTR[TxI2S_Buff_TD[buffer_idx]].TD1[0u], 
        LO16((uint32)TxI2S[buffer_idx]));
}

uint8 PCM3060_SetRegister(uint8 reg, uint8 val) {
    uint8 pcm3060_cmd[2], i, state = 0;
    uint16 err = 0;

    while (state < 2) {
        switch (state) {
        case 0:
            pcm3060_cmd[0] = reg;
            pcm3060_cmd[1] = val;
            I2C_DISPLAY_MasterWriteBuf(PCM3060_I2C_ADDR, pcm3060_cmd, 2, I2C_DISPLAY_MODE_COMPLETE_XFER);
            state++;
            break;
        case 1:
            i = I2C_DISPLAY_MasterStatus();
            if (i & I2C_DISPLAY_MSTAT_ERR_XFER) {
                state--;
            } else if (i & I2C_DISPLAY_MSTAT_WR_CMPLT) {
                state++;
            }
            if (!--err) return 1;
            break;
        }
    }
    return 0;
}


uint8 PCM3060_Init(void) {
    I2S_Start();
    DmaRxInit();
    DmaTxInit();
    return PCM3060_Stop();
}

void PCM3060_Start(void) {
    PCM3060_SetRegister(0x40, 0xC0); // Wakeup
    PCM3060_SetRegister(0x45, 0x80); // Slow rolloff filter
    PCM3060_SetRegister(0x41, 0xff); //Set the volume to full
    PCM3060_SetRegister(0x42, 0xff); //Set the volume to full
    DmaRxStart();
    DmaTxStart();
    I2S_EnableRx();
    I2S_EnableTx();
}

uint8 PCM3060_Stop(void) {
    uint8 ret;
    ret = PCM3060_SetRegister(0x40, 0xF0); // Sleep
    I2S_DisableRx();
    I2S_DisableTx();
    CyDmaChDisable(RxI2S_Stage_DmaHandle);
    CyDmaChDisable(RxI2S_Buff_DmaHandle);
    CyDmaChDisable(TxI2S_Stage_DmaHandle);
    CyDmaChDisable(TxI2S_Buff_DmaHandle);
    CyDmaChDisable(TxI2S_Zero_DmaHandle);
    I2S_ClearRxFIFO();
    I2S_ClearTxFIFO();
    return ret;
}

