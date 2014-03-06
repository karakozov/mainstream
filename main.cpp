
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "pcie_test.h"
#include "acdsp.h"
#include "acsync.h"
#include "iniparser.h"
#include "fpga_base.h"
#include "exceptinfo.h"
#include "fpga.h"
#include "isvi.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);

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
    struct sync_params_t sync_params;

    if(!getParams(argc, argv, params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    if(!getSyncParams(argc, argv, sync_params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    showParams(params);
    showSyncParams(sync_params);

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

#if 1
    try {

        std::vector<Fpga*> fpgaList;
        std::vector<acdsp*> boardList;
        acsync *sync = 0;

        create_fpga_list(fpgaList, 10);
        create_board_list(fpgaList, boardList, &sync);

        if(sync) {
            sync->PowerON(true);
            sync->progFD(sync_params.sync_mode, sync_params.sync_selclkout, sync_params.sync_fd, sync_params.sync_fo);
        }

        start_pcie_test(boardList, params);

        delete_board_list(boardList,sync);
        delete_fpga_list(fpgaList);
    }
    catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }
#else
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
        IPC_delay(100);

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
#endif

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    fprintf(stderr, "STOP\n");

    return 0;
}

//-----------------------------------------------------------------------------
