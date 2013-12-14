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

class acdsp
{
public:
    acdsp();
    virtual ~acdsp();

    int setSi57xFreq(float freq);
    int enableSwitchOut(unsigned mask);
    void initFpga(unsigned fpgaNum);

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
    int adjustDma(U32 fpgaNum, U32 DmaChan, U32 adjust);
    int doneDma(U32 fpgaNum, U32 DmaChan, U32 done);
    void infoDma(U32 fpgaNum);
    bool writeBlock(U32 fpgaNum, U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer(U32 fpgaNum, U32 DmaChan, IPC_handle file, int fpos = 0);

private:
    i2c *iic;
    Si571 *si571;
    std::vector<Fpga*>      m_fpga;

    void createFpgaDevices();
    void deleteFpgaDevices();
    Fpga *FPGA(unsigned fpgaNum);
};

#endif // __ACDSP_H__
