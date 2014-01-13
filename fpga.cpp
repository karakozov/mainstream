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
    m_ddr = 0;
    m_strm.clear();
    m_trd.clear();
    openFpgaDevice();
}

//-----------------------------------------------------------------------------

Fpga::~Fpga()
{
    m_trd.clear();
    delete m_ddr;
    deleteDmaCannels();
    closeFpgaDevice();
}

//-----------------------------------------------------------------------------

void Fpga::init()
{
    try {

        //Set RST and FIFO_RST in MODE0 in all tetrades
        for( int trd=0; trd<TRD_NUM; trd++ ) {
            FpgaRegPokeInd(trd, 0, 0x3);
        }

        delay(1);

        //Clear RST and FIFO_RST in MODE0 in all tetrades
        for( int trd=0; trd<TRD_NUM; trd++ ) {
            FpgaRegPokeInd(trd, 0, 0);
        }

        scanFpgaTetrades();
        createDmaChannels();

        m_ddr = new Memory(this);

    } catch(...) {

        fprintf(stderr, "%s, %d, %s(): Exception!", __FILE__, __LINE__, __FUNCTION__);
        throw;
    }
}

//-----------------------------------------------------------------------------

void Fpga::resetFifo(unsigned trd)
{
    U32 mode0 = FpgaRegPeekInd(trd, 0);

    mode0 |= 0x2;

    FpgaRegPokeInd(trd, 0, mode0);

    mode0 &= ~0x2;

    delay(1);

    FpgaRegPokeInd(trd, 0, mode0);
}

//-----------------------------------------------------------------------------

void Fpga::resetTrd(unsigned trd)
{
    U32 mode0 = FpgaRegPeekInd(trd, 0);

    mode0 |= 0x1;

    FpgaRegPokeInd(trd, 0, mode0);

    mode0 &= ~0x1;

    delay(1);

    FpgaRegPokeInd(trd, 0, mode0);
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

U32 Fpga::FpgaWriteRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_WRITE_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                0,
                0);
    if(res < 0){
        throw;
    }
    return 0;
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaWriteRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_WRITE_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                NULL,
                0);
    if(res < 0) {
        throw;
    }
    return 0;
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaReadRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_READ_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw;
    }
    return 0;
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaReadRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpgaDev,
                IOCTL_AMB_READ_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw;
    }
    return 0;
}

//-----------------------------------------------------------------------------

void Fpga::openFpgaDevice(void)
{
    char name[256];
    m_fpgaDev = IPC_openDevice(name, AmbDeviceName, m_fpgaID);
    if(!m_fpgaDev) {
        fprintf(stderr, "Error open FPGA%d\n", m_fpgaID);
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

void Fpga::createMemory()
{
    m_ddr = new Memory(this);
}

//-----------------------------------------------------------------------------

void Fpga::deleteMemory()
{
    if(m_ddr) delete m_ddr;
}

//-----------------------------------------------------------------------------

Stream* Fpga::stream(U32 DmaChan)
{
    if((DmaChan >= m_strm.size()) || !m_strm.at(DmaChan)) {
        fprintf(stderr, "Invalid DMA number: %d\n", DmaChan);
        throw;
    }
    return m_strm.at(DmaChan);
}

//-----------------------------------------------------------------------------

void Fpga::scanFpgaTetrades()
{
    for(unsigned i=0; i<TRD_NUM; i++) {

        unsigned trd_ID = FpgaRegPeekInd(i, 0x100);
        if(trd_ID != 0xff && trd_ID != 0x0) {
            fprintf(stderr, "TRD %d: - ID 0x%x\n", i, trd_ID);
            m_trd.push_back(trd_ID);
        } else {
            m_trd.push_back(0);
        }
    }
}

//-----------------------------------------------------------------------------

int Fpga::trd_number(unsigned trdID)
{
    for(unsigned i=0; i<m_trd.size(); i++) {

        unsigned fpga_trd_ID = m_trd.at(i);
        if(fpga_trd_ID == trdID) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------

Memory* Fpga::DDR3()
{
    return m_ddr;
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

int Fpga::setDmaRequestFlag(U32 DmaChan, U32 flag)
{
    return stream(DmaChan)->setDmaRequestFlag(flag);
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

bool Fpga::setMemory(U32 mem_mode, U32 PretrigMode, U32& PostTrigSize, U32& Buf_size)
{
    return m_ddr->setMemory(mem_mode, PretrigMode, PostTrigSize, Buf_size);
}

//-----------------------------------------------------------------------------
