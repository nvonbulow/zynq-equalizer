/*
 * main.c
 *
 *  Created on: Feb 16, 2022
 *      Author: nick
 */


#include "audio/audio.h"
#include "dma/dma.h"
#include "intc/intc.h"
#include "userio/userio.h"
#include "iic/iic.h"

/***************************** Include Files *********************************/

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xdebug.h"
#include "xiic.h"
#include "xaxidma.h"
#include "xtime_l.h"

#include "fft/fft.h"
#include <complex.h>


#ifdef XPAR_INTC_0_DEVICE_ID
 #include "xintc.h"
 #include "microblaze_sleep.h"
#else
 #include "xscugic.h"
#include "sleep.h"
#include "xil_cache.h"
#endif

/************************** Constant Definitions *****************************/

/*
 * Device hardware build related constants.
 */

// Audio constants
// Number of seconds to record/playback
#define NR_SEC_TO_REC_PLAY		5

// ADC/DAC sampling rate in Hz
//#define AUDIO_SAMPLING_RATE		1000
#define AUDIO_SAMPLING_RATE	  96000

// Number of samples to record/playback
#define NR_AUDIO_SAMPLES 4096

/* Timeout loop counter for reset
 */
#define RESET_TIMEOUT_COUNTER	10000

#define TEST_START_VALUE	0x0


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/
#if (!defined(DEBUG))
extern void xil_printf(const char *format, ...);
#endif


/************************** Variable Definitions *****************************/
/*
 * Device instance definitions
 */

static XIic sIic;
static XAxiDma sAudioDma;		/* XAxiDma connected to the audio codec */

static XGpio sUserIO;

#ifdef XPAR_INTC_0_DEVICE_ID
 static XIntc sIntc;
#else
 static XScuGic sIntc;
#endif

//
// Interrupt vector table
#ifdef XPAR_INTC_0_DEVICE_ID
const ivt_t ivt[] = {
    //IIC
    {XPAR_AXI_INTC_0_AXI_IIC_0_IIC2INTC_IRPT_INTR, (XInterruptHandler)XIic_InterruptHandler, &sIic},
    //DMA Stream to MemoryMap Interrupt handler
    {XPAR_AXI_INTC_0_AXI_DMA_0_S2MM_INTROUT_INTR, (XInterruptHandler)fnS2MMInterruptHandler, &sAudioDma},
    //DMA MemoryMap to Stream Interrupt handler
    {XPAR_AXI_INTC_0_AXI_DMA_0_MM2S_INTROUT_INTR, (XInterruptHandler)fnMM2SInterruptHandler, &sAudioDma},
    //User I/O (buttons, switches, LEDs)
    {XPAR_AXI_INTC_0_AXI_GPIO_0_IP2INTC_IRPT_INTR, (XInterruptHandler)fnUserIOIsr, &sUserIO}
};
#else
const ivt_t ivt[] = {
    //IIC
    {XPAR_FABRIC_AXI_IIC_0_IIC2INTC_IRPT_INTR, (Xil_ExceptionHandler)XIic_InterruptHandler, &sIic},
    //Audio DMA Stream to MemoryMap Interrupt handler
    {XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, (Xil_ExceptionHandler)fnS2MMInterruptHandler, &sAudioDma},
    //Audio DMA MemoryMap to Stream Interrupt handler
    {XPAR_FABRIC_AXI_DMA_0_MM2S_INTROUT_INTR, (Xil_ExceptionHandler)fnMM2SInterruptHandler, &sAudioDma},
    //User I/O (buttons, switches, LEDs)
    {XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR, (Xil_ExceptionHandler)fnUserIOIsr, &sUserIO},
    // FFT DMA Stream to MemoryMap Interrupt handler
    //{XPAR_FABRIC_AXI_DMA_1_S2MM_INTROUT_INTR},
};
#endif


int initHw() {
    int Status;
    //Initialize the interrupt controller
    Status = fnInitInterruptController(&sIntc);
    if(Status != XST_SUCCESS) {
        xil_printf("Error initializing interrupts");
        return XST_FAILURE;
    }


    // Initialize IIC controller
    Status = fnInitIic(&sIic);
    if(Status != XST_SUCCESS) {
        xil_printf("Error initializing I2C controller");
        return XST_FAILURE;
    }

    // Initialize User I/O driver
    Status = fnInitUserIO(&sUserIO);
    if(Status != XST_SUCCESS) {
        xil_printf("User I/O ERROR");
        return XST_FAILURE;
    }


    //Initialize DMA
    Status = fnConfigDma(&sAudioDma);
    if(Status != XST_SUCCESS) {
        xil_printf("DMA configuration ERROR");
        return XST_FAILURE;
    }


    //Initialize Audio I2S
    Status = fnInitAudio();
    if(Status != XST_SUCCESS) {
        xil_printf("Audio initializing ERROR");
        return XST_FAILURE;
    }

    {
        XTime  tStart, tEnd;
        XTime_GetTime(&tStart);
        do {
            XTime_GetTime(&tEnd);
        }
        while((tEnd-tStart)/(COUNTS_PER_SECOND/10) < 20);
    }
    //Initialize Audio I2S
    Status = fnInitAudio();
    if(Status != XST_SUCCESS) {
        xil_printf("Audio initializing ERROR");
        return XST_FAILURE;
    }


    // Enable all interrupts in our interrupt vector table
    // Make sure all driver instances using interrupts are initialized first
    fnEnableInterrupts(&sIntc, ivt, sizeof(ivt)/sizeof(ivt[0]));
    return XST_SUCCESS;
}

