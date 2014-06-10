#include "adc_test_thread.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

adc_test_thread::adc_test_thread(std::vector<acdsp*>& boardList) : m_boardList(boardList)
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
    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        datafiles_t isviFile;
        flgnames_t flgName;
        hdr_t hdr;

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

                char fname[64];
                snprintf(fname, sizeof(fname), "data_%d%d.bin", brd->slotNumber(), j);
                isviFile.push_back(createDataFile(fname));

                char tmpflg[64];
                snprintf(tmpflg, sizeof(tmpflg), "data_%d%d.flg", brd->slotNumber(), j);
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
    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        // allocate DMA buffer for 2 FPGA ADC
        vector<dmabuf_t> Buffers;
        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if((m_params.fpgaMask & (0x1 << j))) {

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

void adc_test_thread::startAdcDma()
{
    unsigned N = m_boardList.size();

    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

                brd->setDmaSource(j, m_params.dmaChannel, ADC_TRD);
                brd->setDmaDirection(j, m_params.dmaChannel, BRDstrm_DIR_IN);
                brd->RegPokeInd(j, ADC_TRD, 0x10, m_params.adcMask);
                brd->RegPokeInd(j, ADC_TRD, 0x17, (m_params.adcStart << 4));
                brd->startDma(j, m_params.dmaChannel, 0);
                brd->RegPokeInd(j, ADC_TRD, 0, 0x2038);
            }
        }
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::stopAdcDma()
{
    unsigned N = m_boardList.size();

    // Stop ADC and DMA channels for non masked FPGA
    // free DMA buffers, close ISVI data file
    for(unsigned i=0; i<N; ++i) {

        acdsp* brd = m_boardList.at(i);
        //datafiles_t isviFile = isviFiles.at(i);

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

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

    for(unsigned i=0; i<N; ++i) {

        acdsp* brd = m_boardList.at(i);

        // prepare and start ADC and MEM for non masked FPGA
        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(!(m_params.fpgaMask & (0x1 << j)))
                continue;

            brd->DDR3(j)->setMemory(1, 0, PostTrigSize, MemBufSize);
            brd->DDR3(j)->Enable(true);

            brd->setDmaSource(j, m_params.dmaChannel, MEM_TRD);
            brd->setDmaRequestFlag(j, m_params.dmaChannel, BRDstrm_DRQ_HALF);
            brd->resetFifo(j, ADC_TRD);
            brd->resetFifo(j, MEM_TRD);
            brd->resetDmaFifo(j, m_params.dmaChannel);
            brd->RegPokeInd(j, ADC_TRD, 0x10, m_params.adcMask);
            brd->RegPokeInd(j, ADC_TRD, 0x17, (m_params.adcStart << 4));
            brd->RegPokeInd(j, MEM_TRD, 0x0, 0x2038);
            brd->RegPokeInd(j, ADC_TRD, 0x0, 0x2038);
        }
    }
}

//-----------------------------------------------------------------------------

