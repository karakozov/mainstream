
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
    explicit Stream(IPC_handle fpga, U08 hwAddr, U08 hwFpgaNum, unsigned channel);
    virtual ~Stream();

    void start();
    void stop();

    int allocateDmaMemory(BRDctrl_StreamCBufAlloc* sSCA);
    int allocateDmaMemory(void** pBuf,
                          U32 blkSize,
                          U32 blkNum,
                          U32 isSysMem,
                          U32 dir,
                          U32 addr,
                          BRDstrm_Stub **pStub);
    int freeDmaMemory();
    int startDma(int isCycle);
    int stopDma();
    int stateDma(U32 msTimeout, int& state, U32& blkNum);
    int waitDmaBuffer(U32 timeout);
    int waitDmaBlock(U32 timeout);
    int resetDmaFifo();
    int setDmaSource(U32 src);
    int setDmaDirection(U32 dir);
    int setDmaRequestFlag(U32 flag);
    int adjustDma(U32 adjust);
    int doneDma(U32 done);

    bool writeBlock(IPC_handle file, int blockNumber);
    bool writeBuffer(IPC_handle file, int fpos = 0);

private:

    IPC_handle      m_fpgaDev;
    U08             m_hwAddr;
    U08             m_hwFpgaNum;
    Mapper*         m_map;
    unsigned        m_DmaChan;
    unsigned        m_CMD;
    unsigned long   m_counter;
    bool            m_logInfo;
    bool            m_logErr;

    AMB_MEM_DMA_CHANNEL          *m_Descr;
    U32                          m_DescrSize;

#ifdef _WIN32
    OVERLAPPED  m_OvlStartStream;        // DMA buffer end event
    HANDLE      m_hBlockEndEvent;        // DMA block end event
#endif

    static thread_value __IPC_API stream_thread(void *params);
};

//-----------------------------------------------------------------------------

struct StreamParam {
    class Stream            *strm;
    AMB_MEM_DMA_CHANNEL     *dscr;
    U32                     timeout;
    IPC_handle              dataFile;
};

//-----------------------------------------------------------------------------


#endif //__STREAMADC_H__
