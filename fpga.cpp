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

Fpga::Fpga(unsigned id) : m_fpgaID(id)
{
    m_fpgaDev = 0;
    m_strm.clear();
    openFpgaDevice();
    createDmaChannels();
}

//-----------------------------------------------------------------------------

Fpga::~Fpga()
{
    deleteDmaCannels();
    closeFpgaDevice();
}

//-----------------------------------------------------------------------------

void Fpga::initFpga()
{
    //Set RST and FIFO_RST in MODE0 in all tetrades
    for( int trd=0; trd<8; trd++ ) {
        FpgaRegPokeInd(trd, 0, 0x3);
    }

    delay(10);

    //zero all other registers
    for( int trd=0; trd<8; trd++ ) {
        for( int ii=1; ii<32; ii++ ) {
            FpgaRegPokeInd(trd, ii, 0);
        }
    }

    delay(10);

    //Clear RST and FIFO_RST in MODE0 in all tetrades
    for( int trd=0; trd<8; trd++ ) {
        FpgaRegPokeInd(trd, 0, 0);
    }

    delay(10);
}

//-----------------------------------------------------------------------------

void Fpga::FpgaRegPokeInd(S32 TetrNum, S32 RegNum, U32 RegVal)
{
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, RegVal};

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_WRITE_REG_DATA,
                &reg_data,
                sizeof(AMB_DATA_REG),
                NULL,
                0);
    if(res < 0){
        throw;
    }
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaRegPeekInd(S32 TetrNum, S32 RegNum)
{
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, 0};

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_READ_REG_DATA,
                &reg_data,
                sizeof(AMB_DATA_REG),
                &reg_data,
                sizeof(AMB_DATA_REG));
    if(res < 0){
        throw;
    }

    return reg_data.Value;
}

//-----------------------------------------------------------------------------

void Fpga::FpgaRegPokeDir(S32 TetrNum, S32 RegNum, U32 RegVal)
{
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, RegVal};

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_WRITE_REG_DATA_DIR,
                &reg_data,
                sizeof(AMB_DATA_REG),
                NULL,
                0);
    if(res < 0){
        throw;
    }
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaRegPeekDir(S32 TetrNum, S32 RegNum)
{
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, 0};

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_READ_REG_DATA_DIR,
                &reg_data,
                sizeof(AMB_DATA_REG),
                &reg_data,
                sizeof(AMB_DATA_REG));
    if(res < 0){
        throw;
    }

    return reg_data.Value;
}

//-----------------------------------------------------------------------------

void Fpga::openFpgaDevice(void)
{
    char name[256];
    m_fpgaDev = IPC_openDevice(name, AmbDeviceName, m_fpgaID);
    if(!m_fpgaDev) {
        throw;
    }
    fprintf(stderr, "Open FPGA%d\n", m_fpgaID);
}

//-----------------------------------------------------------------------------

void Fpga::closeFpgaDevice()
{
    fprintf(stderr, "Close FPGA%d\n", m_fpgaID);
    IPC_closeDevice(m_fpgaDev);
}

//-----------------------------------------------------------------------------

void Fpga::createDmaChannels()
{
    for(unsigned i=0; i<DMA_CHANNEL_NUM; i++) {

        AMB_GET_DMA_INFO InfoDescrip;
        InfoDescrip.DmaChanNum = i;

        int res = IPC_ioctlDevice( m_fpgaDev, IOCTL_AMB_GET_INFOIO, &InfoDescrip, sizeof(AMB_GET_DMA_INFO), &InfoDescrip, sizeof(AMB_GET_DMA_INFO));
        if(res < 0) {
            m_strm.push_back(0);
            continue;
        }

        Stream *strm = new Stream(m_fpgaDev, i);
        m_strm.push_back(strm);
    }
}

//-----------------------------------------------------------------------------

void Fpga::deleteDmaCannels()
{
    for(unsigned i=0; i<m_strm.size(); i++) {

        Stream *strm = m_strm.at(i);
        if(strm) delete strm;
    }
    m_strm.clear();
}

//-----------------------------------------------------------------------------

