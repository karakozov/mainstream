#include "adc_test_thread.h"
#include "isvi.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

adc_test_thread::adc_test_thread(std::vector<acdsp*>& boardList, acsync* sync) : m_boardList(boardList), m_sync(sync)
{
    m_start = false;
}

//-----------------------------------------------------------------------------

adc_test_thread::~adc_test_thread()
{
    m_start = false;
    QThread::wait();
}

//-----------------------------------------------------------------------------

void adc_test_thread::start_adc_test(bool start, struct app_params_t& params)
{
    m_start = start;
    m_params = params;

    if(m_start) {
        QThread::start();
    } else {
        m_start = false;
        QThread::wait();
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::prepareIsvi(isvidata_t& isviFiles, isviflg_t& flgNames, isvihdr_t& isviHdrs)
{
    unsigned N = m_boardList.size();

    //--------------------------------------------------------------
    //Prepare data and flag files for ISVI and counters
    for(int i=(N-1); i>=0; i--) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        datafiles_t isviFile;
        flgnames_t flgName;
        hdr_t hdr;

        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(((m_params.fpgaMask >> j) & 0x1)) {

                char fname[64];
                snprintf(fname, sizeof(fname), "CH%d_%d.bin", brd->slotNumber()-2, j);
                isviFile.push_back(createDataFile(fname));

                char tmpflg[64];
                snprintf(tmpflg, sizeof(tmpflg), "CH%d_%d.flg", brd->slotNumber()-2, j);
                createFlagFile(tmpflg);
                flgName.push_back(tmpflg);

                string s;
                createIsviHeader(s, brd->slotNumber(), j, m_params);
                hdr.push_back(s);
            }
        }

        isviFiles.push_back(isviFile);
        flgNames.push_back(flgName);
        isviHdrs.push_back(hdr);
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::prepareDma(vector<brdbuf_t>& dmaBuffers)
{
    unsigned N = m_boardList.size();

    //-------------------------------------------------------------
    // Allocate DMA buffers for all board and all onboard FPGA ADC
    for(int i=(N-1); i>=0; i--) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        // allocate DMA buffer for 2 FPGA ADC
        vector<dmabuf_t> Buffers;
        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(((m_params.fpgaMask >> j) & 0x1)) {

                dmabuf_t dmaBlocks(m_params.dmaBlockCount, 0);
                BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, m_params.dmaBlockCount, m_params.dmaBlockSize, dmaBlocks.data(), NULL, };
                brd->allocateDmaMemory(j, m_params.dmaChannel, &sSCA);
                Buffers.push_back(dmaBlocks);
            }
        }

        // save board buffers for later using
        dmaBuffers.push_back(Buffers);
    }
}

//-----------------------------------------------------------------------------

U32 calc_stmode(const struct app_params_t& params)
{
    U32 stmode = 0x0;

    if(params.adcStart == 0x2) {
        stmode |= 0x87;
    } else {
        stmode &= ~0x80;
    }

    if(params.adcStartInv) {
        stmode |= 0x40;
    } else {
        stmode &= ~0x40;
    }

    return stmode;
}

//-----------------------------------------------------------------------------

void adc_test_thread::startAdcDma()
{
    unsigned N = m_boardList.size();

    if(m_sync) m_sync->ResetSync(true);

    for(int i=(N-1); i>=0; i--) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(((m_params.fpgaMask >> j) & 0x1)) {

                brd->setDmaSource(j, m_params.dmaChannel, ADC_TRD);
                brd->setDmaDirection(j, m_params.dmaChannel, BRDstrm_DIR_IN);
                brd->RegPokeInd(j, ADC_TRD, 0x10, m_params.adcMask);

                U32 stmode = calc_stmode(m_params);

                brd->RegPokeInd(j, ADC_TRD, 0x5, stmode);
                brd->RegPokeInd(j, ADC_TRD, 0x17, (m_params.adcStart << 4));
                brd->startDma(j, m_params.dmaChannel, 0);
                brd->RegPokeInd(j, ADC_TRD, 0, 0x2038);
                IPC_delay(1);
                U32 mode0 = brd->RegPeekInd(j, ADC_TRD, 0);
                mode0 = mode0;
            }
        }
    }

    if(m_sync) m_sync->ResetSync(false);
}

//-----------------------------------------------------------------------------

