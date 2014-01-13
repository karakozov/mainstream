
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "acdsp.h"
#include "memory.h"

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
    if(argc != 5) {
        fprintf(stderr, "usage: mainstream <FPGA number> <DMA channel> <ADC mask> <ADC frequency>\n");
        return -1;
    }

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

    try {

        U32 fpgaNum = strtol(argv[1], 0, 16);
        U32 DmaChan = strtol(argv[2], 0, 16);
        U32 AdcMask = strtol(argv[3], 0, 16);
        float AdcFreq = strtof(argv[4], 0);
        void *pBuffers[4] = {0,0,0,0,};

        fprintf(stderr, "fpgaNum: 0x%d\n", fpgaNum);
        fprintf(stderr, "DmaChan: 0x%d\n", DmaChan);
        fprintf(stderr, "AdcMask: 0x%x\n", AdcMask);
        fprintf(stderr, "AdcFreq: %f\n", AdcFreq);

        acdsp brd;

        brd.setSi57xFreq(AdcFreq);

#if USE_SIGNAL
        boardPtr = &brd;
#endif

        fprintf(stderr, "Start testing DMA: %d\n", DmaChan);
        fprintf(stderr, "DMA information:\n" );
        brd.infoDma(fpgaNum);

#ifndef ALLOCATION_TYPE_1
        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, STREAM_BLK_NUM, STREAM_BLK_SIZE, pBuffers, NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(fpgaNum, DmaChan, &sSCA);
#else
        BRDstrm_Stub *pStub = 0;
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(fpgaNum, DmaChan, pBuffers,STREAM_BLK_SIZE,STREAM_BLK_NUM,1,BRDstrm_DIR_IN,0x1000,&pStub);
#endif
        delay(100);

        char fname[64];
        snprintf(fname, sizeof(fname), "data_%d.bin", fpgaNum);
        IPC_handle isviFile = createDataFile(fname);

        char flgname[64];
        snprintf(flgname, sizeof(flgname), "data_%d.flg", fpgaNum);
        createFlagFile(flgname);

        //---------------------------------------------------- DATA FROM STREAM ADC

        brd.dataFromAdc(fpgaNum, DmaChan, AdcMask, isviFile, flgname, sSCA);

        //---------------------------------------------------- DDR3 FPGA AS MEMORY

        brd.dataFromMemAsMem(fpgaNum, DmaChan, AdcMask, 16, isviFile, flgname, sSCA);

        //---------------------------------------------------- DDR3 FPGA AS FIFO

        brd.dataFromMemAsFifo(fpgaNum, DmaChan, AdcMask,isviFile, flgname, sSCA);

        //----------------------------------------------------

        fprintf(stderr, "Free DMA memory\n");
        brd.freeDmaMemory(fpgaNum, DmaChan);

        IPC_closeFile(isviFile);

    } catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    return 0;
}

//-----------------------------------------------------------------------------
