#include "isvi.h"
#include "acdsp.h"
#include "i2c.h"
#include "si571.h"
#include "fpga.h"
#include "stream.h"
#include "memory.h"
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

acdsp::acdsp()
{
    m_fpga.clear();
    m_exit = false;
    m_iic = new i2c(1);
    enableSwitchOut(0x1);
    m_si571 = new Si571(m_iic);

    createFpgaDevices();
    createFpgaMemory();
}

//-----------------------------------------------------------------------------

acdsp::~acdsp()
{
    deleteFpgaMemory();
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

void acdsp::createFpgaDevices()
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

void acdsp::createFpgaMemory()
{
    try {
        for(unsigned i=0; i<m_fpga.size(); i++) {
            Memory *ddr = new Memory(m_fpga.at(i));
            if(!ddr) {
                throw;
            }
            m_ddr.push_back(ddr);
        }
    } catch(...)  {
        deleteFpgaMemory();
    }
}

//-----------------------------------------------------------------------------

void acdsp::deleteFpgaMemory()
{
    for(unsigned i=0; i<m_ddr.size(); i++) {
        Memory *ddr = m_ddr.at(i);
        m_ddr.at(i) = 0;
        delete ddr;
    }
    m_ddr.clear();
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

Memory* acdsp::DDR3(unsigned fpgaNum)
{
    if(fpgaNum >= m_ddr.size()) {
        fprintf(stderr, "Invalid FPGA number: %d\n", fpgaNum);
        throw;
    }
    return m_ddr.at(fpgaNum);
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

#define MAIN_TRD 0
#define ADC_TRD  4
#define MEM_TRD  5

//-----------------------------------------------------------------------------

void acdsp::dataFromAdc(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    fprintf(stderr, "Set DMA source\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, ADC_TRD);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.fpgaNumber, params.dmaChannel, BRDstrm_DIR_IN);

    fprintf(stderr, "Set ADC channels mask\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x10, params.adcMask);

    fprintf(stderr, "Set ADC start mode\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x17, (0x3 << 4));

    fprintf(stderr, "Start DMA\n");
    startDma(params.fpgaNumber, params.dmaChannel, 0);

    fprintf(stderr, "Start ADC\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0, 0x2038);

    vector<u32*> data;
    for(unsigned i=0; i<sSCA.blkNum; i++)
        data.push_back((u32*)sSCA.ppBlk[i]);

    unsigned counter = 0;

    while(1) {

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_adc = RegPeekDir(params.fpgaNumber, ADC_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status_adc);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(params.fpgaNumber,ADC_TRD,0x0));
        for(unsigned i=0; i<data.size(); i++) {
            u32 *value = data.at(i);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, " ]\r");

        RegPokeInd(params.fpgaNumber,ADC_TRD,0,0x0);
        stopDma(params.fpgaNumber, params.dmaChannel);
        resetFifo(params.fpgaNumber, ADC_TRD);
        resetDmaFifo(params.fpgaNumber, params.dmaChannel);
        startDma(params.fpgaNumber,params.dmaChannel,0);
        delay(10);
        RegPokeInd(params.fpgaNumber,ADC_TRD,0,0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(50);
    }

    RegPokeInd(params.fpgaNumber,ADC_TRD,0,0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);
}

//-----------------------------------------------------------------------------

void acdsp::dataFromMemAsMem(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = params.dmaBuffersCount * sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    DDR3(params.fpgaNumber)->setMemory(1, 0, PostTrigSize, MemBufSize);
    DDR3(params.fpgaNumber)->Enable(true);

    fprintf(stderr, "setDmaSource(MEM_TRD)\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, MEM_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(params.fpgaNumber, params.dmaChannel, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(ADC_TRD)\n");
    resetFifo(params.fpgaNumber, ADC_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(params.fpgaNumber, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(params.fpgaNumber, params.dmaChannel);

    fprintf(stderr, "setAdcMask(0x%x)\n", params.adcMask);
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x10, params.adcMask);

    fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x17, (0x3 << 4));

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x2038);
    IPC_delay(10);

    // дожидаемся окончания сбора
    fprintf(stderr, "Waiting data DDR3...");
    bool ok = true;
    while(!DDR3(params.fpgaNumber)->AcqComplete()) {
        if(exitFlag()) {
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
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x0");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x0);

    DDR3(params.fpgaNumber)->Enable(false);

    unsigned counter = 0;

    fprintf(stderr, "\n");
    for(counter = 0; counter < params.dmaBuffersCount; counter++) {

        startDma(params.fpgaNumber, params.dmaChannel, 0x0);

        IPC_delay(10);

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_adc = RegPeekDir(params.fpgaNumber, ADC_TRD, 0x0);
            u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_adc, status_mem);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, counter * sSCA.blkSize * sSCA.blkNum);
            lockDataFile(flgName, counter);
            fprintf(stderr, "Write DMA buffer: %d\r", counter);
            sync();
        }
    }
    fprintf(stderr, "\n");

    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);

}

//-----------------------------------------------------------------------------

void acdsp::dataFromMemAsFifo(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    DDR3(params.fpgaNumber)->setMemory(2, 0, PostTrigSize, MemBufSize);
    DDR3(params.fpgaNumber)->Enable(true);

    fprintf(stderr, "setDmaSource(ADC_TRD)\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, ADC_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(params.fpgaNumber, params.dmaChannel, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(ADC_TRD)\n");
    resetFifo(params.fpgaNumber, ADC_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(params.fpgaNumber, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(params.fpgaNumber, params.dmaChannel);

    fprintf(stderr, "startDma(): Cycle = %d\n", 0x0);
    startDma(params.fpgaNumber, params.dmaChannel, 0x0);

    fprintf(stderr, "setAdcMask(0x%x)\n", params.adcMask);
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x10, params.adcMask);

    fprintf(stderr, "setAdcStartMode(0x%x)\n", (0x3 << 4));
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x17, (0x3 << 4));

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "ADC_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x2038);
    IPC_delay(10);

    vector<u32*> data;
    for(unsigned i=0; i<sSCA.blkNum; i++)
        data.push_back((u32*)sSCA.ppBlk[i]);

    unsigned counter = 0;

    while(1) {

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_adc = RegPeekDir(params.fpgaNumber, ADC_TRD, 0x0);
            u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_adc, status_mem);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        u32 status_adc = RegPeekDir(params.fpgaNumber, ADC_TRD, 0x0);
        u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
        fprintf(stderr, "%d: ADC: 0x%.4X - MEM: 0x%.4X [", ++counter, (u16)status_adc, (u16)status_mem);
        for(unsigned i=0; i<data.size(); i++) {
            u32 *value = data.at(i);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, " ]\r");


        RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x0);
        RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
        stopDma(params.fpgaNumber, params.dmaChannel);
        resetFifo(params.fpgaNumber, ADC_TRD);
        resetFifo(params.fpgaNumber, MEM_TRD);
        resetDmaFifo(params.fpgaNumber, params.dmaChannel);
        startDma(params.fpgaNumber,params.dmaChannel,0x0);
        delay(10);
        RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x2038);
        RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(250);
    }

    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x0);
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);
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