void adc_test_thread::stopAdcDma()
{
    unsigned N = m_boardList.size();

    // Stop ADC and DMA channels for non masked FPGA
    // free DMA buffers, close ISVI data file
    for(int i=(N-1); i>=0; i--) {

        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        //datafiles_t isviFile = isviFiles.at(i);
        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(((m_params.fpgaMask >> j) & 0x1)) {

                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                IPC_delay(10);
                brd->freeDmaMemory(j, m_params.dmaChannel);
                //IPC_closeFile(isviFile.at(j)); !!!!!!!!!!!!!!
            }
        }
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::startAdcDmaMem()
{
    unsigned N = m_boardList.size();

    U32 MemBufSize = m_params.dmaBuffersCount * m_params.dmaBlockSize * m_params.dmaBlockCount;
    U32 PostTrigSize = 0;

    if(m_sync) m_sync->ResetSync(true);

    for(int i=(N-1); i>=0; i--) {

        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        // prepare and start ADC and MEM for non masked FPGA
        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(!((m_params.fpgaMask >> j) & 0x1))
                continue;

            brd->DDR3(j)->Enable(false);

            brd->setDmaSource(j, m_params.dmaChannel, MEM_TRD);
            brd->setDmaRequestFlag(j, m_params.dmaChannel, BRDstrm_DRQ_HALF);

            //brd->RegPokeInd(j, ADC_TRD, 0xC, 0x100);

            brd->resetFifo(j, ADC_TRD);
            brd->resetFifo(j, MEM_TRD);
            brd->resetDmaFifo(j, m_params.dmaChannel);
            brd->RegPokeInd(j, ADC_TRD, 0x10, m_params.adcMask);

            brd->DDR3(j)->setMemory(1, 0, PostTrigSize, MemBufSize);

            U32 stmode = calc_stmode(m_params);

            brd->RegPokeInd(j, ADC_TRD, 0x5, stmode);
            brd->RegPokeInd(j, ADC_TRD, 0x17, (m_params.adcStart << 4));
            brd->resetFifo(j, ADC_TRD);
            brd->resetFifo(j, MEM_TRD);
            brd->RegPokeInd(j, MEM_TRD, 0x0, 0x2038);
            brd->RegPokeInd(j, ADC_TRD, 0x0, 0x2038);

            IPC_delay(1);
            U32 mode01 = brd->RegPeekInd(j, ADC_TRD, 0);
            U32 mode02 = brd->RegPeekInd(j, MEM_TRD, 0);
            mode01 = mode01;
            mode02 = mode02;
        }
    }

    if(m_sync) m_sync->ResetSync(false);
}

//-----------------------------------------------------------------------------

void adc_test_thread::stopAdcDmaMem()
{
    unsigned N = m_boardList.size();

    // Stop ADC, MEM and DMA channels for non masked FPGA
    // free DMA buffers, close ISVI data file
    for(int i=(N-1); i>=0; i--) {

        acdsp* brd = m_boardList.at(i);

        if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
            continue;

        //datafiles_t isviFile = isviFiles.at(i);
        for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

            if(brd->isFpgaDsp(j))
                continue;

            if(((m_params.fpgaMask >> j) & 0x1)) {

                brd->RegPokeInd(j, MEM_TRD, 0x0, 0x0);
                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                IPC_delay(10);
                brd->freeDmaMemory(j, m_params.dmaChannel);
                //IPC_closeFile(isviFile.at(j)); !!!!!!!!!!!!!! TODO
            }
        }
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::dataFromMemAsMem()
{
    unsigned N = m_boardList.size();

    //-------------------------------------------------------------
    // Allocate DMA buffers for all board and all onboard FPGA ADC
    vector<brdbuf_t> dmaBuffers;
    prepareDma(dmaBuffers);

    //--------------------------------------------------------------
    //Prepare data and flag files for ISVI and counters
    isvidata_t isviFiles;
    isviflg_t  flgNames;
    isvihdr_t isviHdrs;
    prepareIsvi(isviFiles, flgNames, isviHdrs);

    unsigned pass_counter = 0;

    while(m_start) {

        startAdcDmaMem();

        //select resource for enabled board only
        unsigned brd_index = 0;

        //for(unsigned i=0; i<N; i++) {
        for(int i=(N-1); i>=0; i--) {

            acdsp* brd = m_boardList.at(i);

            if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
                continue;

            datafiles_t isviFile = isviFiles.at(brd_index);
            flgnames_t flgName = flgNames.at(brd_index);
            hdr_t hdrs = isviHdrs.at(brd_index);
            QString info;

            if(!m_start)
                break;

            //select resource for enabled FPGA only
            unsigned fpga_index = 0;

            // Save MEM data in ISVI file for non masked FPGA
            for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

                if(brd->isFpgaDsp(j))
                    continue;

                if(!((m_params.fpgaMask >> j) & 0x1))
                    continue;

                if(!m_start)
                    break;

                for(unsigned counter = 0; counter < m_params.dmaBuffersCount; counter++) {

                    brd->startDma(j, m_params.dmaChannel, 0x0);

                    int res = brd->waitDmaBuffer(j, m_params.dmaChannel, 2000);
                    if( res != 0 ) {

                        u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                        u32 status_mem = brd->RegPeekDir(j, MEM_TRD, 0x0);
                        info = QString::number(pass_counter) + " - ";
                        info += " BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                        info += " - ERROR TIMEOUT! ADC STATUS = 0x" + QString::number(status_adc,16) + " MEM_STATUS = 0x" + QString::number(status_mem,16);
                        emit updateInfo(info);
                        break;

                    } else {

                        brd->writeBuffer(j, m_params.dmaChannel, isviFile.at(fpga_index), counter * m_params.dmaBlockSize * m_params.dmaBlockCount);
                        info = QString::number(pass_counter) + " - ";
                        info += " BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                        info += " - Write DMA buffer: " + QString::number(counter);
                        emit updateInfo(info);
                    }

                    brd->stopDma(j, m_params.dmaChannel);
                }

                string h = hdrs.at(fpga_index);
                IPC_writeFile(isviFile.at(fpga_index), (void*)h.c_str(), h.size());
                lockDataFile(flgName.at(fpga_index).c_str(), pass_counter);

                brd->RegPokeInd(j, MEM_TRD, 0x0, 0x0);
                brd->RegPokeInd(j, ADC_TRD, 0x0, 0x0);
                brd->resetFifo(j, ADC_TRD);
                brd->resetFifo(j, MEM_TRD);
                brd->resetDmaFifo(j, m_params.dmaChannel);

                ++fpga_index;
            }
            ++brd_index;
            emit updateInfo("");
        }
        ++pass_counter;
        IPC_delay(500);
    }

    stopAdcDmaMem();
    IPC_delay(500);
}

