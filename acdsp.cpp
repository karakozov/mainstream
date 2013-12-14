#include "acdsp.h"
#include "i2c.h"
#include "si571.h"
#include "fpga.h"
#include "stream.h"

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

    iic = new i2c(1);
    enableSwitchOut(0x1);
    si571 = new Si571(iic);

    createFpgaDevices();
}

//-----------------------------------------------------------------------------

acdsp::~acdsp()
{
    deleteFpgaDevices();

    delete si571;
    delete iic;
}

//-----------------------------------------------------------------------------

int acdsp::enableSwitchOut(unsigned mask)
{
    if(!iic) return 0;

    // enable i2c switch output to Si571
    iic->write(0x70,mask);

    // check switch settings
    unsigned val = iic->read(0x70);
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
    si571->SetRate(&desiredFreq);
    fprintf(stderr, "desiredFreq = %f\n", desiredFreq);

    float setFreq = 0;
    si571->GetRate(&setFreq);
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
    if((fpgaNum < 0) || (fpgaNum >= m_fpga.size())) {
        fprintf(stderr, "Invalid FPGA number: %d\n", fpgaNum);
        throw;
    }
    return m_fpga.at(fpgaNum);
}

//-----------------------------------------------------------------------------

void acdsp::initFpga(unsigned fpgaNum)
{
    FPGA(fpgaNum)->initFpga();
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
