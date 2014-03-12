#include "pcie_test.h"
#include "iniparser.h"
#include "isvi.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------

unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber, unsigned from)
{
    fprintf(stderr, "%s()\n", __FUNCTION__);

    fpgaList.clear();

    for(unsigned i=from; i<from+fpgaNumber; i++) {
        try {
            Fpga *fpga = new Fpga(i);
            fpgaList.push_back(fpga);
        }
        catch(except_info_t err) {
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
    fprintf(stderr, "%s()\n", __FUNCTION__);

    for(unsigned i=0; i<fpgaList.size(); i++) {
        Fpga *fpga = fpgaList.at(i);
        delete fpga;
    }
    fpgaList.clear();
}

//-----------------------------------------------------------------------------

unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync_board)
{
    fprintf(stderr, "%s()\n", __FUNCTION__);

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
                if(sync_board) {
                    *sync_board = new acsync(fpga);
                    break;
                }
            }
        }
    }

    // scan geographical addresses
    std::vector<Fpga*> fpgaLocal;
    for(unsigned slotIndex=1; slotIndex<=6; slotIndex++) {

        for(unsigned i=0; i<fpgaList.size(); i++) {

            Fpga *fpga = fpgaList.at(i);
            bool okid =  fpga->FpgaDeviceID(ID);
            bool okhw = fpga->FpgaHwAddress(hwAddr, fpgaNum);

            if(okid && okhw) {

                // skip AC_SYNC
                if(ID == 0x5514) {
                    continue;
                }

                // use only fpga with the same hwAddr
                if(hwAddr == slotIndex) {
                    fpgaLocal.push_back(fpga);
                }
            }
        }

        if(fpgaLocal.size() == 3) {
            acdsp* brd = new acdsp(fpgaLocal);
            boardList.push_back(brd);
            fpgaLocal.clear();
        }
    }

    unsigned boards = boardList.size();
    if(sync_board) {
        if(*sync_board) boards++;
    }

    fprintf(stderr, "Found %d boards\n", boards);

    return boards;
}

//-----------------------------------------------------------------------------

void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync_board)
{
    fprintf(stderr, "%s()\n", __FUNCTION__);

    if(sync_board) {
        delete sync_board;
    }

    for(unsigned i=0; i<boardList.size(); i++) {
        acdsp *brd = boardList.at(i);
        delete brd;
        fprintf(stderr, "%s(): delete ACDSP: %p\n", __FUNCTION__, brd);
    }

    boardList.clear();

    fprintf(stderr, "%s(): clear brd list\n", __FUNCTION__);
}

//-----------------------------------------------------------------------------

u32 make_sign(u32 chan, u8 hwAddr, u8 fpgaNum)
{
    switch(chan) {
    case 0: return (0x11110000 | (hwAddr << 8 | fpgaNum));
    case 1: return (0x22220000 | (hwAddr << 8 | fpgaNum));
    case 2: return (0x33330000 | (hwAddr << 8 | fpgaNum));
    case 3: return (0x44440000 | (hwAddr << 8 | fpgaNum));
    case 4: return (0x55550000 | (hwAddr << 8 | fpgaNum));
    case 5: return (0x66660000 | (hwAddr << 8 | fpgaNum));
    case 6: return (0x77770000 | (hwAddr << 8 | fpgaNum));
    case 7: return (0x88880000 | (hwAddr << 8 | fpgaNum));
    default: return 0x12345678;
    }
}

//-----------------------------------------------------------------------------
#ifdef __linux__
table* create_display_table(std::vector<pe_chn_rx*>& rx_fpga, std::vector<pe_chn_tx*>& tx_fpga)
{
    unsigned WIDTH = 25;
    unsigned HEIGHT = 3;
    unsigned ROW = 2*rx_fpga.size() + 2;
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
        //t->set_status_text("%s", "Format [A / B / C / D]  A - trd check ok, B - rx sign err, C - rx block err, D - trd check err");
        t->set_status_text("%s", "");

    //---------------------------------------

    t->set_cell_text(0, 0, "%s", "CHANNEL");

    for(unsigned i=1; i<ROW/2; i++) {
        t->set_cell_text(i, 0, "RX_CHN%d", i-1);
    }

    for(unsigned i=1; i<COL; i++) {
        t->set_cell_text(0, i, "TX_CHN%d", i-1);
    }

    //---------------------------------------

    t->set_cell_text(rx_fpga.size() + 1, 0, "%s", "CHANNEL");

    for(unsigned i=1; i<COL; i++) {
        t->set_cell_text(rx_fpga.size() + 1, i, "TX_CHN%d MB/s", i-1);
    }

    for(unsigned i=1; i<ROW/2; i++) {
        t->set_cell_text(rx_fpga.size() + 1 + i, 0, "RX_CHN%d", i-1);
    }

    //---------------------------------------

    return t;
}
#endif
//-----------------------------------------------------------------------------

