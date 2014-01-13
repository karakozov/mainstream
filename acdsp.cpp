#include "isvi.h"
#include "acdsp.h"
#include "i2c.h"
#include "si571.h"
#include "fpga.h"
#include "stream.h"
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

acdsp::acdsp()
{
    m_fpga.clear();
    m_exit = false;
    m_iic = new i2c(1);
    enableSwitchOut(0x1);
    m_si571 = new Si571(m_iic);

    createFpgaDevices();
}

//-----------------------------------------------------------------------------

acdsp::~acdsp()
{
    deleteFpgaDevices();

    delete m_si571;
    delete m_iic;
}

//-----------------------------------------------------------------------------

int acdsp::enableSwitchOut(unsigned mask)
{
    if(!m_iic) return 0;

    // enable i2c switch output to Si571
    m_iic->write(0x70,mask);

    // check switch settings
    unsigned val = m_iic->read(0x70);
    if(val != mask) {
        fprintf(stderr, "Can't set i2c switch: mask = 0x%x\n", mask);
        throw;
    }
    return val;
}

//-----------------------------------------------------------------------------

int acdsp::setSi57xFreq(float freq)
{
    float desiredFreq = freq;
    m_si571->SetRate(&desiredFreq);
    fprintf(stderr, "desiredFreq = %f\n", desiredFreq);

    float setFreq = 0;
    m_si571->GetRate(&setFreq);
    fprintf(stderr, "setFreq = %f\n", setFreq);

    return 0;
}

//-----------------------------------------------------------------------------

void acdsp::createFpgaDevices(void)
{
    try {
        for(unsigned i=0; i<FPGA_COUNT; i++) {
            Fpga *fpga = new Fpga(i);
            if(!fpga) {
                throw;
            }
            fpga->init();
            m_fpga.push_back(fpga);
        }
    } catch(...)  {
        deleteFpgaDevices();
    }
}

//-----------------------------------------------------------------------------

void acdsp::deleteFpgaDevices()
{
    for(unsigned i=0; i<m_fpga.size(); i++) {
        Fpga *fpga = m_fpga.at(i);
        m_fpga.at(i) = 0;
        delete fpga;
    }
    m_fpga.clear();
}

//-----------------------------------------------------------------------------