//-----------------------------------------------------------------------------

void acdsp::dataFromMain(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    fprintf(stderr, "Set DMA source\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, MAIN_TRD);

    IPC_delay(1000);

    fprintf(stderr, "Set DMA direction\n");
    setDmaDirection(params.fpgaNumber, params.dmaChannel, BRDstrm_DIR_IN);

    IPC_delay(1000);

    fprintf(stderr, "Set MAIN test mode\n");
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0xC, 0x1);

    IPC_delay(1000);

    fprintf(stderr, "Start DMA\n");
    startDma(params.fpgaNumber, params.dmaChannel, 0);

    IPC_delay(1000);

    fprintf(stderr, "Start MAIN\n");
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0, 0x2038);

    IPC_delay(1000);

    vector<u32*> data;
    for(unsigned i=0; i<sSCA.blkNum; i++)
        data.push_back((u32*)sSCA.ppBlk[i]);

    unsigned counter = 0;

    while(1) {

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_main = RegPeekDir(params.fpgaNumber, MAIN_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X\n", status_main);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        fprintf(stderr, "%d: STATUS = 0x%.4X [", ++counter, (u16)RegPeekDir(params.fpgaNumber,MAIN_TRD,0x0));
        for(unsigned i=0; i<data.size(); i++) {
            u32 *value = data.at(i);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, " ]\r");

        RegPokeInd(params.fpgaNumber,MAIN_TRD,0,0x0);
        stopDma(params.fpgaNumber, params.dmaChannel);
        resetFifo(params.fpgaNumber, MAIN_TRD);
        resetDmaFifo(params.fpgaNumber, params.dmaChannel);
        RegPokeInd(params.fpgaNumber, MAIN_TRD, 0xC, 0x0);
        startDma(params.fpgaNumber,params.dmaChannel,0);
        delay(10);
        RegPokeInd(params.fpgaNumber, MAIN_TRD, 0xC, 0x1);
        RegPokeInd(params.fpgaNumber,MAIN_TRD,0,0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(50);
    }

    RegPokeInd(params.fpgaNumber,MAIN_TRD,0,0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);
}

//-----------------------------------------------------------------------------

void acdsp::dataFromMainToMemAsMem(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = params.dmaBuffersCount * sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    DDR3(params.fpgaNumber)->setMemory(1, 0, PostTrigSize, MemBufSize);
    DDR3(params.fpgaNumber)->Enable(true);

    fprintf(stderr, "setDmaSource(MEM_TRD)\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, MEM_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(params.fpgaNumber, params.dmaChannel, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(MAIN_TRD)\n");
    resetFifo(params.fpgaNumber, MAIN_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(params.fpgaNumber, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(params.fpgaNumber, params.dmaChannel);

    fprintf(stderr, "setMainTestMode(0x%x)\n", 0x1);
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0xC, 0x1);

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "MAIN_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, ADC_TRD, 0x0, 0x2038);
    IPC_delay(10);

    // дожидаемся окончания сбора
    fprintf(stderr, "Waiting data DDR3...");
    bool ok = true;
    while(!DDR3(params.fpgaNumber)->AcqComplete()) {
        if(exitFlag()) {
            ok = false;
            break;
        }
    }
    IPC_delay(10);
    if(ok) {
        fprintf(stderr, "OK\n");
    } else {
        fprintf(stderr, "ERROR\n");
        return;
    }

    fprintf(stderr, "MEM_TRD: MODE0 = 0x0\n");
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);

    fprintf(stderr, "MAIN_TRD: MODE0 = 0x0");
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x0);

    DDR3(params.fpgaNumber)->Enable(false);

    unsigned counter = 0;

    fprintf(stderr, "\n");
    for(counter = 0; counter < params.dmaBuffersCount; counter++) {

        startDma(params.fpgaNumber, params.dmaChannel, 0x0);

        IPC_delay(10);

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_main = RegPeekDir(params.fpgaNumber, MAIN_TRD, 0x0);
            u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", status_main, status_mem);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, counter * sSCA.blkSize * sSCA.blkNum);
            lockDataFile(flgName, counter);
            fprintf(stderr, "Write DMA buffer: %d\r", counter);
            sync();
        }
    }
    fprintf(stderr, "\n");

    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);

}

