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

typedef vector<void*> dmabuf_t;
typedef vector<dmabuf_t> brdbuf_t;

typedef vector<IPC_handle> datafiles_t;
typedef vector<datafiles_t> isvidata_t;

typedef vector<string> flgnames_t;
typedef vector<flgnames_t> isviflg_t;
typedef vector<string> hdr_t;
typedef vector<hdr_t> isvihdr_t;

typedef vector<unsigned> cnt_t;
typedef vector<cnt_t> count_t;

//-----------------------------------------------------------------------------

void adc_test_thread::run()
{
    if(m_boardList.empty()) {
        return;
    }

    unsigned N = m_boardList.size();

    //-------------------------------------------------------------
    // Allocate DMA buffers for all board and all onboard FPGA ADC
    vector<brdbuf_t> dmaBuffers;
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

    //--------------------------------------------------------------
    //Prepare data and flag files for ISVI and counters
    count_t counters;
    isvidata_t isviFiles;
    isviflg_t  flgNames;
    isvihdr_t isviHdrs;
    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        datafiles_t isviFile;
        flgnames_t flgName;
        cnt_t count;
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

                count.push_back(0);

                string s;
                createIsviHeader(s, brd->slotNumber(), j, m_params);
                hdr.push_back(s);
            }
        }

        isviFiles.push_back(isviFile);
        flgNames.push_back(flgName);
        counters.push_back(count);
        isviHdrs.push_back(hdr);
    }

    //---------------------------------------------------------------
    // prepare and start ADC and DMA channels for non masked FPGA
    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

                fprintf(stderr, "Set DMA source\n");
                brd->setDmaSource(j, m_params.dmaChannel, ADC_TRD);

                fprintf(stderr, "Set DMA direction\n");
                brd->setDmaDirection(j, m_params.dmaChannel, BRDstrm_DIR_IN);

                fprintf(stderr, "Set ADC channels mask\n");
                brd->RegPokeInd(j, ADC_TRD, 0x10, m_params.adcMask);

                fprintf(stderr, "Set ADC start mode\n");
                brd->RegPokeInd(j, ADC_TRD, 0x17, (0x3 << 4));

                fprintf(stderr, "Start DMA channel\n");
                brd->startDma(j, m_params.dmaChannel, 0);

                fprintf(stderr, "Start ADC\n");
                brd->RegPokeInd(j, ADC_TRD, 0, 0x2038);
            }
        }
    }

    //---------------------------------------------------------------

    while(m_start) {

        for(unsigned i=0; i<N; ++i) {

            acdsp* brd = m_boardList.at(i);
            vector<dmabuf_t> Buffers = dmaBuffers.at(i);
            datafiles_t isviFile = isviFiles.at(i);
            flgnames_t flgName = flgNames.at(i);
            cnt_t count = counters.at(i);
            hdr_t hdrs = isviHdrs.at(i);

            // save ADC data into ISVI files for non masked FPGA
            for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

                if(!(m_params.fpgaMask & (0x1 << j)))
                    continue;

                if( brd->waitDmaBuffer(j, m_params.dmaChannel, 2000) < 0 ) {

                    u32 status_adc = brd->RegPeekDir(j, ADC_TRD, 0x0);
                    fprintf( stderr, "ERROR TIMEOUT! ADC STATUS = 0x%.4X\n", status_adc);
                    break;

                } else {

                    brd->writeBuffer(j, m_params.dmaChannel, isviFile.at(j), 0);

                    string h = hdrs.at(j);
                    IPC_writeFile(isviFile.at(j), (void*)h.c_str(), h.size());

                    lockDataFile(flgName.at(j).c_str(), count.at(j));
                }

                //display 1-st element of each block
                fprintf(stderr, "%d: STATUS = 0x%.4X [", ++count.at(j), (u16)brd->RegPeekDir(j,ADC_TRD,0x0));
                for(unsigned k=0; k<Buffers[j].size(); k++) {
                    u32* value = (u32*)Buffers[j].at(k);
                    fprintf(stderr, " 0x%.8x ", value[0]);
                }
                fprintf(stderr, " ]\r");

                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                brd->resetFifo(j, ADC_TRD);
                brd->resetDmaFifo(j, m_params.dmaChannel);
                brd->startDma(j,m_params.dmaChannel,0);
                IPC_delay(10);
                brd->RegPokeInd(j,ADC_TRD,0,0x2038);
            }
        }
    }

    // Stop ADC and DMA channels for non masked FPGA
    // free DMA buffers, close ISVI data file
    for(unsigned i=0; i<N; ++i) {

        // Take one board from list
        acdsp* brd = m_boardList.at(i);
        datafiles_t isviFile = isviFiles.at(i);

        for(unsigned j=0; j<ADC_FPGA_COUNT; ++j) {

            if(m_params.fpgaMask & (0x1 << j)) {

                brd->RegPokeInd(j,ADC_TRD,0,0x0);
                brd->stopDma(j, m_params.dmaChannel);
                IPC_delay(10);
                brd->freeDmaMemory(j, m_params.dmaChannel);
                IPC_closeFile(isviFile.at(j));
            }
        }
    }
}
