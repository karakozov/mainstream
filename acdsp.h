#ifndef __ACDSP_H__
#define __ACDSP_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "utypes.h"
#include "ddwambpex.h"
#include "dbglog.h"
#include "ctrlstrm.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define FPGA_COUNT          3
#define DMA_CHANNEL_NUM     4

//-----------------------------------------------------------------------------

class i2c;
class Si571;
class Fpga;
class Memory;

class acdsp
{
public:
    acdsp();
    virtual ~acdsp();

    int setSi57xFreq(float freq);
    int enableSwitchOut(unsigned mask);
    Fpga *FPGA(unsigned fpgaNum);

    void RegPokeInd(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void RegPokeDir(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(U32 fpgaNum, S32 trdNo, S32 rgnum);

    int allocateDmaMemory(U32 fpgaNum, U32 DmaChan, BRDctrl_StreamCBufAlloc* param);
    int allocateDmaMemory(U32 fpgaNum, U32 DmaChan,
                          void** pBuf,
                          U32 blkSize,
                          U32 blkNum,
                          U32 isSysMem,
                          U32 dir,
                          U32 addr,
                          BRDstrm_Stub **pStub);
    int freeDmaMemory(U32 fpgaNum, U32 DmaChan);
    int startDma(U32 fpgaNum, U32 DmaChan, int isCycle);
    int stopDma(U32 fpgaNum, U32 DmaChan);
    int stateDma(U32 fpgaNum, U32 DmaChan, U32 msTimeout, int& state, U32& blkNum);
    int waitDmaBuffer(U32 fpgaNum, U32 DmaChan, U32 timeout);
    int waitDmaBlock(U32 fpgaNum, U32 DmaChan, U32 timeout);
    int resetDmaFifo(U32 fpgaNum, U32 DmaChan);
    int setDmaSource(U32 fpgaNum, U32 DmaChan, U32 src);
    int setDmaDirection(U32 fpgaNum, U32 DmaChan, U32 dir);
    int setDmaRequestFlag(U32 fpgaNum, U32 DmaChan, U32 flag);
    //-----------------------------------------------------------------------------
    int adjustDma(U32 fpgaNum, U32 DmaChan, U32 adjust);
    int doneDma(U32 fpgaNum, U32 DmaChan, U32 done);
    void infoDma(U32 fpgaNum);
    bool writeBlock(U32 fpgaNum, U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer(U32 fpgaNum, U32 DmaChan, IPC_handle file, int fpos = 0);
    void resetFifo(U32 fpgaNum, U32 trd);

    void dataFromAdc(U32 fpgaNum, U32 DmaChan, U32 AdcMask, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMemAsFifo(U32 fpgaNum, U32 DmaChan, U32 AdcMask, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMemAsMem(U32 fpgaNum, U32 DmaChan, U32 AdcMask, U32 BufferCounter, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);

    void setExitFlag(bool exit);
    bool exitFlag();


private:
    i2c                     *m_iic;
    Si571                   *m_si571;
    std::vector<Fpga*>       m_fpga;
    BRDctrl_StreamCBufAlloc  m_sSCA;
    bool                     m_exit;

    void createFpgaDevices();
    void deleteFpgaDevices();
};

#endif // __ACDSP_H__
