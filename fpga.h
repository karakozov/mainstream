#ifndef __FPGA_H__
#define __FPGA_H__

#include "gipcy.h"
#include "fpga_base.h"
#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define DMA_CHANNEL_NUM         4
#define FPGA_TRD_NUM            8
#define FPGA_BLK_NUM            8

//-----------------------------------------------------------------------------

class Stream;

//-----------------------------------------------------------------------------

class Fpga : public fpga_base
{
public:
    explicit Fpga(unsigned id);
    virtual ~Fpga();

    void init();
    void resetFifo(unsigned trd);
    void resetTrd(unsigned trd);
    bool trd_number(unsigned trdID, unsigned& number);

    void FpgaRegPokeInd(S32 trdNo, S32 rgnum, U32 val);
    U32  FpgaRegPeekInd(S32 trdNo, S32 rgnum);
    void FpgaRegPokeDir(S32 trdNo, S32 rgnum, U32 val);
    U32  FpgaRegPeekDir(S32 trdNo, S32 rgnum);
    U32  FpgaWriteRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32  FpgaWriteRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32  FpgaReadRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32  FpgaReadRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32  FpgaBarRead(U32 bar, U32 offset);
    void FpgaBarWrite(U32 bar, U32 offset, U32 val);
    void FpgaBlockWrite(U32 nb, U32 reg, U32 val);
    U32  FpgaBlockRead(U32 nb, U32 reg);
    bool FpgaHwAddress(U08& hwAddr, U08& fpgaNum);
    bool FpgaDeviceID(U16& device_id);
    bool FpgaTemperature(float& t);

    int allocateDmaMemory(U32 DmaChan, BRDctrl_StreamCBufAlloc* param);
    int allocateDmaMemory(U32 DmaChan,
                          void** pBuf,
                          U32 blkSize,
                          U32 blkNum,
                          U32 isSysMem,
                          U32 dir,
                          U32 addr,
                          BRDstrm_Stub **pStub);
    int freeDmaMemory(U32 DmaChan);
    int startDma(U32 DmaChan, int isCycle);
    int stopDma(U32 DmaChan);
    int stateDma(U32 DmaChan, U32 msTimeout, int& state, U32& blkNum);
    int waitDmaBuffer(U32 DmaChan, U32 timeout);
    int waitDmaBlock(U32 DmaChan, U32 timeout);
    int resetDmaFifo(U32 DmaChan);
    int setDmaSource(U32 DmaChan, U32 src);
    int setDmaRequestFlag(U32 DmaChan, U32 flag);
    int setDmaDirection(U32 DmaChan, U32 dir);
    int adjustDma(U32 DmaChan, U32 adjust);
    int doneDma(U32 DmaChan, U32 done);
    void infoDma();
    bool writeBlock(U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer(U32 DmaChan, IPC_handle file, int fpos = 0);
    bool fpgaInfo(AMB_CONFIGURATION &info);
    bool fpgaBlock(unsigned startSearch, u16 id, fpga_block_t& block);
    bool fpgaTrd(unsigned startSearch, u16 id, fpga_trd_t& trd);
    Memory* ddr3() {return m_ddr;}

private:
    std::vector<Stream*>         m_strm;
    std::vector<fpga_block_t>    m_fpga_blocks;
    std::vector<fpga_trd_t>      m_fpga_trd;
    Memory*                      m_ddr;

    void createDmaChannels();
    void deleteDmaCannels();
    void scanFpgaBlocks();
    void scanFpgaTetrades();
    void createDDR3();

    bool dmaChannelInfo(U32 DmaChan, U32& dir, U32& FifoSize, U32& MaxDmaSize);
    Stream* stream(U32 DmaChan);

    bool syncFpga();
};

//-----------------------------------------------------------------------------

#endif // __FPGA_H__
