
#ifndef __STREAM_H__
#define __STREAM_H__

#include <vector>
#include <string>
#include <sstream>

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "utypes.h"
#include "ddwambpex.h"
#include "dbglog.h"
#include "ctrlstrm.h"
#include "mapper.h"

//-----------------------------------------------------------------------------

struct exception_t {
    std::string str;
};

//-----------------------------------------------------------------------------
#define CMD_START 0x55
#define CMD_STOP  0xAA
//-----------------------------------------------------------------------------

class Stream {

public:
    explicit Stream(IPC_handle fpga, unsigned channel);
    virtual ~Stream();

    void start();
    void stop();

    int allocateDmaMemory(BRDctrl_StreamCBufAlloc* sSCA);
    int allocateDmaMemory(void** pBuf,
                          u32 blkSize,
                          u32 blkNum,
                          u32 isSysMem,
                          u32 dir,
                          u32 addr,
                          BRDstrm_Stub **pStub);
    int freeDmaMemory();
    int startDma(int isCycle);
    int stopDma();
    int stateDma(u32 msTimeout, int& state, u32& blkNum);
    int waitDmaBuffer(u32 timeout);
    int waitDmaBlock(u32 timeout);
    int resetDmaFifo();
    int setDmaSource(u32 src);
    int setDmaDirection(u32 dir);
    int adjustDma(u32 adjust);
    int doneDma(u32 done);

    bool writeBlock(IPC_handle file, int blockNumber);
    bool writeBuffer(IPC_handle file, int fpos = 0);

private:

    IPC_handle      m_fpgaDev;
    unsigned        m_DmaChan;
    unsigned        m_CMD;
    unsigned long   m_counter;
    bool            m_logInfo;
    bool            m_logErr;
    Mapper          m_map;

    AMB_MEM_DMA_CHANNEL          *m_Descr;
    u32                          m_DescrSize;

    static thread_value __IPC_API stream_thread(void *params);
};

//-----------------------------------------------------------------------------

struct StreamParam {
    class Stream            *strm;
    AMB_MEM_DMA_CHANNEL     *dscr;
    u32                     timeout;
    IPC_handle              dataFile;
};

//-----------------------------------------------------------------------------


#endif //__STREAMADC_H__