Fpga* acdsp::FPGA(unsigned fpgaNum)
{
    if(fpgaNum >= m_fpga.size()) {
        fprintf(stderr, "Invalid FPGA number: %d\n", fpgaNum);
        throw;
    }
    return m_fpga.at(fpgaNum);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                     REGISTER ACCESS LEVEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void acdsp::RegPokeInd(U32 fpgaNum, S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA(fpgaNum)->FpgaRegPokeInd(TetrNum, RegNum, RegVal);
}

//-----------------------------------------------------------------------------

U32 acdsp::RegPeekInd(U32 fpgaNum, S32 TetrNum, S32 RegNum)
{
    return FPGA(fpgaNum)->FpgaRegPeekInd(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------

void acdsp::RegPokeDir(U32 fpgaNum, S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA(fpgaNum)->FpgaRegPokeDir(TetrNum,RegNum,RegVal);
}

//-----------------------------------------------------------------------------

U32 acdsp::RegPeekDir(U32 fpgaNum, S32 TetrNum, S32 RegNum)
{
    return FPGA(fpgaNum)->FpgaRegPeekDir(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                          DMA CHANNEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int acdsp::allocateDmaMemory(U32 fpgaNum, U32 DmaChan, BRDctrl_StreamCBufAlloc* param)
{
    return FPGA(fpgaNum)->allocateDmaMemory(DmaChan, param);
}

//-----------------------------------------------------------------------------

int acdsp::allocateDmaMemory(U32 fpgaNum, U32 DmaChan,
                             void** pBuf,
                             U32 blkSize,
                             U32 blkNum,
                             U32 isSysMem,
                             U32 dir,
                             U32 addr,
                             BRDstrm_Stub **pStub)
{
    return FPGA(fpgaNum)->allocateDmaMemory(DmaChan, pBuf, blkSize, blkNum, isSysMem, dir, addr, pStub);
}

//-----------------------------------------------------------------------------

int acdsp::freeDmaMemory(U32 fpgaNum, U32 DmaChan)
{
    return FPGA(fpgaNum)->freeDmaMemory(DmaChan);
}

//-----------------------------------------------------------------------------

int acdsp::startDma(U32 fpgaNum, U32 DmaChan, int isCycle)
{
    return FPGA(fpgaNum)->startDma(DmaChan, isCycle);
}

//-----------------------------------------------------------------------------

int acdsp::stopDma(U32 fpgaNum, U32 DmaChan)
{
    return FPGA(fpgaNum)->stopDma(DmaChan);
}

//-----------------------------------------------------------------------------

int acdsp::stateDma(U32 fpgaNum, U32 DmaChan, U32 msTimeout, int& state, U32& blkNum)
{
    return FPGA(fpgaNum)->stateDma(DmaChan, msTimeout, state, blkNum);
}

//-----------------------------------------------------------------------------

int acdsp::waitDmaBuffer(U32 fpgaNum, U32 DmaChan, U32 timeout)
{
    return FPGA(fpgaNum)->waitDmaBuffer(DmaChan, timeout);
}

//-----------------------------------------------------------------------------

int acdsp::waitDmaBlock(U32 fpgaNum, U32 DmaChan, U32 timeout)
{
    return FPGA(fpgaNum)->waitDmaBlock(DmaChan, timeout);
}

//-----------------------------------------------------------------------------

int acdsp::resetDmaFifo(U32 fpgaNum, U32 DmaChan)
{
    return FPGA(fpgaNum)->resetDmaFifo(DmaChan);
}

//-----------------------------------------------------------------------------

int acdsp::setDmaSource(U32 fpgaNum, U32 DmaChan, U32 src)
{
    return FPGA(fpgaNum)->setDmaSource(DmaChan, src);
}

//-----------------------------------------------------------------------------

int acdsp::setDmaDirection(U32 fpgaNum, U32 DmaChan, U32 dir)
{
    return FPGA(fpgaNum)->setDmaDirection(DmaChan, dir);
}

//-----------------------------------------------------------------------------

int acdsp::setDmaRequestFlag(U32 fpgaNum, U32 DmaChan, U32 flag)
{
    return FPGA(fpgaNum)->setDmaRequestFlag(DmaChan, flag);
}

//-----------------------------------------------------------------------------

int acdsp::adjustDma(U32 fpgaNum, U32 DmaChan, U32 adjust)
{
    return FPGA(fpgaNum)->adjustDma(DmaChan, adjust);
}

//-----------------------------------------------------------------------------

int acdsp::doneDma(U32 fpgaNum, U32 DmaChan, U32 done)
{
    return FPGA(fpgaNum)->doneDma(DmaChan, done);
}

//-----------------------------------------------------------------------------

void acdsp::resetFifo(U32 fpgaNum, U32 trd)
{
    return FPGA(fpgaNum)->resetFifo(trd);
}

//-----------------------------------------------------------------------------

void acdsp::infoDma(U32 fpgaNum)
{
    return FPGA(fpgaNum)->infoDma();
}

//-----------------------------------------------------------------------------

bool acdsp::writeBlock(U32 fpgaNum, U32 DmaChan, IPC_handle file, int blockNumber)
{
    return FPGA(fpgaNum)->writeBlock(DmaChan, file, blockNumber);
}

//-----------------------------------------------------------------------------

bool acdsp::writeBuffer(U32 fpgaNum, U32 DmaChan, IPC_handle file, int fpos)
{
    return FPGA(fpgaNum)->writeBuffer(DmaChan, file, fpos);
}

//-----------------------------------------------------------------------------

#define ADC_TRD 4
#define MEM_TRD 5

//-----------------------------------------------------------------------------

void acdsp::dataFromAdc(U32 fpgaNum, U32 DmaChan, U32 AdcMask, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    fprintf(stderr, "Set DMA source\n");
    setDmaSource(fpgaNum, DmaChan, ADC_TRD);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(fpgaNum, DmaChan, BRDstrm_DIR_IN);

    fprintf(stderr, "Set ADC channels mask\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0x10, AdcMask);

    fprintf(stderr, "Set ADC start mode\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0x17, (0x3 << 4));

    fprintf(stderr, "Start DMA\n");
    startDma(fpgaNum, DmaChan, 0);

    fprintf(stderr, "Start ADC\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0, 0x2038);

    u32 *data0 = (u32*)sSCA.ppBlk[0];
    u32 *data1 = (u32*)sSCA.ppBlk[1];
    u32 *data2 = (u32*)sSCA.ppBlk[2];
    u32 *data3 = (u32*)sSCA.ppBlk[3];

    unsigned counter = 0;

    while(1) {

        if( waitDmaBuffer(fpgaNum, DmaChan, 2000) < 0 ) {

            U32 status = RegPeekDir(fpgaNum,ADC_TRD,0x0);
            fprintf( stderr, "ERROR TIMEOUT! STATUS = 0x%.4X\n", status);
            break;

        } else {

            writeBuffer(fpgaNum, DmaChan, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        u32 status = RegPeekDir(fpgaNum,ADC_TRD,0x0);
        fprintf(stderr, "%d: STATUS = 0x%.4X [0x%.8x 0x%.8x 0x%.8x 0x%.8x]\r", ++counter, (u16)status, data0[0], data1[0], data2[0], data3[0]);

        stopDma(fpgaNum, DmaChan);
        RegPokeDir(fpgaNum,ADC_TRD,0,0x3);
        startDma(fpgaNum, DmaChan, 0);
        delay(10);
        RegPokeDir(fpgaNum,ADC_TRD,0,0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(50);
    }

    fprintf(stderr, "Stop DMA channel\n");
    stopDma(fpgaNum, DmaChan);

    fprintf(stderr, "Reset DMA FIFO\n");
    resetDmaFifo(fpgaNum, DmaChan);
}

//-----------------------------------------------------------------------------

void acdsp::dataFromMemAsMem(U32 fpgaNum, U32 DmaChan, U32 AdcMask, U32 BufferCounter, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = BufferCounter * sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    FPGA(fpgaNum)->DDR3()->setMemory(1, 0, PostTrigSize, MemBufSize);
    FPGA(fpgaNum)->DDR3()->Enable(true);

    fprintf(stderr, "setDmaSource(MEM_TRD)\n");
    setDmaSource(fpgaNum, DmaChan, MEM_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(fpgaNum, DmaChan, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(ADC_TRD)\n");
    resetFifo(fpgaNum, ADC_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(fpgaNum, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(fpgaNum, DmaChan);

    fprintf(stderr, "setAdcMask(0x%x)\n", AdcMask);
    RegPokeInd(fpgaNum, ADC_TRD, 0x10, AdcMask);

    fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
    RegPokeInd(fpgaNum, ADC_TRD, 0x17, (0x3 << 4));

    //fprintf(stderr, "ADC_TRD: TEST = 0x100\n");
    //RegPokeInd(fpgaNum, ADC_TRD, 0xC, (1 << 8));

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(fpgaNum, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0x0, 0x2038);
    IPC_delay(10);

    // дожидаемся окончания сбора
    fprintf(stderr, "Waiting data DDR3...");
    bool ok = true;
    while(!FPGA(fpgaNum)->DDR3()->AcqComplete()) {
        if(m_exit) {
            ok = false;
            break;
        }
    }
    if(ok) {
        fprintf(stderr, "OK\n");
    } else {
        fprintf(stderr, "ERROR\n");
        return;
    }

    fprintf(stderr, "MEM_TRD: MODE0 = 0x0\n");
    RegPokeInd(fpgaNum, MEM_TRD, 0x0, 0x0);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x0");
    RegPokeInd(fpgaNum, ADC_TRD, 0x0, 0x0);

    fprintf(stderr, "ADC_TRD: TEST = 0x0\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0xC, 0x0);

    unsigned counter = 0;

    fprintf(stderr, "\n");
    for(counter = 0; counter < BufferCounter; counter++) {

        startDma(fpgaNum, DmaChan, 0x0);

        IPC_delay(10);

        if( waitDmaBuffer(fpgaNum, DmaChan, 2000) < 0 ) {

            u32 status = RegPeekDir(fpgaNum, ADC_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status);
            status = RegPeekDir(fpgaNum, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MEM STATUS = 0x%.4X\n", status);
            break;

        } else {

            writeBuffer(fpgaNum, DmaChan, isviFile, counter * sSCA.blkSize * sSCA.blkNum);
            lockDataFile(flgName, counter);
            fprintf(stderr, "Write DMA buffer: %d\r", counter);
            sync();
        }
    }
    fprintf(stderr, "\n");
}

//-----------------------------------------------------------------------------

void acdsp::dataFromMemAsFifo(U32 fpgaNum, U32 DmaChan, U32 AdcMask, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    FPGA(fpgaNum)->DDR3()->setMemory(2, 0, PostTrigSize, MemBufSize);
    FPGA(fpgaNum)->DDR3()->Enable(true);

    fprintf(stderr, "setDmaSource(ADC_TRD)\n");
    setDmaSource(fpgaNum, DmaChan, ADC_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(fpgaNum, DmaChan, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(ADC_TRD)\n");
    resetFifo(fpgaNum, ADC_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(fpgaNum, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(fpgaNum, DmaChan);

    fprintf(stderr, "startDma(): Cycle = %d\n", 0x0);
    startDma(fpgaNum, DmaChan, 0x0);

    fprintf(stderr, "setAdcMask(0x%x)\n", AdcMask);
    RegPokeInd(fpgaNum, ADC_TRD, 0x10, AdcMask);

    fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
    RegPokeInd(fpgaNum, ADC_TRD, 0x17, (0x3 << 4));

    //fprintf(stderr, "ADC_TRD: TEST = 0x100\n");
    //RegPokeInd(fpgaNum, ADC_TRD, 0xC, (1 << 8));

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(fpgaNum, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
    RegPokeInd(fpgaNum, ADC_TRD, 0x0, 0x2038);
    IPC_delay(10);

    unsigned counter = 0;

    u32 *data0 = (u32*)sSCA.ppBlk[0];
    u32 *data1 = (u32*)sSCA.ppBlk[1];
    u32 *data2 = (u32*)sSCA.ppBlk[2];
    u32 *data3 = (u32*)sSCA.ppBlk[3];

    while(1) {

        if( waitDmaBuffer(fpgaNum, DmaChan, 2000) < 0 ) {

            u32 status = RegPeekDir(fpgaNum, ADC_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status);
            status = RegPeekDir(fpgaNum, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MEM STATUS = 0x%.4X\n", status);
            break;

        } else {

            writeBuffer(fpgaNum, DmaChan, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        u32 status_adc = RegPeekDir(fpgaNum, ADC_TRD, 0x0);
        u32 status_mem = RegPeekDir(fpgaNum, MEM_TRD, 0x0);
        fprintf(stderr, "%d: ADC: 0x%.4X - MEM: 0x%.4X  [0x%.8x 0x%.8x 0x%.8x 0x%.8x]\r",
                ++counter, (u16)status_adc, (u16)status_mem, data0[0], data1[0], data2[0], data3[0]);

        stopDma(fpgaNum, DmaChan);
        RegPokeDir(fpgaNum,ADC_TRD,0,0x3);
        startDma(fpgaNum,DmaChan,0);
        delay(10);
        RegPokeDir(fpgaNum,ADC_TRD,0,0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(50);
    }
}

//-----------------------------------------------------------------------------

void acdsp::setExitFlag(bool exit)
{
    m_exit = exit;
}

//-----------------------------------------------------------------------------

bool acdsp::exitFlag()
{
    if(IPC_kbhit())
        return true;
    return m_exit;
}
