
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "acdsp.h"

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

IPC_handle createFile(const char *fname);
bool lockDataFile(const char* fname, int counter);
void unlockDataFile(const char* fname);

//-----------------------------------------------------------------------------

#define STREAM_TRD          4
#define STREAM_BLK_SIZE     0x40000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(stderr, "usage: mainstream <FPGA number> <DMA channel>\n");
        return -1;
    }

    IPC_initKeyboard();

    try {

        U32 fpgaNum = strtol(argv[1], 0, 10);
        U32 DmaChan = strtol(argv[2], 0, 10);
        void *pBuffers[4] = {0,0,0,0,};

        acdsp brd;

        brd.setSi57xFreq(500000000.0);

        fprintf(stderr, "Init FPGA\n");
        brd.initFpga(fpgaNum);

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
        char flgname[64];
        snprintf(flgname, sizeof(flgname), "data_%d.flg", fpgaNum);
        IPC_handle isviFile = createFile(fname);

        fprintf(stderr, "Set DMA source\n");
        brd.setDmaSource(fpgaNum, DmaChan, STREAM_TRD);

        fprintf(stderr, "Set DMA direction\n");
        brd.setDmaDirection(fpgaNum, DmaChan, BRDstrm_DIR_IN);

        fprintf(stderr, "Set ADC channels mask\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0x10,0xF);

        fprintf(stderr, "Set ADC start mode\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0x17,(0x3 << 4));

        fprintf(stderr, "Start DMA\n");
        brd.startDma(fpgaNum, DmaChan, 0);

        delay(10);

        fprintf(stderr, "Start ADC\n");
        brd.RegPokeInd(fpgaNum,STREAM_TRD,0,0x2038);

        u32 *data0 = (u32*)pBuffers[0];
        u32 *data1 = (u32*)pBuffers[1];
        u32 *data2 = (u32*)pBuffers[2];
        u32 *data3 = (u32*)pBuffers[3];

        unsigned couter = 0;

        while(1) {


            if( brd.waitDmaBuffer(fpgaNum, DmaChan, 2000) < 0 ) {

                u32 status = brd.RegPeekDir(fpgaNum,STREAM_TRD,0x0);
                fprintf( stderr, "STATUS = 0x%.4X\n", status);
                break;

            } else {

                // than write data for ISVI enabled - NFS wiil hangup after few times. (issue)
                // not working via ISVI running on NFS linux server directly (issue)
                if(lockDataFile(flgname, 100)) {
                    brd.writeBuffer(fpgaNum, DmaChan, isviFile, 0);
                    unlockDataFile(flgname);
                }
            }

            u32 status = brd.RegPeekDir(fpgaNum,STREAM_TRD,0x0);
            fprintf(stderr, "%d: STATUS = 0x%.4X [0x%x 0x%x 0x%x 0x%x]\r", ++couter, (u16)status, data0[0], data1[0], data2[0], data3[0]);

            brd.stopDma(fpgaNum, DmaChan);
            brd.RegPokeDir(fpgaNum,STREAM_TRD,0,0x3);
            brd.startDma(fpgaNum, DmaChan, 0);
            delay(10);
            brd.RegPokeDir(fpgaNum,STREAM_TRD,0,0x2038);

            if(IPC_kbhit()) {
                fprintf(stderr, "\n");
                break;
            }

            IPC_delay(50);
        }

        fprintf(stderr, "Stop DMA channel\n");
        brd.stopDma(fpgaNum, DmaChan);

        fprintf(stderr, "Free DMA memory\n");
        brd.freeDmaMemory(fpgaNum, DmaChan);

        fprintf(stderr, "Reset DMA FIFO\n");
        brd.resetDmaFifo(fpgaNum, DmaChan);

        IPC_closeFile(isviFile);

    } catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }

    IPC_cleanupKeyboard();

    return 0;
}

//-----------------------------------------------------------------------------

void WriteFlagSinc(const char *fileName, int flg, int isNewParam, int idx)
{
    IPC_handle handle = NULL;
    int val[2] = {0, 0};

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    val[0] = flg;
    val[1] = isNewParam;
    int res = IPC_writeFile(handle, val, sizeof(val));
    if(res < 0) {
        fprintf( stderr, "Error write flag sync\r\n" );
    }
    IPC_closeFile(handle);
}

//-----------------------------------------------------------------------------

int  ReadFlagSinc(const char *fileName, int idx)
{
    IPC_handle handle = NULL;
    int flg;

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    int res = IPC_readFile(handle, &flg, sizeof(flg));
    if(res < 0) {
        fprintf( stderr, "Error read flag sync\r\n" );
    }
    IPC_closeFile(handle);

    //fprintf( stderr, "%s(): FLG = 0x%x\n", __func__, flg );

    return flg;
}

//-----------------------------------------------------------------------------

bool lockDataFile(const char* fname, int counter)
{
    int attempt = 0;
    do {
        int flg = ReadFlagSinc(fname, 0);
        if(flg == 0x0) {
            return true;
        }
        ++attempt;
        IPC_delay(1);
    } while(attempt <= counter);
    return false;
}

//-----------------------------------------------------------------------------

void unlockDataFile(const char* fname)
{
    WriteFlagSinc(fname,0xffffffff,1,0);
}

//-----------------------------------------------------------------------------

IPC_handle createFile(const char *fname)
{
    //IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR | IPC_FILE_DIRECT);
    IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR);
    if(!fd) {
        fprintf(stderr, "%s(): Error create file: %s\n", __FUNCTION__, fname);
        throw;
    }

    fprintf(stderr, "%s(): %s - Ok\n", __FUNCTION__, fname);

    return fd;
}

//-----------------------------------------------------------------------------
