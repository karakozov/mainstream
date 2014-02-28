#ifndef __ACDSP_H__
#define __ACDSP_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"
#include "pe_chn_tx.h"
#include "pe_chn_rx.h"
#include "trd_check.h"
#include "nctable.h"

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define ACDSP_FPGA_COUNT          3
#define DMA_CHANNEL_NUM           4

//-----------------------------------------------------------------------------

class i2c;
class Si571;
class Fpga;
class Memory;

class acdsp
{
public:
    acdsp(std::vector<Fpga*>& fpgaList);
    virtual ~acdsp();

    // Si571 INTERFACE
    int setSi57xFreq(float freq);
    int enableSwitchOut(unsigned mask);

    // DATA INTERFACE
    void dataFromAdc(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMemAsFifo(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMemAsMem(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMain(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMainToMemAsFifo(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);
    void dataFromMainToMemAsMem(struct app_params_t& params, IPC_handle isviFile, const char *flgName, BRDctrl_StreamCBufAlloc& sSCA);

    // EXIT
    void setExitFlag(bool exit);
    bool exitFlag();

    // TRD INTERFACE
    void RegPokeInd(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void RegPokeDir(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void resetFifo(U32 fpgaNum, U32 trd);

    // DMA INTERFACE
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
    int adjustDma(U32 fpgaNum, U32 DmaChan, U32 adjust);
    int doneDma(U32 fpgaNum, U32 DmaChan, U32 done);
    void infoDma(U32 fpgaNum);

    // ISVI INTERFACE
    bool writeBlock(U32 fpgaNum, U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer(U32 fpgaNum, U32 DmaChan, IPC_handle file, int fpos = 0);
    void start_local_pcie_test(struct app_params_t& params);

    Fpga *FPGA(unsigned fpgaNum);
    const std::vector<Fpga*>& FPGA_LIST() { return m_fpga; }

private:
    i2c                     *m_iic;
    Si571                   *m_si571;
    std::vector<Fpga*>       m_fpga;
    std::vector<Memory*>     m_ddr;
    BRDctrl_StreamCBufAlloc  m_sSCA;
    bool                     m_exit;
    bool                     m_cleanup;
    table*                   m_t;

    void createFpgaMemory();
    void deleteFpgaMemory();

    Memory *DDR3(unsigned fpgaNum);
    //Fpga *FPGA(unsigned fpgaNum);
};

#endif // __ACDSP_H__
