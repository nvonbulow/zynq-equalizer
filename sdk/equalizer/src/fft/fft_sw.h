/******************************************************************************
 * @file audio.h
 * Audio driver include file.
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

#ifndef FFT_SW_H_
#define FFT_SW_H_

#include <math.h>
#include <complex.h>
#include "xparameters.h"
#include "xgpio.h"
#include "xaxidma.h"
#include "xil_io.h"
#include "xiic.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "xstatus.h"
#include "sleep.h"
#include "../dma/dma.h"
#include "../hw.h"


/************************** Constant Definitions *****************************/

enum eFFT_nfft {
	NFFT_1     = 0,
	NFFT_2     = 1,
	NFFT_4     = 2,
	NFFT_8     = 3,
	NFFT_16    = 4,
	NFFT_32    = 5,
	NFFT_64    = 6,
	NFFT_128   = 7,
	NFFT_256   = 8,
	NFFT_512   = 9,
	NFFT_1024  = 10,
	NFFT_2048  = 11,
	NFFT_4096  = 12,
	NFFT_8192  = 13,
	NFFT_16384 = 14,
	NFFT_32768 = 15,
};

enum eFFT_fwdinv {
	FFT_INV = 0,
	FFT_FWD = 1,
};


/************************** Variable Definitions *****************************/

typedef struct {
	// scale
	u32 scale_sch_0;
	enum eFFT_fwdinv fwd_inv_0;
	enum eFFT_nfft nfft;
} sFFTConfig_t;

typedef struct {
	XGpio* gpio;
	XAxiDma* dma;
	sFFTConfig_t config;
} FFT;


/************************** Function Definitions *****************************/

XStatus fnInitFFT();
XStatus fnConfigureFFT(FFT* inst, sFFTConfig_t config);


/************************** Software Implementation *********************************/

void fft(float complex input[], float complex output[], int n);

void ifft(float complex input[], float complex output[], int n);

#endif /* AUDIO_H_ */