typedef float complex cplx;
int equalize(u32* audioBuf, int size) {
    int i;
    cplx* tSeries = malloc(sizeof(cplx)*size);
    cplx* fSeries = malloc(sizeof(cplx)*size);

    for(i = 0; i < size; i++) {
        tSeries[i] = audioBuf[i];
    }

    fft(tSeries, fSeries, size);
    // do equalization here

    ifft(fSeries, tSeries, size);

    for(i = 0; i < size; i++) {
        audioBuf[i] = cabsf(tSeries[i]);
    }

    free(tSeries);
    free(fSeries);

    return 1;
}

/*****************************************************************************/
/**
*
* Main function
*
* This function is the main entry of the interrupt test. It does the following:
*	Initialize the interrupt controller
*	Initialize the IIC controller
*	Initialize the User I/O driver
*	Initialize the DMA engine
*	Initialize the Audio I2S controller
*	Enable the interrupts
*	Wait for a button event then start selected task
*	Wait for task to complete
*
* @param	None
*
* @return
*		- XST_SUCCESS if example finishes successfully
*		- XST_FAILURE if example fails.
*
* @note		None.
*
******************************************************************************/
int main(void)
{
    u32* audio_buffers[2];
    int Status;
    int i;
    XTime start, stop;

    Demo.u8Verbose = 1;

    //Xil_DCacheDisable();

    xil_printf("\r\n--- Entering main() --- \r\n");

    initHw();

    xil_printf("----------------------------------------------------------\r\n");
    xil_printf("Zybo DMA Audio Demo\r\n");
    xil_printf("----------------------------------------------------------\r\n");
    audio_buffers[0] = malloc(sizeof(u32)*NR_AUDIO_SAMPLES*2);
    audio_buffers[1] = &audio_buffers[0][NR_AUDIO_SAMPLES];

    if(!audio_buffers[0] || !audio_buffers[1]) {
        xil_printf("Failed to allocate memory for the audio buffer\r\n");
        return XST_FAILURE;
    }
    xil_printf("Base Address: %d\r\n", audio_buffers[0]);
    xil_printf("Base Address: %d\r\n", audio_buffers[1]);

    //Demo.fStartRecord = 1;

    //main loop
    while(1) {

        // Checking the DMA S2MM event flag
        if (Demo.fAudioRecord && !XAxiDma_Busy(&sAudioDma, XAXIDMA_DEVICE_TO_DMA))
        {
            xil_printf("\r\nRecordong Done...");

            // Disable Stream function to send data (S2MM)
            //Xil_Out32(I2S_STREAM_CONTROL_REG, 0x00000000);
            //Xil_Out32(I2S_TRANSFER_CONTROL_REG, 0x00000000);

            fnAudioStreamEnable(I2S_STREAM_INPUT, 0);
            fnAudioTransferEnable(I2S_TRANSFER_INPUT, 0);

            Xil_DCacheInvalidateRange((u32) audio_buffers[0], sizeof(u32)*NR_AUDIO_SAMPLES);

            //microblaze_invalidate_dcache();
            // Reset S2MM event and record flag
            Demo.fAudioDmaS2MMEvent = 0;
            Demo.fAudioRecord = 0;
            // Swap buffers
            u32* tmp = audio_buffers[0];
            audio_buffers[0] = audio_buffers[1];
            audio_buffers[1] = tmp;



            // Start recording the next buffer
            //for (i = 0; i < 10000; i++) {}
            //sleep(2);
            fnAudioRecord(sAudioDma, audio_buffers[0], NR_AUDIO_SAMPLES);
            Demo.fAudioRecord = 1;

            //Status = XAxiDma_SimpleTransfer(&sAudioDma,(u32) audio_buffers[0], sizeof(u32)*NR_AUDIO_SAMPLES, XAXIDMA_DEVICE_TO_DMA);
            //if(Status != XST_SUCCESS) {
            //     xil_printf("Error starting audio recording");
            //}

            // Equalize the audio
            XTime_GetTime(&start);
            //equalize(audio_buffers[1], NR_AUDIO_SAMPLES);
            XTime_GetTime(&stop);
            printf("\r\nTook %.2f us\r\n",
            		1.0*(stop-start) / (COUNTS_PER_SECOND/1000000));

            if(!Demo.fAudioPlayback) {
            	// Start streaming the audio
            	fnAudioPlay(sAudioDma, audio_buffers[1], NR_AUDIO_SAMPLES);
            	Demo.fAudioPlayback = 1;
            }

        }
        // Checking the DMA MM2S event flag
        if (Demo.fAudioPlayback && !XAxiDma_Busy(&sAudioDma, XAXIDMA_DMA_TO_DEVICE))
        {
            xil_printf("\r\nPlayback Done...");

            // Disable Stream function to send data (S2MM)
            //Xil_Out32(I2S_STREAM_CONTROL_REG, 0x00000000);
            //Xil_Out32(I2S_TRANSFER_CONTROL_REG, 0x00000000);
            fnAudioStreamEnable(I2S_STREAM_OUTPUT, 0);
            fnAudioTransferEnable(I2S_TRANSFER_OUTPUT, 0);

            //Flush cache
            Xil_DCacheFlushRange((u32) audio_buffers[0], sizeof(u32)*NR_AUDIO_SAMPLES);
            //Reset MM2S event and playback flag
            Demo.fAudioDmaMM2SEvent = 0;
            // Start streaming the audio
            fnAudioPlay(sAudioDma, audio_buffers[1], NR_AUDIO_SAMPLES);
            Demo.fAudioPlayback = 1;
        }

        // Checking the DMA Error event flag
        if (Demo.fAudioDmaError)
        {
            xil_printf("\r\nDma Error...");
            xil_printf("\r\nDma Reset...");


            Demo.fAudioDmaError = 0;
            Demo.fAudioPlayback = 0;
            Demo.fAudioRecord = 0;
        }

        // Checking the btn change event
        if(Demo.fUserIOEvent) {

            switch(Demo.chBtn) {
                case 'u':
                    if (!Demo.fAudioRecord && !Demo.fAudioPlayback)
                    {
                        xil_printf("\r\nStart Recording...\r\n");
                        fnSetMicInput();

                        fnAudioRecord(sAudioDma, audio_buffers[0], NR_AUDIO_SAMPLES);
                        Demo.fAudioRecord = 1;
                    }
                    else
                    {
                        if (Demo.fAudioRecord)
                        {
                            xil_printf("\r\nStill Recording...\r\n");
                        }
                        else
                        {
                            xil_printf("\r\nStill Playing back...\r\n");
                        }
                    }
                    break;
                case 'd':
                    if (!Demo.fAudioRecord && !Demo.fAudioPlayback)
                    {
                        xil_printf("\r\nStart Playback...\r\n");
                        fnSetHpOutput();
                        fnAudioPlay(sAudioDma, audio_buffers[0], NR_AUDIO_SAMPLES);
                        Demo.fAudioPlayback = 1;
                    }
                    else
                    {
                        if (Demo.fAudioRecord)
                        {
                            xil_printf("\r\nStill Recording...\r\n");
                        }
                        else
                        {
                            xil_printf("\r\nStill Playing back...\r\n");
                        }
                    }
                    break;
                case 'r':
                    if (!Demo.fAudioRecord && !Demo.fAudioPlayback)
                    {
                        xil_printf("\r\nStart Recording...\r\n");
                        fnSetLineInput();
                        fnAudioRecord(sAudioDma, audio_buffers[0], NR_AUDIO_SAMPLES);
                        Demo.fAudioRecord = 1;
                    }
                    else
                    {
                        if (Demo.fAudioRecord)
                        {
                            xil_printf("\r\nStill Recording...\r\n");
                        }
                        else
                        {
                            xil_printf("\r\nStill Playing back...\r\n");
                        }
                    }
                    break;
                case 'l':
                    if (!Demo.fAudioRecord && !Demo.fAudioPlayback)
                    {
                        xil_printf("\r\nStart Playback...");
                        fnSetLineOutput();
                        fnAudioPlay(sAudioDma, audio_buffers[0], NR_AUDIO_SAMPLES);
                        Demo.fAudioPlayback = 1;
                    }
                    else
                    {
                        if (Demo.fAudioRecord)
                        {
                            xil_printf("\r\nStill Recording...\r\n");
                        }
                        else
                        {
                            xil_printf("\r\nStill Playing back...\r\n");
                        }
                    }
                    break;
                default:
                    break;
            }

            // Reset the user I/O flag
            Demo.chBtn = 0;
            Demo.fUserIOEvent = 0;
        }

    }

    xil_printf("\r\n--- Exiting main() --- \r\n");


    return XST_SUCCESS;

}