void adc_test_thread::stopAdcDmaMem()
{
    unsigned N = m_boardList.size();

    // Stop ADC, MEM and DMA channels for non masked FPGA
    // free DMA buffers, close ISVI data file
    for(unsigned i=0; i<N; ++i) {

        acdsp* brd = m_boardList.at(i);
        //datafiles_t isviFile = isviFiles.at(i);

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

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

        for(unsigned i=0; i<N; ++i) {

            acdsp* brd = m_boardList.at(i);
            datafiles_t isviFile = isviFiles.at(i);
            flgnames_t flgName = flgNames.at(i);
            hdr_t hdrs = isviHdrs.at(i);
            QString info;

            if(!m_start)
                break;

            // Save MEM data in ISVI file for non masked FPGA
            for(unsigned j=0; j<brd->FPGA_LIST().size(); ++j) {

                if(!(m_params.fpgaMask & (0x1 << j)))
                    continue;

                if(brd->isFpgaDsp(j))
                    continue;

                if(!m_start)
                    break;

                for(unsigned counter = 0; counter < m_params.dmaBuffersCount; counter++) {

                    brd->startDma(j, m_params.dmaChannel, 0x0);

                    if( brd->waitDmaBuffer(j, m_params.dmaChannel, 100) < 0 ) {

                        u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                        u32 status_mem = brd->RegPeekDir(j, MEM_TRD, 0x0);
                        info += " BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                        info += " - ERROR TIMEOUT! ADC STATUS = 0x" + QString::number(status_adc) + "MEM_STATUS = 0x" + QString::number(status_mem);
                        emit updateInfo(info);
                        break;

                    } else {

                        brd->writeBuffer(j, m_params.dmaChannel, isviFile[j], counter * m_params.dmaBlockSize * m_params.dmaBlockCount);
                        info = QString::number(pass_counter) + " - ";
                        info += "BRD: " + QString::number(i) + " FPGA: " + QString::number(j);
                        info += " - Write DMA buffer: " + QString::number(counter);
                        emit updateInfo(info);
                    }

                    brd->stopDma(j, m_params.dmaChannel);
                }

                string h = hdrs.at(j);
                IPC_writeFile(isviFile.at(j), (void*)h.c_str(), h.size());
                lockDataFile(flgName[j].c_str(), pass_counter);

                brd->RegPokeInd(j, MEM_TRD, 0x0, 0x0);
                brd->RegPokeInd(j, ADC_TRD, 0x0, 0x0);
                brd->resetFifo(j, ADC_TRD);
                brd->resetFifo(j, MEM_TRD);
                brd->resetDmaFifo(j, m_params.dmaChannel);
            }
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
    // prepare and start ADC and DMA channels for non masked FPGA
    startAdcDma();

    //---------------------------------------------------------------

    unsigned pass_counter = 0;

    while(m_start) {

        for(unsigned i=0; i<N; ++i) {

            acdsp* brd = m_boardList.at(i);
            vector<dmabuf_t> Buffers = dmaBuffers.at(i);
            datafiles_t isviFile = isviFiles.at(i);
            flgnames_t flgName = flgNames.at(i);
            hdr_t hdrs = isviHdrs.at(i);
            QString info;

            // save ADC data into ISVI files for non masked FPGA
            for(unsigned j=0; j<brd->FPGA_LIST().size(); ++j) {

                if(!(m_params.fpgaMask & (0x1 << j)))
                    continue;

                if(brd->isFpgaDsp(j))
                    continue;

                if( brd->waitDmaBuffer(j, m_params.dmaChannel, 2000) < 0 ) {

                    u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                    info = "BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                    info += " - ERROR TIMEOUT! ADC STATUS = 0x" + QString::number(status_adc, 16);
                    emit updateInfo(info);
                    break;

                } else {

                    brd->writeBuffer(j, m_params.dmaChannel, isviFile.at(j), 0);
                    string h = hdrs.at(j);
                    IPC_writeFile(isviFile.at(j), (void*)h.c_str(), h.size());
                    lockDataFile(flgName.at(j).c_str(), pass_counter);
                }

                //display 1-st element of each block
                u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                info = QString::number(pass_counter) + " - ";
                info += "BRD: " + QString::number(brd->slotNumber()) + " FPGA: " + QString::number(j);
                info += " : STATUS = 0x" + QString::number((u16)status_adc) + " [";
                for(unsigned k=0; k<Buffers[j].size(); k++) {
                    u32* value = (u32*)Buffers[j].at(k);
                    info += (" 0x" + QString::number(value[0], 16) + " ");
                }
                info += " ]";
                emit updateInfo(info);

                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                brd->resetFifo(j, ADC_TRD);
                brd->resetDmaFifo(j, m_params.dmaChannel);
                brd->startDma(j,m_params.dmaChannel,0);
                IPC_delay(10);
                brd->RegPokeInd(j,ADC_TRD,0,0x2038);
            }
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
