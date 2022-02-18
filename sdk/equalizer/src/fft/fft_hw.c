/******************************************************************************
 * @file audio.c
 * Audio driver.
 *
 * @authors RoHegbeC
 *
 * @date 2014-Oct-30
 *
 * @copyright
 * (c) 2015 Copyright Digilent Incorporated
 * All Rights Reserved
 *
 * This program is free software; distributed under the terms of BSD 3-clause
 * license ("Revised BSD License", "New BSD License", or "Modified BSD License")
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name(s) of the above-listed copyright holder(s) nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @desciption
 *
 * This program was initially developed to be run from within the BRAM. It is
 * constructed to run in a polling mode, in which the program poles the Empty and
 * Full signals of the two FIFO's which are implemented in the audio I2S VHDL core.
 * In order to have a continuous and stable Sound both when recording and playing
 * the user must ensure that DDR cache is enabled. This is only mandatory when the
 * program is loaded in to the DDR, if the program is stored in the BRAM then
 * the cache is not mandatory.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who          Date     Changes
 * ----- ------------ ----------- -----------------------------------------------
 * 1.00  RoHegbeC 2014-Oct-30 First release
 *
 * </pre>
 *
 *****************************************************************************/


#include "fft.h"
#include "../hw.h"

/************************** Variable Definitions *****************************/

extern volatile sHwState_t Demo;

sFFTConfig_t default_config = {
    .scale_sch_0 = 0,
    .fwd_inv_0 = FFT_FWD,
    .nfft = NFFT_16,
};

/******************************************************************************
 * Initialize FFT engine
 *
 * @param	none
 *
 * @return	none.
 *****************************************************************************/
XStatus fnInitFFT(FFT* inst, XAxiDma *AxiDma, XGpio *Gpio)
{
    inst->gpio = Gpio;
    inst->dma = AxiDma;

    fnConfigureFFT(inst, default_config);

    return XST_SUCCESS;
}

/*****************************************************************************
 * Configure FFT engine by setting the TDATA of the configuration stream
 * 
 * @param	inst		FFT instance
 * @param	config		FFT configuration
 * 
 *****************************************************************************/

XStatus fnConfigureFFT(FFT* inst, sFFTConfig_t config) {
    u32 bits;

    // output nfft to the first channel
    XGpio_DiscreteWrite(inst->gpio, 1, config.nfft);

    // output config to the second channel
    bits = (config.scale_sch_0 << 1) | config.fwd_inv_0;
    XGpio_DiscreteWrite(inst->gpio, 2, bits);

    inst->config = config;

    return XST_SUCCESS;
}

XStatus fnWriteFFTBuffer(FFT* inst, u32* buffer, u32 size) {
    int Status;

    if(size != pow(2, inst->config.nfft)) {
        xil_printf("FFT buffer size is not correct\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_SimpleTransfer(inst->dma, (u32)buffer, sizeof(u32), XAXIDMA_DMA_TO_DEVICE);
    if (Status != XST_SUCCESS) {
        xil_printf("DMA transfer failed\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

XStatus fnReadFFTBuffer(FFT* inst, u32* buffer, u32 size) {
    int Status;

    if(size != pow(2, inst->config.nfft)) {
        xil_printf("FFT buffer size is not correct\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_SimpleTransfer(inst->dma, (u32)buffer, sizeof(u32), XAXIDMA_DEVICE_TO_DMA);
    if (Status != XST_SUCCESS) {
        xil_printf("DMA transfer failed\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

XStatus fnFFTSync(FFT* inst, u32* input, u32* output, u32 size) {
    int Status;

    if(size != pow(2, inst->config.nfft)) {
        xil_printf("FFT buffer size is not correct\n");
        return XST_FAILURE;
    }

    Status = fnWriteFFTBuffer(inst, input, size);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    Status = fnReadFFTBuffer(inst, output, size);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}
