
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "acdsp.h"
#include "acsync.h"
#include "iniparser.h"
#include "fpga_base.h"
#include "exceptinfo.h"
#include "fpga.h"

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
#endif

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);
bool start_pcie_test(std::vector<acdsp*>& boards, struct app_params_t& params);
unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync);
unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber);
void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync);
void delete_fpga_list(std::vector<Fpga*>& fpgaList);

//-----------------------------------------------------------------------------

#define ADC_TRD             4
#define MEM_TRD             5
#define STREAM_BLK_SIZE     0x10000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

#define ADC_SAMPLE_FREQ         (500000000.0)
#define ADC_CHANNEL_MASK        (0xF)
#define ADC_START_MODE          (0x3 << 4)
#define ADC_MAX_CHAN            (0x4)

//-----------------------------------------------------------------------------
#define USE_SIGNAL 0
//-----------------------------------------------------------------------------

#if USE_SIGNAL
acdsp *boardPtr = 0;
static int g_StopFlag = 0;
void stop_exam(int sig)
{
    fprintf(stderr, "\nSIGNAL = %d\n", sig);
    g_StopFlag = 1;
    if(boardPtr) boardPtr->setExitFlag(true);
}
#endif

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct app_params_t params;
    struct sync_params_t sync_params;

    if(!getParams(argc, argv, params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    if(!getSyncParams(argc, argv, sync_params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    showParams(params);
    showSyncParams(sync_params);

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

#if 1
    try {
/*
        acdsp brd;
        brd.setSi57xFreq(params.adcFreq);
        brd.start_local_pcie_test(params);
*/
        std::vector<Fpga*> fpgaList;
        std::vector<acdsp*> boardList;
        acsync *sync = 0;

        create_fpga_list(fpgaList, 10);
        create_board_list(fpgaList, boardList, &sync);

        if(sync) {
          sync->PowerON(true);
          sync->progFD(sync_params.sync_mode, sync_params.sync_selclkout, sync_params.sync_fd, sync_params.sync_fo);
        }

        IPC_delay(1000);

        delete_board_list(boardList,sync);
        delete_fpga_list(fpgaList);
    }
    catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }
#else
    try {

        acdsp brd;

        brd.setSi57xFreq(params.adcFreq);

#if USE_SIGNAL
        boardPtr = &brd;
#endif

        fprintf(stderr, "Start testing DMA: %d\n", params.dmaChannel);
        fprintf(stderr, "DMA information:\n" );
        brd.infoDma(params.fpgaNumber);

        vector<void*> Buffers(params.dmaBlockCount, NULL);

#ifndef ALLOCATION_TYPE_1
        BRDctrl_StreamCBufAlloc sSCA = {BRDstrm_DIR_IN, 1, params.dmaBlockCount, params.dmaBlockSize, Buffers.data(), NULL, };
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(params.fpgaNumber, params.dmaChannel, &sSCA);
#else
        BRDstrm_Stub *pStub = 0;
        fprintf(stderr, "Allocate DMA memory\n");
        brd.allocateDmaMemory(params.fpgaNumber, params.dmaChannel, Buffers.data(), params.dmaBlockSize, params.dmaBlockCount, 1, BRDstrm_DIR_IN, 0x1000, &pStub);
#endif
        IPC_delay(100);

        char fname[64];
        snprintf(fname, sizeof(fname), "data_%d.bin", params.fpgaNumber);
        IPC_handle isviFile = createDataFile(fname);

        char flgname[64];
        snprintf(flgname, sizeof(flgname), "data_%d.flg", params.fpgaNumber);
        createFlagFile(flgname);

        //---------------------------------------------------- DATA FROM STREAM ADC

        if(params.fpgaNumber != 0) {

            if(params.testMode == 0)
                brd.dataFromAdc(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS MEMORY

            if(params.testMode == 1)
                brd.dataFromMemAsMem(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS FIFO

            if(params.testMode == 2)
                brd.dataFromMemAsFifo(params, isviFile, flgname, sSCA);

            //----------------------------------------------------

        } else {

            //---------------------------------------------------- DATA FROM MAIN STREAM

            if(params.testMode == 0)
                brd.dataFromMain(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS MEMORY

            if(params.testMode == 1)
                brd.dataFromMainToMemAsMem(params, isviFile, flgname, sSCA);

            //---------------------------------------------------- DDR3 FPGA AS FIFO

            if(params.testMode == 2)
                brd.dataFromMainToMemAsFifo(params, isviFile, flgname, sSCA);

            //----------------------------------------------------
        }

        fprintf(stderr, "Free DMA memory\n");
        brd.freeDmaMemory(params.fpgaNumber, params.dmaChannel);

        Buffers.clear();

        IPC_closeFile(isviFile);

    }
    catch(...) {

        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
    }
#endif

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    fprintf(stderr, "STOP\n");

    return 0;
}

//-----------------------------------------------------------------------------

unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync)
{
    U16 ID = 0;
    U08 hwAddr = 0;
    U08 fpgaNum = 0;
    boardList.clear();

    for(unsigned i=0; i<fpgaList.size(); i++) {

        Fpga *fpga = fpgaList.at(i);
        bool okid = fpga->FpgaDeviceID(ID);
        bool okhw = fpga->FpgaHwAddress(hwAddr, fpgaNum);
        if(okid && okhw) {
        if(ID == 0x5514) {
            if(sync) {
              *sync = new acsync(fpga);
              break;
            }
        }
        }
    }

    // scan geographical addresses
    for(unsigned j=1; j<6; j++) {

        std::vector<Fpga*> fpgaLocal;
        for(unsigned i=0; i<fpgaList.size(); i++) {

            Fpga *fpga = fpgaList.at(i);
            bool okid =  fpga->FpgaDeviceID(ID);
            bool okhw = fpga->FpgaHwAddress(hwAddr, fpgaNum);

            if(okid && okhw) {

            // skip AC_SYNC
            if(ID == 0x5514) continue;

            // use only fpga with the same hwAddr
            if(hwAddr == j) {
                fpgaLocal.push_back(fpga);
            }
            }
        }

        if(fpgaLocal.size()) {
          boardList.push_back(new acdsp(fpgaLocal));
          fpgaLocal.clear();
        }
    }

    unsigned boards = boardList.size();
    if(sync) {
      if(*sync) boards++;
    }

    fprintf(stderr, "Found %d boards\n", boards);

    return boardList.size();
}

//-----------------------------------------------------------------------------

void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync)
{
    if(sync)
        delete sync;

    for(unsigned i=0; i<boardList.size(); i++) {
        acdsp *brd = boardList.at(i);
        delete brd;
    }
    boardList.clear();
}

//-----------------------------------------------------------------------------

unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber)
{
    fpgaList.clear();

    for(unsigned i=0; i<fpgaNumber; i++) {
        try {
            Fpga *fpga = new Fpga(i);
            fpgaList.push_back(fpga);
        }
        catch(exception_info_t err) {
            fprintf(stderr, "%s", err.info.c_str());
            break;
        }
        catch(...) {
            fprintf(stderr, "Exception in create_fpga_list()\n");
            break;
        }
    }

    fprintf(stderr, "Found %d FPGA\n", fpgaList.size());

    for(unsigned i=0; i<fpgaList.size(); i++) {
        U16 ID = 0;
        U08 hwAddr = 0;
        U08 fpgaNum = 0;
        Fpga *fpga = fpgaList.at(i);
        fpga->FpgaDeviceID(ID);
        fpga->FpgaHwAddress(hwAddr, fpgaNum);
        fprintf(stderr, "HW: 0x%.2X\tID: 0x%.4X\tNUM: 0x%.4X\n", hwAddr, ID, fpgaNum );
    }

    return fpgaList.size();
}

//-----------------------------------------------------------------------------

void delete_fpga_list(std::vector<Fpga*>& fpgaList)
{
    for(unsigned i=0; i<fpgaList.size(); i++) {
        Fpga *fpga = fpgaList.at(i);
        delete fpga;
    }
    fpgaList.clear();
}

//-----------------------------------------------------------------------------

void prepare_adc_fpga(Fpga* fpga, struct app_params_t& params)
{
    fpga->FpgaRegPokeInd(ADC_TRD, 0x10, params.adcMask);
    fpga->FpgaRegPokeInd(ADC_TRD, 0x17, (0x3 << 4));
    fpga->FpgaRegPokeInd(ADC_TRD, 0, 0x2038);
}

//-----------------------------------------------------------------------------

u32 make_sign(u32 chan)
{
    switch(chan) {
    case 0: return 0x11111111;
    case 1: return 0x22222222;
    case 2: return 0x33333333;
    case 3: return 0x44444444;
    case 4: return 0x55555555;
    case 5: return 0x66666666;
    case 6: return 0x77777777;
    case 7: return 0x88888888;
    default: return 0x12345678;
    }
}

//-----------------------------------------------------------------------------

bool start_pcie_test(std::vector<acdsp*>& boards, struct app_params_t& params)
{
    std::vector<AMB_CONFIGURATION> dsp_cfg;
    std::vector<AMB_CONFIGURATION> adc_cfg;

    std::vector<Fpga*> dsp_fpga;
    std::vector<Fpga*> adc_fpga;

    std::vector<trd_check*> check_fpga;
    std::vector<pe_chn_rx*> rx_fpga;
    std::vector<pe_chn_tx*> tx_fpga;

    //-----------------------------------------------------------------------------
    // Prepare all configuration and related objects
    for(unsigned i=0; i<boards.size(); i++) {

        acdsp* brd = boards.at(i);
        if(!brd) {
            continue;
        }

        std::vector<Fpga*> fpgaList = brd->FPGA_LIST();

        for(unsigned j=0; j<fpgaList.size(); j++) {

            AMB_CONFIGURATION cfg;
            Fpga* fpga = fpgaList.at(j);

            fpga->fpgaInfo(cfg);

            if(cfg.PhysAddress[2] && cfg.Size[2]) {

                dsp_cfg.push_back(cfg);
                dsp_fpga.push_back(fpga);
                rx_fpga.push_back(new pe_chn_rx(fpga));
                check_fpga.push_back(new trd_check(fpga));

            } else {

                adc_cfg.push_back(cfg);
                adc_fpga.push_back(fpga);
                tx_fpga.push_back(new pe_chn_tx(fpga));
            }
        }

        brd->setSi57xFreq(params.adcFreq);
    }

    //-----------------------------------------------------------------------------

    fprintf(stderr, "DSP_FPGA: %d\n", dsp_fpga.size());
    fprintf(stderr, "ADC_FPGA: %d\n", adc_fpga.size());

    //-----------------------------------------------------------------------------
    // Configure all objects for transmit and receive data
    for(unsigned i=0; i<dsp_fpga.size(); i++) {

        AMB_CONFIGURATION cfg0 = dsp_cfg.at(i);
        Fpga* dsp = adc_fpga.at(i);
        pe_chn_rx* rx = rx_fpga.at(i);
        trd_check* check = check_fpga.at(i);

        check->start_check(true);
        dsp->FpgaRegPokeInd(4, 0, 0x2038);

        for(unsigned j=0; j<adc_fpga.size(); j++) {

            Fpga* adc = adc_fpga.at(j);
            pe_chn_tx* tx = tx_fpga.at(j);

            prepare_adc_fpga(adc, params);

            u32 rx_fpga_addr = cfg0.PhysAddress[2] + 0x2000*j + 0x1000;
            u32 channel_sign = make_sign(j);


            fprintf(stderr, "RX%d --- TX%d --- SRC_ADDR 0x%X --- SIGN 0x%X\n", i, j, rx_fpga_addr | 0x1, channel_sign);
            rx->set_fpga_addr(j, rx_fpga_addr, channel_sign);

            fprintf(stderr, "RX%d --- TX%d --- DST_ADDR 0x%X --- SIGN 0x%X\n", j, j, rx_fpga_addr | 0x1, channel_sign);
            tx->set_fpga_addr(i, rx_fpga_addr, channel_sign);
        }

        rx->start_rx(true);
    }

    //-----------------------------------------------------------------------------

    for(unsigned j=0; j<adc_fpga.size(); j++) {

        pe_chn_tx* tx = tx_fpga.at(j);
        tx->start_tx(true);
    }

    //-----------------------------------------------------------------------------

    unsigned WIDTH = 25;
    unsigned HEIGHT = 3;
    unsigned ROW = rx_fpga.size() + 1;
    unsigned COL = tx_fpga.size() + 1;

    //-----------------------------------------------------------------------------

    table *t = new table(COL,WIDTH,HEIGHT);
    if(!t) {
        exit(-1);
    }

    t->create_table(ROW, COL);

    if(t->create_header())
        t->set_header_text("%s", "TEST PCIE EXCHANGE");

    if(t->create_status())
        t->set_status_text("%s", "Format [A / B / C / D]  A - Ok block, B - sign err, D - block err, C - Err block");

    t->set_cell_text(0, 0, "%s", "CHANNEL");

    //---------------------------------------

    for(unsigned i=1; i<ROW; i++) {
        t->set_cell_text(i, 0, "RX_CHN%d", i-1);
    }

    for(unsigned i=1; i<COL; i++) {
        t->set_cell_text(0, i, "TX_CHN%d", i-1);
    }

    //---------------------------------------

    while(1) {

        if(IPC_kbhit()) {
            break;
        }

        IPC_delay(500);

        for(unsigned i=0; i<rx_fpga.size(); i++) {

            pe_chn_rx* rx = rx_fpga.at(i);
            trd_check* check = check_fpga.at(i);

            for(unsigned j=0; j<tx_fpga.size(); j++) {

                u32 ok_block_number = check->ok_block_number(j);
                u32 sign_err_number = rx->sign_err_number(j);
                u32 block_err_number = rx->block_err_number(j);
                u32 check_err_number = check->err_block_number(j);

                t->set_cell_text(i+1, j+1, "%d / %d / %d / %d", ok_block_number, sign_err_number, block_err_number, check_err_number);
            }
        }
    }

    delete t;

    //-----------------------------------------------------------------------------
    // Deconfigure and delete all objects
    for(unsigned i=0; i<dsp_fpga.size(); i++) {

        Fpga* dsp = adc_fpga.at(i);
        pe_chn_rx* rx = rx_fpga.at(i);
        trd_check* check = check_fpga.at(i);

        check->start_check(false);
        dsp->FpgaRegPokeInd(4, 0, 0);
        rx->start_rx(false);

        for(unsigned j=0; j<adc_fpga.size(); j++) {

            Fpga* adc = adc_fpga.at(j);
            pe_chn_tx* tx = tx_fpga.at(j);

            adc->FpgaRegPokeInd(ADC_TRD, 0x17, 0x0);
            adc->FpgaRegPokeInd(ADC_TRD, 0x0, 0x0);

            tx->start_tx(false);

            delete tx;
        }

        delete rx;
        delete check;
    }

    rx_fpga.clear();
    tx_fpga.clear();
    check_fpga.clear();
    dsp_cfg.clear();
    adc_cfg.clear();
    dsp_fpga.clear();
    adc_fpga.clear();

    return true;
}

//-----------------------------------------------------------------------------
