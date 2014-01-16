#ifndef __FPGA_H__
#define __FPGA_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "fpga_base.h"
#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"
#include "mapper.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define DMA_CHANNEL_NUM  4
#define TRD_NUM          8

//-----------------------------------------------------------------------------

class Stream;
class Memory;

//-----------------------------------------------------------------------------

class Fpga : public fpga_base
{
public:
    explicit Fpga(unsigned id);
    virtual ~Fpga();

    void init();
    Memory* DDR3();
    void resetFifo(unsigned trd);
    void resetTrd(unsigned trd);
    int trd_number(unsigned trdID);

    void FpgaRegPokeInd(S32 trdNo, S32 rgnum, U32 val);
    U32 FpgaRegPeekInd(S32 trdNo, S32 rgnum);
    void FpgaRegPokeDir(S32 trdNo, S32 rgnum, U32 val);
    U32 FpgaRegPeekDir(S32 trdNo, S32 rgnum);
    U32 FpgaWriteRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32 FpgaWriteRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32 FpgaReadRegBuf(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);
    U32 FpgaReadRegBufDir(U32 TetrNum, U32 RegNum, void* RegBuf, U32 RegBufSize);

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
    bool setMemory(U32 mem_mode, U32 PretrigMode, U32& PostTrigSize, U32& Buf_size);
    bool fpgaInfo(AMB_CONFIGURATION &info);

    bool writeSPD(unsigned id, unsigned data_address, unsigned data_value, int imeout);
    bool readSPD(unsigned id, unsigned data_address, unsigned& data_value, int imeout);

private:
    std::vector<unsigned>   m_trd;
    std::vector<Stream*>    m_strm;
    Memory                 *m_ddr;

    void createDmaChannels();
    void deleteDmaCannels();
    void createMemory();
    void deleteMemory();
    void scanFpgaTetrades();

    bool dmaChannelInfo(U32 DmaChan, U32& dir, U32& FifoSize, U32& MaxDmaSize);
    Stream* stream(U32 DmaChan);
};

//-----------------------------------------------------------------------------

#endif // __FPGA_H__
