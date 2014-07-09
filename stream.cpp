
#include "stream.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#else
#ifdef _WIN64
#define snprintf		_snprintf
#endif
#endif

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------

template <typename T>
string toString ( T Number )
{
    ostringstream ss;
    ss << Number;
    return ss.str();
}

//-----------------------------------------------------------------------------

Stream::Stream(IPC_handle handle, U08 hwAddr, U08 hwFpgaNum, unsigned channel) :
    m_fpgaDev(handle), m_hwAddr(hwAddr), m_hwFpgaNum(hwFpgaNum), m_DmaChan(channel)
{
    m_logInfo = false;
    m_logErr = true;
    m_Descr = 0;

#ifdef _c6x_
    m_map = new Mapper();
#else
    m_map = new Mapper(m_fpgaDev);
#endif

#ifdef _WIN32
    char nameEvent[MAX_PATH];
    snprintf(nameEvent, sizeof(nameEvent), "event_DmaChan_%x_%x_%d", m_hwAddr, m_hwFpgaNum, m_DmaChan);
    m_hBlockEndEvent = CreateEventA(0, true, true, nameEvent);
    m_OvlStartStream.hEvent = CreateEventA(0, true, true, 0);
#endif

    stopDma();
    resetDmaFifo();
}

//-----------------------------------------------------------------------------

Stream::~Stream()
{
#ifdef _WIN32
    CloseHandle(m_hBlockEndEvent);
    CloseHandle(m_OvlStartStream.hEvent);
#endif

    if(m_map) delete m_map;
}

//-----------------------------------------------------------------------------