typedef struct counter_t {
    std::vector<u32>    dataVector;
} counter_t;

//-----------------------------------------------------------------------------

typedef struct pcie_speed_t {
    std::vector<float>  dataVector;
} pcie_speed_t;

//-----------------------------------------------------------------------------

template <class T>
void clearCounters(std::vector<T>& param)
{
    for(unsigned i=0; i<param.size(); i++) {
        T& entry = param.at(i);
        entry.dataVector.clear();
    }
    param.clear();
}

//-----------------------------------------------------------------------------

void getCounters(std::vector<trd_check*>& check_fpga, std::vector<counter_t>& counters, unsigned tx_number)
{
    clearCounters(counters);

    for(unsigned i=0; i<check_fpga.size(); i++) {

        trd_check* check = check_fpga.at(i);
        counter_t rd_cnt;

        for(unsigned j=0; j<tx_number; j++) {

            rd_cnt.dataVector.push_back(check->rd_block_number(j));
        }

        counters.push_back(rd_cnt);
    }
}

//-----------------------------------------------------------------------------

void showCounters(std::vector<counter_t>& counters, table* t)
{
    for(unsigned i=0; i<counters.size(); i++) {
        counter_t& rd_cnt = counters.at(i);
        if(!t) {
            fprintf(stderr, "RX%.2u", i);
        }
        for(unsigned j=0; j<rd_cnt.dataVector.size(); j++) {
            if(!t) {
                fprintf(stderr, "\tTX[%.2u]:\t%d ", j, rd_cnt.dataVector.at(j));
            } else {
                t->set_cell_text(i+1, j+1, "%d", rd_cnt.dataVector.at(j));
            }
        }
        if(!t) {
            fprintf(stderr, "\n");
        }
    }
}

//-----------------------------------------------------------------------------

void calculateSpeed(std::vector<pcie_speed_t>& dataRate, std::vector<counter_t>& counters0, std::vector<counter_t>& counters1, float dt)
{
    clearCounters(dataRate);

    for(unsigned i=0; i<counters0.size(); i++) {

        counter_t& rd_cnt0 = counters0.at(i);
        counter_t& rd_cnt1 = counters1.at(i);

        pcie_speed_t rateVector;

        for(unsigned j=0; j<rd_cnt0.dataVector.size(); j++) {

            u32 c0 = rd_cnt0.dataVector.at(j);
            u32 c1 = rd_cnt1.dataVector.at(j);

            float rxRate = 1000.0*(((c1-c0)*16.0*32768.0)/(1024.*1024.)/dt);

            rateVector.dataVector.push_back(rxRate);
        }

        dataRate.push_back(rateVector);
    }
}

//-----------------------------------------------------------------------------

void showSpeed(std::vector<pcie_speed_t>& dataRate, table *t)
{
    for(unsigned i=0; i<dataRate.size(); i++) {

        pcie_speed_t& rateVector = dataRate.at(i);

        if(!t) {
            fprintf(stderr, "RX%.2d", i);
        }

        for(unsigned j=0; j<rateVector.dataVector.size(); j++) {

            float rxRate = rateVector.dataVector.at(j);
            if(!t) {
                fprintf(stderr, "\tTX[%.2d]:\t%.2f ", j, rxRate);
            } else {
                t->set_cell_text(i+dataRate.size()+2, j+1, "%.2f ", rxRate);
            }
        }
        if(!t) {
            fprintf(stderr, "\n");
        }
    }
}

//-----------------------------------------------------------------------------