//-----------------------------------------------------------------------------

void acdsp::dataFromMainToMemAsFifo(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA)
{
    U32 MemBufSize = sSCA.blkSize * sSCA.blkNum;
    U32 PostTrigSize = 0;

    DDR3(params.fpgaNumber)->setMemory(2, 0, PostTrigSize, MemBufSize);
    DDR3(params.fpgaNumber)->Enable(true);

    fprintf(stderr, "setDmaSource(MAIN_TRD)\n");
    setDmaSource(params.fpgaNumber, params.dmaChannel, MAIN_TRD);

    fprintf(stderr, "setDmaRequestFlag(BRDstrm_DRQ_HALF)\n");
    setDmaRequestFlag(params.fpgaNumber, params.dmaChannel, BRDstrm_DRQ_HALF);

    fprintf(stderr, "resetFifo(MAIN_TRD)\n");
    resetFifo(params.fpgaNumber, MAIN_TRD);

    fprintf(stderr, "resetFifo(MEM_TRD)\n");
    resetFifo(params.fpgaNumber, MEM_TRD);

    fprintf(stderr, "resetDmaFifo()\n");
    resetDmaFifo(params.fpgaNumber, params.dmaChannel);

    fprintf(stderr, "setMainTestMode(0x%x)\n", 0x1);
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0xC, 0x1);

    fprintf(stderr, "startDma(): Cycle = %d\n", 0x0);
    startDma(params.fpgaNumber, params.dmaChannel, 0x0);

    fprintf(stderr, "MEM_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);
    IPC_delay(10);

    fprintf(stderr, "MAIN_TRD: MODE0 = 0x2038\n");
    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x2038);
    IPC_delay(10);

    vector<u32*> data;
    for(unsigned i=0; i<sSCA.blkNum; i++)
        data.push_back((u32*)sSCA.ppBlk[i]);

    unsigned counter = 0;

    while(1) {

        if( waitDmaBuffer(params.fpgaNumber, params.dmaChannel, 2000) < 0 ) {

            u32 status_main = RegPeekDir(params.fpgaNumber, MAIN_TRD, 0x0);
            u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
            fprintf( stderr, "ERROR TIMEOUT! MAIN STATUS = 0x%.4X MEM STATUS = 0x%.4X\n", (u16)status_main, (u16)status_mem);
            break;

        } else {

            writeBuffer(params.fpgaNumber, params.dmaChannel, isviFile, 0);
            lockDataFile(flgName, counter);
        }

        u32 status_main = RegPeekDir(params.fpgaNumber, MAIN_TRD, 0x0);
        u32 status_mem = RegPeekDir(params.fpgaNumber, MEM_TRD, 0x0);
        fprintf(stderr, "%d: MAIN: 0x%.4X - MEM: 0x%.4X [", ++counter, (u16)status_main, (u16)status_mem);
        for(unsigned i=0; i<data.size(); i++) {
            u32 *value = data.at(i);
            fprintf(stderr, " 0x%.8x ", value[0]);
        }
        fprintf(stderr, " ]\r");


        RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x0);
        RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
        stopDma(params.fpgaNumber, params.dmaChannel);
        resetFifo(params.fpgaNumber, MAIN_TRD);
        resetFifo(params.fpgaNumber, MEM_TRD);
        resetDmaFifo(params.fpgaNumber, params.dmaChannel);
        startDma(params.fpgaNumber,params.dmaChannel,0x0);
        delay(10);
        RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x2038);
        RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x2038);

        if(exitFlag()) {
            fprintf(stderr, "\n");
            break;
        }

        IPC_delay(250);
    }

    RegPokeInd(params.fpgaNumber, MAIN_TRD, 0x0, 0x0);
    RegPokeInd(params.fpgaNumber, MEM_TRD, 0x0, 0x0);
    stopDma(params.fpgaNumber, params.dmaChannel);
}

//-----------------------------------------------------------------------------