int Stream::allocateDmaMemory(BRDctrl_StreamCBufAlloc* sSCA)
{
    m_DescrSize = sizeof(AMB_MEM_DMA_CHANNEL) + (sSCA->blkNum - 1) * sizeof(void*);
    m_Descr = (AMB_MEM_DMA_CHANNEL*) new u8[m_DescrSize];

    m_Descr->DmaChanNum = m_DmaChan;
    m_Descr->Direction = sSCA->dir;
    m_Descr->LocalAddr = 0;
    m_Descr->MemType = sSCA->isCont;
    m_Descr->BlockCnt = sSCA->blkNum;
    m_Descr->BlockSize = sSCA->blkSize;
    m_Descr->pStub = NULL;
#ifdef _WIN32
    m_Descr->hBlockEndEvent = m_hBlockEndEvent;
#else
    m_Descr->hBlockEndEvent = 0;
#endif

    for(U32 iBlk = 0; iBlk < sSCA->blkNum; iBlk++) {
        m_Descr->pBlock[iBlk] = NULL;
    }

    if( IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_SET_MEMIO, m_Descr, m_DescrSize, m_Descr, m_DescrSize) < 0 ) {
        throw except_info("%s, %d: %s() - Error allocate buffer for DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_DmaChan);
    }

    for(U32 iBlk = 0; iBlk < m_Descr->BlockCnt; iBlk++) {

        void *MappedAddress = m_map->mapPhysicalAddress(m_Descr->pBlock[iBlk], m_Descr->BlockSize);
        if(!MappedAddress) {
            throw except_info("%s, %d: %s() - Error map physical address: 0x%.8x\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->pBlock[iBlk]);
        }

        fprintf(stderr, "%d: %p -> %p\n", iBlk, (void*)m_Descr->pBlock[iBlk], MappedAddress);

        m_Descr->pBlock[iBlk] = MappedAddress;
        sSCA->ppBlk[iBlk] = MappedAddress;
    }

    if(m_Descr->pStub) {

        void *StubAddress = m_map->mapPhysicalAddress(m_Descr->pStub, sizeof(BRDstrm_Stub));

        if(!StubAddress) {
            throw except_info("%s, %d: %s() - Error map stub: 0x%.8x\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->pStub);
        }

        fprintf(stderr, "Stub: %p -> %p\n", (void*)m_Descr->pStub, StubAddress);

        m_Descr->pStub = StubAddress;
        sSCA->pStub = (BRDstrm_Stub*)m_Descr->pStub;
    }

    sSCA->blkNum = m_Descr->BlockCnt;

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::allocateDmaMemory(void** pBuf,
                              U32 blkSize,
                              U32 blkNum,
                              U32 isSysMem,
                              U32 dir,
                              U32 addr,
                              BRDstrm_Stub **pStub )
{
    m_DescrSize = sizeof(AMB_MEM_DMA_CHANNEL) + (blkNum - 1) * sizeof(void*);
    m_Descr = (AMB_MEM_DMA_CHANNEL*) new u8[m_DescrSize];

    m_Descr->DmaChanNum = m_DmaChan;
    m_Descr->Direction = dir;
    m_Descr->LocalAddr = addr;
    m_Descr->MemType = isSysMem;
    m_Descr->BlockCnt = blkNum;
    m_Descr->BlockSize = blkSize;
    m_Descr->pStub = NULL;
#ifdef _WIN32
    m_Descr->hBlockEndEvent = m_hBlockEndEvent;
#else
    m_Descr->hBlockEndEvent = 0;
#endif

    for(U32 iBlk = 0; iBlk < blkNum; iBlk++) {
        m_Descr->pBlock[iBlk] = NULL;
    }

    if( IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_SET_MEMIO, m_Descr, m_DescrSize, m_Descr, m_DescrSize) < 0 ) {
        throw except_info("%s, %d: %s() - Error allocate buffer for DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_DmaChan);
    }

    for(U32 iBlk = 0; iBlk < m_Descr->BlockCnt; iBlk++) {

        void *MappedAddress = m_map->mapPhysicalAddress(m_Descr->pBlock[iBlk], m_Descr->BlockSize);
        if(!MappedAddress) {
            throw except_info("%s, %d: %s() - Error map physical address: 0x%.8x\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->pBlock[iBlk]);
        }

        fprintf(stderr, "%d: %p -> %p\n", iBlk, (void*)m_Descr->pBlock[iBlk], MappedAddress);

        m_Descr->pBlock[iBlk] = MappedAddress;
        pBuf[iBlk] = m_Descr->pBlock[iBlk];
    }

    if(m_Descr->pStub) {

        void *StubAddress = m_map->mapPhysicalAddress(m_Descr->pStub, sizeof(BRDstrm_Stub));

        if(!StubAddress) {
            throw except_info("%s, %d: %s() - Error map stub: 0x%.8x\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->pStub);
        }

        fprintf(stderr, "Stub: %p -> %p\n", (void*)m_Descr->pStub, StubAddress);

        m_Descr->pStub = StubAddress;
        *pStub = (BRDstrm_Stub*)m_Descr->pStub;
    }

    return 0;
}

//-----------------------------------------------------------------------------

int  Stream::freeDmaMemory()
{
    if(m_Descr) {

#ifdef _WIN32
        IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_FREE_MEMIO, m_Descr, m_DescrSize, 0, 0);
#else
        int res = IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_FREE_MEMIO, m_Descr, m_DescrSize, 0, 0);
        if(res < 0)
        {
            throw except_info("%s, %d: %s() - Error free buffer for DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
#endif
        delete[] m_Descr;
        m_Descr = NULL;
    }

    return 0;
}

//-----------------------------------------------------------------------------

int  Stream::startDma(int IsCycling)
{
    if(m_Descr)
    {
        AMB_START_DMA_CHANNEL StartDescrip;
        StartDescrip.DmaChanNum = m_DmaChan;
        StartDescrip.IsCycling = IsCycling;

#ifdef _WIN32
        ResetEvent(m_Descr->hBlockEndEvent);
        ResetEvent(m_OvlStartStream.hEvent);

        int res = IPC_ioctlDeviceOvl(m_fpgaDev, IOCTL_AMB_START_MEMIO,
                                      &StartDescrip,
                                      sizeof(StartDescrip),0,0,
                                      &m_OvlStartStream);
        return res;
#else
        int res = IPC_ioctlDevice(m_fpgaDev,
                                  IOCTL_AMB_START_MEMIO,
                                  &StartDescrip,
                                  sizeof(StartDescrip),0,0);
        if (res < 0)
        {
            throw except_info("%s, %d: %s() - Error start DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
#endif
    }
    return 0;
}

//-----------------------------------------------------------------------------

int  Stream::stopDma()
{
    if(m_Descr)
    {
        BRDstrm_Stub* pStub = (BRDstrm_Stub*)m_Descr->pStub;
        if(pStub) {
            if(pStub->state == BRDstrm_STAT_RUN)
            {
                AMB_STATE_DMA_CHANNEL StateDescrip;
                StateDescrip.DmaChanNum = m_DmaChan;
                StateDescrip.Timeout = 0;

                if (IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_STOP_MEMIO,&StateDescrip,sizeof(StateDescrip),&StateDescrip,sizeof(StateDescrip)) < 0)
                {
                    throw except_info("%s, %d: %s() - Error stop DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
                }
            }
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------

int  Stream::stateDma(U32 msTimeout, int& state, U32& blkNum)
{
    AMB_STATE_DMA_CHANNEL StateDescrip;
    StateDescrip.DmaChanNum = m_DmaChan;
    StateDescrip.Timeout = msTimeout;

    if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_STATE_MEMIO,&StateDescrip,sizeof(StateDescrip),&StateDescrip,sizeof(StateDescrip)))
    {
        throw except_info("%s, %d: %s() - Error get DMA %d state\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
    }
    blkNum = StateDescrip.BlockCntTotal;
    state = StateDescrip.DmaChanState;

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::waitDmaBuffer(U32 msTimeout)
{
    if(m_Descr)
    {
#ifdef _WIN32
        int res = WaitForSingleObject(m_OvlStartStream.hEvent, msTimeout);
        if(res != WAIT_OBJECT_0) {
            return res;
        }
#else
        AMB_STATE_DMA_CHANNEL StateDescrip;
        StateDescrip.DmaChanNum = m_DmaChan;
        StateDescrip.Timeout = msTimeout;

        int res = IPC_ioctlDevice(m_fpgaDev,
                                  IOCTL_AMB_WAIT_DMA_BUFFER,
                                  &StateDescrip,
                                  sizeof(StateDescrip),
                                  &StateDescrip,
                                  sizeof(StateDescrip));
        if (0 > res)
        {
            fprintf(stderr, "%s(): Error wait buffer DMA\n", __FUNCTION__ );
            return -1;
        }
#endif
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::waitDmaBlock(U32 msTimeout)
{
    if(m_Descr)
    {
#ifdef _WIN32
        int res = WaitForSingleObject(m_Descr->hBlockEndEvent, msTimeout);
        if(res != WAIT_OBJECT_0) {
            return res;
        }
#else
        AMB_STATE_DMA_CHANNEL StateDescrip;
        StateDescrip.DmaChanNum = m_DmaChan;
        StateDescrip.Timeout = msTimeout;

        int res = IPC_ioctlDevice(m_fpgaDev,
                                  IOCTL_AMB_WAIT_DMA_BLOCK,
                                  &StateDescrip,
                                  sizeof(StateDescrip),
                                  &StateDescrip,
                                  sizeof(StateDescrip));
#endif
        if (0 > res)
        {
            fprintf(stderr, "%s(): Error wait block DMA\n", __FUNCTION__ );
            return -1;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::resetDmaFifo()
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = 0;

    if(m_Descr)
    {
        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_RESET_FIFO, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            fprintf(stderr, "%s(): Error reset FIFO\n", __FUNCTION__ );
            return -1;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::setDmaSource(U32 addr)
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = addr;

    if(m_Descr)
    {
        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_SET_SRC_MEMIO, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            throw except_info("%s, %d: %s() - Error set DMA %d source\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::setDmaRequestFlag(U32 flag)
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = flag;

    if(m_Descr)
    {
        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_SET_DRQ_MEMIO, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            throw except_info("%s, %d: %s() - Error set DMA %d request flag\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::setDmaDirection(U32 direction)
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = direction;

    if(m_Descr)
    {
        SetDescrip.Param = direction;

        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_SET_DIR_MEMIO, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            throw except_info("%s, %d: %s() - Error set DMA %d direction\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::adjustDma(U32 mode)
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = mode;

    if(m_Descr)
    {
        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_ADJUST, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            throw except_info("%s, %d: %s() - Error adjust DMA %d\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

int Stream::doneDma(U32 blockNumber)
{
    AMB_SET_DMA_CHANNEL SetDescrip;
    SetDescrip.DmaChanNum = m_DmaChan;
    SetDescrip.Param = blockNumber;

    if(m_Descr)
    {
        if (0 > IPC_ioctlDevice(m_fpgaDev, IOCTL_AMB_DONE, &SetDescrip, sizeof(SetDescrip), 0, 0))
        {
            throw except_info("%s, %d: %s() - Error set DMA %d done\n", __FILE__, __LINE__, __FUNCTION__, m_Descr->DmaChanNum);
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

thread_value __IPC_API Stream::stream_thread(void *params)
{
    StreamParam *param = reinterpret_cast<StreamParam*>(params);
    if(!param) {
        fprintf(stderr, "%s(): Invalid parameters\n", __FUNCTION__);
        return 0;
    }

    Stream *strm = reinterpret_cast<Stream*>(param->strm);
    if(!strm) {
        fprintf(stderr, "%s(): Invalid class pointer\n", __FUNCTION__);
        return 0;
    }

    unsigned N = param->dscr->BlockCnt;
    BRDctrl_StreamStub *stub = reinterpret_cast<BRDctrl_StreamStub*>(param->dscr->pStub);
    IPC_handle file = param->dataFile;

    if(!stub) {
        fprintf(stderr, "%s(): Invalid stub pointer\n", __FUNCTION__);
        return 0;
    }

    fprintf(stderr, "Waiting DMA interrupt...\n");

    unsigned long processedBlock = 0;
    int err = 0;

    //struct timeval start;
    //time_start(&start);

    while(strm->m_CMD == CMD_START)
    {

        // Wait interrupt event after data block filled
        //err =  strm->waitDmaBuffer(param->timeout);
        err =  strm->waitDmaBlock(param->timeout);
        if(err < 0) {

            continue;

        } else {

            if(!strm->writeBlock(file, processedBlock)) {
                fprintf(stderr, "Error write %ld data block\n", processedBlock);
                break;
            }

            fprintf(stderr, "processedBlock = %ld\n", processedBlock);
            fprintf(stderr, "stub->totalCounter = %d\n", stub->totalCounter);
            fprintf(stderr, "stub->lastBlock = %d\n", stub->lastBlock);

            processedBlock++;

            if(processedBlock == N) {
                processedBlock = 0;
            }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

bool Stream::writeBlock(IPC_handle file, int blockNumber)
{
    void *currentBlock = m_Descr->pBlock[blockNumber];

    int count = IPC_writeFile(file, currentBlock, m_Descr->BlockSize);
    if(count == -1) {
        fprintf(stderr, "Error write %d data block (%d)n", blockNumber, count);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

bool Stream::writeBuffer(IPC_handle file, int fpos)
{
    IPC_setPosFile(file, fpos, SEEK_SET);

    for(unsigned i=0; i<m_Descr->BlockCnt; i++) {
        if(!writeBlock(file, i)) {
            fprintf(stderr, "Error write %d data block\n", i);
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
/*
bool Stream::prepareBuffer()
{
    int res = -1;
    long pageSize = sysconf(_SC_PAGESIZE);

    res = posix_memalign(&m_Buffer, pageSize, m_Descr->BlockSize * m_Descr->BlockCnt);
    if((res != 0) || !m_Buffer) {
        exception_t err = {"Error in prepareBuffers()"};
        fprintf(stderr, "%s(): %s\n", __FUNCTION__, err.str.c_str());
        throw err;
    }

    return true;
}
*/
//-----------------------------------------------------------------------------