void show_test_result(std::vector<trd_check*>& check_fpga, std::vector<pe_chn_rx*>& rx_fpga, std::vector<pe_chn_tx*>& tx_fpga)
{
    std::vector<counter_t>      counters0;
    std::vector<counter_t>      counters1;
    std::vector<pcie_speed_t>   dataRate;

    bool stop_flag = false;
/*
    while(!stop_flag) {

        struct timeval start0;
        time_start(&start0);

        getCounters(check_fpga, counters0, tx_fpga.size());

        fprintf(stderr, "working time:");
        for(unsigned tt=0; tt<10; ++tt) {
            IPC_delay(1000);
            fprintf(stderr, " %.2f", time_stop(start0)/1000.);
            if(IPC_kbhit()) {
                stop_flag = true;
            }
            if(stop_flag)
                break;
        }
        fprintf(stderr, "\n");

        getCounters(check_fpga, counters1, tx_fpga.size());
        calculateSpeed(dataRate, counters0, counters1, time_stop(start0));
        showSpeed(dataRate);
    }
*/
#ifdef __linux__
    table *t = create_display_table(rx_fpga, tx_fpga);
    if(!t) {
        return;
    }
    //table *t = 0;
    struct timeval start0;
    time_start(&start0);

    while(!stop_flag) {

        struct timeval start1;
        time_start(&start1);

        getCounters(check_fpga, counters0, tx_fpga.size());

        IPC_delay(2000);

        if(!t) {
            fprintf(stderr, "Working time: %.2f\n", time_stop(start0)/1000.);
        } else {
            t->set_status_text("Working time: %.2f", time_stop(start0)/1000.);
        }

        getCounters(check_fpga, counters1, tx_fpga.size());
        calculateSpeed(dataRate, counters0, counters1, time_stop(start1));
        showCounters(counters1, t);
        showSpeed(dataRate, t);

        if(IPC_kbhit()) {
            stop_flag = true;
        }
/*
        for(unsigned i=0; i<rx_fpga.size(); i++) {

            pe_chn_rx* rx = rx_fpga.at(i);
            trd_check* check = check_fpga.at(i);

            for(unsigned j=0; j<tx_fpga.size(); j++) {

                t->set_cell_text(i+1, j+1, "%d / %d / %d / %d", read_block_number, sign_err_number, block_err_number, check_err_number);
                t->set_cell_text(rx_fpga.size()+i+2, j+1, "%.2f / %.2f", average_speed, instant_speed);
            }
        }
*/
    }
    if(t) delete t;
#endif
}

//-----------------------------------------------------------------------------

