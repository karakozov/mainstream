#ifndef __ACDSP_H__
#define __ACDSP_H__

#include "gipcy.h"
#include "utypes.h"
#include "ddwambpex.h"
#include "ctrlstrm.h"
#include "memory.h"
#include "pe_chn_tx.h"
#include "pe_chn_rx.h"
#include "trd_check.h"

#ifndef USE_GUI
#include "nctable.h"
#endif

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define DSP_FPGA_COUNT            1
#define ADC_FPGA_COUNT            2
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
    void dataFromAdc(struct app_params_t& params);
    void dataFromMemAsFifo(struct app_params_t& params);
    void dataFromMemAsMem(struct app_params_t& params);
    void dataFromMain(struct app_params_t& params);
    void dataFromMainToMemAsFifo(struct app_params_t& params);
    void dataFromMainToMemAsMem(struct app_params_t& params);

    // EXIT
    void setExitFlag(bool exit);
    bool exitFlag();

    // TRD INTERFACE
    void RegPokeInd(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekInd(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void RegPokeDir(U32 fpgaNum, S32 trdNo, S32 rgnum, U32 val);
    U32 RegPeekDir(U32 fpgaNum, S32 trdNo, S32 rgnum);
    void resetFifo(U32 fpgaNum, U32 trd);

    //FPGA INTERFACE
    bool infoFpga(U32 fpgaNum, AMB_CONFIGURATION& info);

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
    U32 slotNumber() { return m_slotNumber; }

    // ISVI INTERFACE
    bool writeBlock(U32 fpgaNum, U32 DmaChan, IPC_handle file, int blockNumber);
    bool writeBuffer(U32 fpgaNum, U32 DmaChan, IPC_handle file, int fpos = 0);

    bool isFpgaDsp(U32 fpgaNum);
    bool isFpgaAdc(U32 fpgaNum);

    float getFpgaTemperature(U32 fpgaNum);

#ifndef USE_GUI
    void start_local_pcie_test(struct app_params_t& params);
#endif

    Fpga *FPGA(unsigned fpgaNum);
    const std::vector<Fpga*>& FPGA_LIST() { return m_fpga; }

    Memory *DDR3(unsigned fpgaNum);

    trd_check* get_trd_check() { return m_trd_check; }
    pe_chn_rx* get_chan_rx() { return m_rx; }
    pe_chn_tx* get_chan_tx(int id) { return m_tx[id%2]; }

private:
    std::vector<Fpga*>       m_fpga;
    i2c                     *m_iic;
    Si571                   *m_si571;
    BRDctrl_StreamCBufAlloc  m_sSCA;
    bool                     m_exit;
    U32                      m_slotNumber;

    trd_check*               m_trd_check;
    pe_chn_rx*               m_rx;
    pe_chn_tx*               m_tx[2];

    void createFpgaMemory();
    void deleteFpgaMemory();
};

#endif // __ACDSP_H__
