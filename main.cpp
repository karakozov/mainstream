
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "acdsp.h"
#include "acsync.h"
#include "iniparser.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

void delay(int ms)
{
    struct timeval tv = {0, 0};
    tv.tv_usec = 1000*ms;

    select(0,NULL,NULL,NULL,&tv);
}

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);

//-----------------------------------------------------------------------------

#define ADC_TRD             4
#define MEM_TRD             5
#define STREAM_BLK_SIZE     0x10000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

#define ADC_SAMPLE_FREQ         (500000000.0)
#define ADC_CHANNEL_MASK        (0xF)
#define ADC_START_MODE          (0x3 << 4)
#define ADC_MAX_CHAN            (0x4)

//-----------------------------------------------------------------------------
#define USE_SIGNAL 0
//-----------------------------------------------------------------------------

#if USE_SIGNAL
acdsp *boardPtr = 0;
static int g_StopFlag = 0;
void stop_exam(int sig)
{
    fprintf(stderr, "\nSIGNAL = %d\n", sig);
    g_StopFlag = 1;
    if(boardPtr) boardPtr->setExitFlag(true);
}
#endif

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct app_params_t params;

    if(!getParams(argc, argv, params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    showParams(params);

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

    try {
        acsync sync(0);

        fprintf(stderr, "Create AC_SYNC board\n");
	sync.PowerON(true);
        sync.progFD(0, 1, 448.0, 0.0);
        //sync.progADF4002(10, 56, 0);
        //sync.RegPokeInd(0,4,1,0x7);
        //IPC_delay(1000);
        //sync.RegPokeInd(0,4,1,0x0);
        //acdsp brd;
        //brd.setSi57xFreq(params.adcFreq);
        //brd.start_local_pcie_test(params);
    }
    catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }
/*
    try {

        acdsp brd;

        brd.setSi57xFreq(params.adcFreq);

#if USE_SIGNAL
        boardPtr = &brd;
#endif

        fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
        fprintf(stderr, "DMA information:\n" );
        brd.infoDma(params.fpgaNumber);

        vector<void*> Buffers(params.dmaBlockCount, NULL);

#ifndef ALLOCATION_TYPE_1
        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, Buffers.data(), NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(params.fpgaNumber, params.dmaChannel, &sSCA);
#else
        BRDstrm_Stub *pStub = 0;
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(params.fpgaNumber, params.dmaChannel, Buffers.data(), params.dmaBlockSize, params.dmaBlockCount, 1, BRDstrm_DIR_IN, 0x1000, &pStub);
#endif
        delay(100);

        char fname[64];
        snprintf(fname, sizeof(fname), "data_%d.bin", params.fpgaNumber);
        IPC_handle isviFile = createDataFile(fname);

        char flgname[64];
        snprintf(flgname, sizeof(flgname), "data_%d.flg", params.fpgaNumber);
        createFlagFile(flgname);

        //---------------------------------------------------- DATA FROM STREAM ADC

        if(params.fpgaNumber != 0) {

            if(params.testMode == 0)
                brd.dataFromAdc(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS MEMORY

            if(params.testMode == 1)
                brd.dataFromMemAsMem(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS FIFO

            if(params.testMode == 2)
                brd.dataFromMemAsFifo(params, isviFile, flgname, sSCA);

            //----------------------------------------------------

        } else {

            //---------------------------------------------------- DATA FROM MAIN STREAM

            if(params.testMode == 0)
                brd.dataFromMain(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS MEMORY

            if(params.testMode == 1)
                brd.dataFromMainToMemAsMem(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS FIFO

            if(params.testMode == 2)
                brd.dataFromMainToMemAsFifo(params, isviFile, flgname, sSCA);

            //----------------------------------------------------
        }

        fprintf(stderr, "Free DMA memory\n");
        brd.freeDmaMemory(params.fpgaNumber, params.dmaChannel);

        Buffers.clear();

        IPC_closeFile(isviFile);

    }
    catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }
*/
#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    fprintf(stderr, "STOP\n");

    return 0;
}

//-----------------------------------------------------------------------------