//-----------------------------------------------------------------------------

void adc_test_thread::dataFromAdc()
{
    unsigned N = m_boardList.size();

    //-------------------------------------------------------------
    // Allocate DMA buffers for all board and all onboard FPGA ADC
    vector<brdbuf_t> dmaBuffers;
    prepareDma(dmaBuffers);

    //--------------------------------------------------------------
    //Prepare data and flag files for ISVI and counters
    isvidata_t isviFiles;
    isviflg_t  flgNames;
    isvihdr_t isviHdrs;
    prepareIsvi(isviFiles, flgNames, isviHdrs);

    //---------------------------------------------------------------

    unsigned pass_counter = 0;

    while(m_start) {

        startAdcDma();

        //select resource for enabled board only
        unsigned brd_index = 0;

        for(int i=(N-1); i>=0; i--) {

            acdsp* brd = m_boardList.at(i);

            if(!((m_params.boardMask >> (brd->slotNumber()-1)) & 0x1))
                continue;

            vector<dmabuf_t> Buffers = dmaBuffers.at(brd_index);
            datafiles_t isviFile = isviFiles.at(brd_index);
            flgnames_t flgName = flgNames.at(brd_index);
            hdr_t hdrs = isviHdrs.at(brd_index);
            QString info;

            //select resource for enabled FPGA only
            unsigned fpga_index = 0;

            // save ADC data into ISVI files for non masked FPGA
            for(int j=brd->FPGA_LIST().size()-1; j>=0; j--) {

                if(brd->isFpgaDsp(j))
                    continue;

                if(!((m_params.fpgaMask >> j) & 0x1))
                    continue;

                int res = brd->waitDmaBuffer(j, m_params.dmaChannel, 2000);
                if( res != 0 ) {

                    emit updateInfo("waitDmaBuffer() - 0x" + QString::number(res,16));
                    u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                    info = "BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                    info += " - ERROR TIMEOUT! ADC STATUS = 0x" + QString::number(status_adc, 16);
                    emit updateInfo(info);
                    break;

                } else {

                    brd->writeBuffer(j, m_params.dmaChannel, isviFile.at(fpga_index), 0);
                    string h = hdrs.at(fpga_index);
                    IPC_writeFile(isviFile.at(fpga_index), (void*)h.c_str(), h.size());
                    lockDataFile(flgName.at(fpga_index).c_str(), pass_counter);
                }

                //display 1-st element of each block
                u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                info = QString::number(pass_counter) + " - ";
                info += "BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                info += " : STATUS = 0x" + QString::number((u16)status_adc) + " [";
                for(unsigned k=0; k<Buffers[fpga_index].size(); k++) {
                    u32* value = (u32*)Buffers[fpga_index].at(k);
                    info += (" 0x" + QString::number(value[0], 16) + " ");
                }
                info += " ]";
                emit updateInfo(info);

                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                brd->resetFifo(j, ADC_TRD);
                brd->resetDmaFifo(j, m_params.dmaChannel);

                ++fpga_index;
            }

            ++brd_index;

            emit updateInfo("");
        }

        ++pass_counter;
        IPC_delay(500);
    }

    stopAdcDma();
}

//-----------------------------------------------------------------------------

void adc_test_thread::run()
{
    emit updateInfo("Start ADC test thread");

    try {
        if(!m_boardList.empty()) {

            if(m_params.testMode == 0) {
                dataFromAdc();
            }

            if(m_params.testMode == 1) {
                dataFromMemAsMem();
            }
        }
    } catch(except_info_t err) {
        QString msg = err.info.c_str();
        emit updateInfo(msg);
    } catch(...) {
        emit updateInfo("Error! Exception was generated in program.");
    }

    emit updateInfo("Stop ADC test thread");
}

//-----------------------------------------------------------------------------