bool start_pcie_test(std::vector<acdsp*>& boardList, struct app_params_t& params)
{
    std::vector<AMB_CONFIGURATION> dsp_cfg;
    std::vector<AMB_CONFIGURATION> adc_cfg;
    std::vector<Fpga*> dsp_fpga;
    std::vector<Fpga*> adc_fpga;
    std::vector<trd_check*> check_fpga;
    std::vector<pe_chn_rx*> rx_fpga;
    std::vector<pe_chn_tx*> tx_fpga;
    std::vector<u32> sign;

    //-----------------------------------------------------------------------------
    // Prepare all configuration and related objects
    AMB_CONFIGURATION cfg;
    for(unsigned brdIndex=0; brdIndex<boardList.size(); brdIndex++) {

        acdsp* brd = boardList.at(brdIndex);

        brd->infoFpga(2, cfg);
        dsp_cfg.push_back(cfg);
        dsp_fpga.push_back(brd->FPGA(2));
        rx_fpga.push_back(brd->get_chan_rx());
        check_fpga.push_back(brd->get_trd_check());
        brd->infoFpga(0, cfg);
        adc_cfg.push_back(cfg);
        brd->infoFpga(1, cfg);
        adc_cfg.push_back(cfg);
        adc_fpga.push_back(brd->FPGA(0));
        adc_fpga.push_back(brd->FPGA(1));
        tx_fpga.push_back(brd->get_chan_tx(0));
        tx_fpga.push_back(brd->get_chan_tx(1));
    }

    fprintf(stderr, "DSP_FPGA: %d\n", dsp_fpga.size());
    fprintf(stderr, "ADC_FPGA: %d\n", adc_fpga.size());

    //-----------------------------------------------------------------------------
    // form SIGN for all TX channels
    for(unsigned txIndex=0; txIndex<tx_fpga.size(); txIndex++) {
        u8 hwAddr = 0xff;
        u8 fpgaNum = 0xff;
        adc_fpga.at(txIndex)->FpgaHwAddress(hwAddr, fpgaNum);
        sign.push_back(make_sign(txIndex, hwAddr, fpgaNum));
    }

    //-----------------------------------------------------------------------------
    // program RX channels
    for(unsigned rxIndex=0; rxIndex<dsp_fpga.size(); rxIndex++) {

        AMB_CONFIGURATION cfg0 = dsp_cfg.at(rxIndex);

        trd_check* check = check_fpga.at(rxIndex);
        pe_chn_rx* rx = rx_fpga.at(rxIndex);

        fprintf(stderr, "===== RX %d =====\n", rxIndex);

        for(unsigned txIndex=0; txIndex<adc_fpga.size(); txIndex++) {

            u32 rx_fpga_addr = cfg0.PhysAddress[2] + 0x1000*txIndex + 0x1000;
            fprintf(stderr, "TX%d\t\t SRC_ADDR 0x%X\t\t SIGN 0x%X\n", txIndex, rx_fpga_addr|0x1, sign.at(txIndex));
            rx->set_fpga_addr(txIndex, rx_fpga_addr, sign.at(txIndex));
        }

        check->start_check(true);
        rx->start_rx(true);
    }

    //-----------------------------------------------------------------------------
    // program TX channels (first approach)
    for(unsigned txIndex=0, rx_idx = 0; txIndex < adc_fpga.size(); txIndex++) {

        fprintf(stderr, "===== TX %d =====\n", txIndex);

        pe_chn_tx* tx = tx_fpga.at(txIndex);

        for(unsigned rxIndex = 0; rxIndex < dsp_fpga.size(); rxIndex++) {

            AMB_CONFIGURATION cfg0 = dsp_cfg.at((rxIndex + rx_idx) % dsp_fpga.size());
            u32 rx_fpga_addr = cfg0.PhysAddress[2] + 0x1000*txIndex + 0x1000;
            fprintf(stderr, "RX%d\t\t DST_ADDR 0x%X\t\t SIGN 0x%X\n", rxIndex, rx_fpga_addr|0x1, sign.at(txIndex));
            tx->set_fpga_addr(rxIndex, rx_fpga_addr, sign.at(txIndex));
        }

        ++rx_idx;

        tx->start_tx(true);
    }
/*
    //-----------------------------------------------------------------------------
    // program TX channels (second approach)
    for(unsigned txIndex=0, rx_idx = 0; txIndex < adc_fpga.size(); txIndex++) {

        fprintf(stderr, "===== TX %d =====\n", txIndex);

        pe_chn_tx* tx = tx_fpga.at(txIndex);

        if((txIndex%2) == 0) {
            if(txIndex) {
                ++rx_idx;
            }
        }

        for(unsigned rxIndex = 0; rxIndex < dsp_fpga.size(); rxIndex++) {

            AMB_CONFIGURATION cfg0 = dsp_cfg.at((rxIndex + rx_idx) % dsp_fpga.size());
            u32 rx_fpga_addr = cfg0.PhysAddress[2] + 0x1000*txIndex + 0x1000;
            fprintf(stderr, "RX%d\t\t DST_ADDR 0x%X\t\t SIGN 0x%X\n", rxIndex, rx_fpga_addr|0x1, sign.at(txIndex));
            tx->set_fpga_addr(rxIndex, rx_fpga_addr, sign.at(txIndex));
        }

        tx->start_tx(true);
    }
*/
    //-----------------------------------------------------------------------------

    fprintf(stderr, "Press any key to see result...\n");
    IPC_getch();
    show_test_result(check_fpga, rx_fpga, tx_fpga);

    //-----------------------------------------------------------------------------
    // Deconfigure TX and RX
    fprintf(stderr, "STOP TX...\n");
    for(unsigned txIndex=0; txIndex<tx_fpga.size(); txIndex++) {

        pe_chn_tx* tx = tx_fpga.at(txIndex);
        tx->start_tx(false);
    }

    IPC_delay(500);
    fprintf(stderr, "STOP RX and CHECK...\n");

    for(unsigned rxIndex=0; rxIndex<dsp_fpga.size(); rxIndex++) {

        pe_chn_rx* rx = rx_fpga.at(rxIndex);
        trd_check* check = check_fpga.at(rxIndex);

        check->start_check(false);
        rx->start_rx(false);
    }

    IPC_delay(500);
    fprintf(stderr, "Clear resources...\n");

    rx_fpga.clear();
    tx_fpga.clear();
    check_fpga.clear();
    dsp_cfg.clear();
    adc_cfg.clear();
    dsp_fpga.clear();
    adc_fpga.clear();
    sign.clear();

    fprintf(stderr, "Stop PCIE test\n");

    return true;
}

//-----------------------------------------------------------------------------
