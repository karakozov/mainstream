#include "fpga.h"
#include "stream.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

Fpga::Fpga(unsigned id) : fpga_base(id)
{
    m_strm.clear();
    m_fpga_blocks.clear();
    m_fpga_trd.clear();
    m_ddr = 0;

    init();
}

//-----------------------------------------------------------------------------

Fpga::~Fpga()
{
    m_fpga_blocks.clear();
    m_fpga_trd.clear();
    deleteDmaCannels();
    if(m_ddr) delete m_ddr;
}

//-----------------------------------------------------------------------------

bool Fpga::syncFpga()
{
    int count = 0;

    core_block_write(0, 8, 0);
    IPC_delay(100);
again:
    core_block_write(0, 8, 1);
    IPC_delay(100);
    core_block_write(0, 8, 3);
    IPC_delay(100);
    core_block_write(0, 8, 7);
    IPC_delay(100);

    U32 status = core_block_read(0, 0x10);
    if((status & 0x1)) {
        //fprintf(stderr, "%s(): FPGA sync complete!\n", __FUNCTION__);
        return true;
    }

    if(count > 10) {
        //fprintf(stderr, "%s(): FPGA sync not complete!\n", __FUNCTION__);
        return false;
    }

    ++count;

    goto again;
}

//-----------------------------------------------------------------------------

void Fpga::init()
{
    try {

        syncFpga();
        scanFpgaBlocks();
        scanFpgaTetrades();
        createDmaChannels();
        createDDR3();

    } catch(...) {

        throw except_info("%s, %d: %s() - Error init FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
}

//-----------------------------------------------------------------------------

void Fpga::resetFifo(unsigned trd)
{
    U32 mode0 = FpgaRegPeekInd(trd, 0);

    mode0 |= 0x2;
    FpgaRegPokeInd(trd, 0, mode0);
    mode0 &= ~0x2;
    IPC_delay(1);
    FpgaRegPokeInd(trd, 0, mode0);
}

//-----------------------------------------------------------------------------

void Fpga::resetTrd(unsigned trd)
{
    U32 mode0 = FpgaRegPeekInd(trd, 0);

    mode0 |= 0x1;
    FpgaRegPokeInd(trd, 0, mode0);
    mode0 &= ~0x1;
    IPC_delay(1);
    FpgaRegPokeInd(trd, 0, mode0);
}

//-----------------------------------------------------------------------------
//#define _USE_IOCTL_
//-----------------------------------------------------------------------------

void Fpga::FpgaRegPokeInd(S32 TetrNum, S32 RegNum, U32 RegVal)
{
#ifdef _USE_IOCTL_
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, RegVal};

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_DATA,
                &reg_data,
                sizeof(AMB_DATA_REG),
                NULL,
                0);
    if(res < 0){
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
#else
    core_reg_poke_ind(TetrNum, RegNum, RegVal);
#endif
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaRegPeekInd(S32 TetrNum, S32 RegNum)
{
#ifdef _USE_IOCTL_
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, 0};

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_DATA,
                &reg_data,
                sizeof(AMB_DATA_REG),
                &reg_data,
                sizeof(AMB_DATA_REG));
    if(res < 0){
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    return reg_data.Value;
#else
    return core_reg_peek_ind(TetrNum, RegNum);
#endif
}

//-----------------------------------------------------------------------------

void Fpga::FpgaRegPokeDir(S32 TetrNum, S32 RegNum, U32 RegVal)
{
#ifdef _USE_IOCTL_
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, RegVal};

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_DATA_DIR,
                &reg_data,
                sizeof(AMB_DATA_REG),
                NULL,
                0);
    if(res < 0){
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
#else
    core_reg_poke_dir(TetrNum, RegNum, RegVal);
#endif
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaRegPeekDir(S32 TetrNum, S32 RegNum)
{
#ifdef _USE_IOCTL_
    AMB_DATA_REG reg_data = { 0, TetrNum, RegNum, 0};

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_DATA_DIR,
                &reg_data,
                sizeof(AMB_DATA_REG),
                &reg_data,
                sizeof(AMB_DATA_REG));
    if(res < 0){
        throw except_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    return reg_data.Value;
#else
    return core_reg_peek_dir(TetrNum, RegNum);
#endif
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaWriteRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    return core_write_reg_buf(TetrNum, RegNum, RegBuf, RegBufSize);
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaWriteRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    return core_write_reg_buf_dir(TetrNum, RegNum, RegBuf, RegBufSize);
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaReadRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    return core_read_reg_buf(TetrNum, RegNum, RegBuf, RegBufSize);
}

//-----------------------------------------------------------------------------

U32 Fpga::FpgaReadRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize)
{
    return core_read_reg_buf_dir(TetrNum, RegNum, RegBuf, RegBufSize);
}

//-----------------------------------------------------------------------------

U32  Fpga::FpgaBarRead( U32 bar, U32 offset )
{
    return core_bar_read(bar, offset);
}

//-----------------------------------------------------------------------------

void Fpga::FpgaBarWrite( U32 bar, U32 offset, U32 val )
{
    core_bar_write(bar, offset, val);
}

//-----------------------------------------------------------------------------

void Fpga::FpgaBlockWrite( U32 nb, U32 reg, U32 val )
{
    core_block_write(nb, reg, val);
}