Stream* Fpga::stream(U32 DmaChan)
{
    if((DmaChan < 0) || (DmaChan >= m_strm.size()) || !m_strm.at(DmaChan)) {
        fprintf(stderr, "Invalid DMA number: %d\n", DmaChan);
        throw;
    }
    return m_strm.at(DmaChan);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                          DMA CHANNEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool Fpga::dmaChannelInfo(U32 DmaChan, U32& dir, U32& FifoSize, U32& MaxDmaSize)
{
    AMB_GET_DMA_INFO InfoDescrip;
    InfoDescrip.DmaChanNum = DmaChan;

    int res = IPC_ioctlDevice( m_fpgaDev, IOCTL_AMB_GET_INFOIO, &InfoDescrip, sizeof(AMB_GET_DMA_INFO), &InfoDescrip, sizeof(AMB_GET_DMA_INFO));
    if(res < 0) {
        return false;
    }

    dir = InfoDescrip.Direction;
    FifoSize = InfoDescrip.FifoSize;
    MaxDmaSize = InfoDescrip.MaxDmaSize;

    return true;
}

//-----------------------------------------------------------------------------

void Fpga::infoDma()
{
    for(int iDmaChan = 0; iDmaChan < DMA_CHANNEL_NUM; iDmaChan++)
    {
        U32 dir;
        U32 FifoSize;
        U32 MaxDmaSize;
        if(!dmaChannelInfo(iDmaChan, dir, FifoSize, MaxDmaSize)) {
            fprintf(stderr, "No DMA channel %d\n", iDmaChan);
        } else {
            char strbuf[40];
            switch(dir)
            {
            case BRDstrm_DIR_IN:
                strcpy(strbuf, "Input");
                break;
            case BRDstrm_DIR_OUT:
                strcpy(strbuf, "Output");
                break;
            case BRDstrm_DIR_INOUT:
                strcpy(strbuf, "In/Out");
                break;
            }
            fprintf(stderr, "DMA channel %d: Direction = %d(%s) FIFO size = %d kB, DMA max.size = %d MB\n", iDmaChan, dir, strbuf,
                    FifoSize / 1024, MaxDmaSize / 1024 / 1024);
        }
    }
}

//-----------------------------------------------------------------------------

int Fpga::allocateDmaMemory(U32 DmaChan, BRDctrl_StreamCBufAlloc* param)
{
    return stream(DmaChan)->allocateDmaMemory(param);
}

//-----------------------------------------------------------------------------

int Fpga::allocateDmaMemory(U32 DmaChan,
                      void** pBuf,
                      U32 blkSize,
                      U32 blkNum,
                      U32 isSysMem,
                      U32 dir,
                      U32 addr,
                      BRDstrm_Stub **pStub)
{
    return stream(DmaChan)->allocateDmaMemory(pBuf, blkSize, blkNum, isSysMem, dir, addr, pStub);
}

//-----------------------------------------------------------------------------

int Fpga::freeDmaMemory(U32 DmaChan)
{
    return stream(DmaChan)->freeDmaMemory();
}

//-----------------------------------------------------------------------------

int Fpga::startDma(U32 DmaChan, int isCycle)
{
    return stream(DmaChan)->startDma(isCycle);
}

//-----------------------------------------------------------------------------

int Fpga::stopDma(U32 DmaChan)
{
    return stream(DmaChan)->stopDma();
}

//-----------------------------------------------------------------------------

int Fpga::stateDma(U32 DmaChan, U32 msTimeout, int& state, U32& blkNum)
{
    return stream(DmaChan)->stateDma(msTimeout, state, blkNum);
}

//-----------------------------------------------------------------------------

int Fpga::waitDmaBuffer(U32 DmaChan, U32 timeout)
{
    return stream(DmaChan)->waitDmaBuffer(timeout);
}

//-----------------------------------------------------------------------------

int Fpga::waitDmaBlock(U32 DmaChan, U32 timeout)
{
    return stream(DmaChan)->waitDmaBlock(timeout);
}

//-----------------------------------------------------------------------------

int Fpga::resetDmaFifo(U32 DmaChan)
{
    return stream(DmaChan)->resetDmaFifo();
}

//-----------------------------------------------------------------------------

int Fpga::setDmaSource(U32 DmaChan, U32 src)
{
    return stream(DmaChan)->setDmaSource(src);
}

//-----------------------------------------------------------------------------

int Fpga::setDmaDirection(U32 DmaChan, U32 dir)
{
    return stream(DmaChan)->setDmaDirection(dir);
}

//-----------------------------------------------------------------------------

int Fpga::adjustDma(U32 DmaChan, U32 adjust)
{
    return stream(DmaChan)->adjustDma(adjust);
}

//-----------------------------------------------------------------------------

int Fpga::doneDma(U32 DmaChan, U32 done)
{
    return stream(DmaChan)->doneDma(done);
}

//-----------------------------------------------------------------------------

bool Fpga::writeBlock(U32 DmaChan, IPC_handle file, int blockNumber)
{
    return stream(DmaChan)->writeBlock(file, blockNumber);
}

//-----------------------------------------------------------------------------

bool Fpga::writeBuffer(U32 DmaChan, IPC_handle file, int fpos)
{
    return stream(DmaChan)->writeBuffer(file, fpos);
}

//-----------------------------------------------------------------------------