//-----------------------------------------------------------------------------

U32  Fpga::FpgaBlockRead( U32 nb, U32 reg )
{
    return core_block_read(nb, reg);
}

//-----------------------------------------------------------------------------

void Fpga::createDDR3()
{
    fpga_trd_t memTrd;
    if(fpgaTrd(0, 0x9B, memTrd)) {

        m_ddr = new Memory(this);
        if(!m_ddr) {
            throw except_info("%s, %d: %s() - FPGA%d: Error create DDR3 object!\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
        }

    } else {

        fprintf(stderr, "FPGA%d: DDR3 tetrade not found\n", m_fpgaNumber);
    }
}
//-----------------------------------------------------------------------------

void Fpga::createDmaChannels()
{
    for(unsigned i=0; i<DMA_CHANNEL_NUM; i++) {

        AMB_GET_DMA_INFO InfoDescrip;
        InfoDescrip.DmaChanNum = i;

        int res = IPC_ioctlDevice( m_fpga, IOCTL_AMB_GET_INFOIO, &InfoDescrip, sizeof(AMB_GET_DMA_INFO), &InfoDescrip, sizeof(AMB_GET_DMA_INFO));
        if(res < 0) {
            m_strm.push_back(0);
            continue;
        }

        Stream *strm = new Stream(m_fpga, m_hwAddr, m_hwFpgaNum, i);
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
    if((DmaChan >= m_strm.size()) || !m_strm.at(DmaChan)) {
        throw except_info("%s, %d: %s() - Invalid DMA number: %d.\n", __FILE__, __LINE__, __FUNCTION__, DmaChan);
    }
    return m_strm.at(DmaChan);
}

//-----------------------------------------------------------------------------

void Fpga::scanFpgaBlocks()
{
    for(unsigned i=0; i<FPGA_BLK_NUM; i++) {

        fpga_block_t block;
        memset(&block, 0, sizeof(block));

        unsigned id = core_block_read(i, 0x0) & 0x7ff;

        if((id != 0x7ff) && (id != 0x0)) {

            u32 ver = core_block_read(i, 0x1);

            block.number = i;
            block.id = id & 0xffff;
            block.ver = ver & 0xffff;

            if(id == 0x13) {

                u32 did = core_block_read(i, 0x2);
                block.device_id = (did & 0xffff);
                //fprintf(stderr, "BLOCK%d: ID 0x%.4x  DID 0x%.4x  VER 0x%.4x\n", i, id, did, ver);

            } else {

                //fprintf(stderr, "BLOCK%d: ID 0x%.4x  VER 0x%.4x\n", i, id, ver);
            }

            m_fpga_blocks.push_back(block);
        }
    }
}

//-----------------------------------------------------------------------------

bool Fpga::fpgaBlock(unsigned startSearch, u16 id, fpga_block_t& block)
{
    if(m_fpga_blocks.empty())
        return false;

    if(startSearch >= m_fpga_blocks.size())
        return false;

    for(unsigned i=startSearch; i<m_fpga_blocks.size(); i++) {

        block = m_fpga_blocks.at(i);
        if(block.id == id)
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------

void Fpga::scanFpgaTetrades()
{
    for(unsigned i=0; i<FPGA_TRD_NUM; i++) {

        fpga_trd_t trd;
        memset(&trd, 0, sizeof(trd));

        unsigned id = FpgaRegPeekInd(i, 0x100);
        if(id != 0xffff && id != 0x0) {
            //fprintf(stderr, "TRD %d: - ID 0x%x\n", i, id);
            trd.number = i;
            trd.id = id;

            resetTrd(trd.number);
        }

        m_fpga_trd.push_back(trd);
    }
}

//-----------------------------------------------------------------------------

bool Fpga::trd_number(unsigned trdID, unsigned& number)
{
    for(unsigned i=0; i<m_fpga_trd.size(); i++) {

        fpga_trd_t trd = m_fpga_trd.at(i);

        if(trd.id == trdID) {
            number = trd.number;
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

bool Fpga::fpgaTrd(unsigned startSearch, u16 id, fpga_trd_t& trd)
{
    if(startSearch >= m_fpga_trd.size())
        return false;

    for(unsigned i=startSearch; i<m_fpga_trd.size(); i++) {

        trd = m_fpga_trd.at(i);
        if(trd.id == id)
            return true;
    }

    return false;
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

    int res = IPC_ioctlDevice( m_fpga, IOCTL_AMB_GET_INFOIO, &InfoDescrip, sizeof(AMB_GET_DMA_INFO), &InfoDescrip, sizeof(AMB_GET_DMA_INFO));
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

bool Fpga::fpgaInfo(AMB_CONFIGURATION& cfgInfo)
{
    return info(cfgInfo);
}

//-----------------------------------------------------------------------------

bool Fpga::FpgaHwAddress(U08& hwAddr, U08& fpgaNum)
{
    hwAddr = m_hwAddr;
    fpgaNum = m_hwFpgaNum;
    return true;
}

//-----------------------------------------------------------------------------

bool Fpga::FpgaDeviceID(U16& device_id)
{
    device_id = m_hwID;
    return true;
}

//-----------------------------------------------------------------------------

bool Fpga::FpgaTemperature(float &t)
{
    return false;
}
