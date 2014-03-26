#include "pcie_test.h"
#include "iniparser.h"
#include "isvi.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber, unsigned from)
{
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
    for(unsigned i=0; i<fpgaList.size(); i++) {
        Fpga *fpga = fpgaList.at(i);
        delete fpga;
    }
    fpgaList.clear();
}

//-----------------------------------------------------------------------------

unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync_board, u32 boradMask)
{
    U16 ID = 0;
    U08 hwAddr = 0;
    U08 fpgaNum = 0;
    boardList.clear();

    if(fpgaList.empty())
        return 0;

    // AC_SYNC
    if(boradMask & 0x1) {

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
    }

    // scan geographical addresses for non masked AC_DSP boards
    std::vector<Fpga*> fpgaLocal;
    for(unsigned slotIndex=0; slotIndex<6; slotIndex++) {

        if(!(boradMask & (0x1 << slotIndex)))
            continue;

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
                if(hwAddr == (slotIndex+1)) {
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
        if(*sync_board) ++boards;
    }

    fprintf(stderr, "Found %d boards\n", boards);

    return boards;
}

//-----------------------------------------------------------------------------

void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync_board)
{
    if(sync_board) {
        delete sync_board;
    }

    for(unsigned i=0; i<boardList.size(); i++) {
        acdsp *brd = boardList.at(i);
        delete brd;
    }

    boardList.clear();
}

//-----------------------------------------------------------------------------

#ifdef __linux__
table* create_display_table(std::vector<acdsp*>& boardList)
{
    unsigned N = boardList.size();
    unsigned WIDTH = 17;
    unsigned HEIGHT = 3;
    unsigned ROW = 2*N + 2;
    unsigned COL = 2*N + 1;

    //-----------------------------------------------------------------------------

    table *t = new table(COL,WIDTH,HEIGHT);
    if(!t) {
        exit(-1);
    }
    t->create_table(ROW, COL);

    if(t->create_header())
        t->set_header_text("%s", "TEST PCIE EXCHANGE");

    if(t->create_status())
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

    t->set_cell_text(N + 1, 0, "%s", "CHANNEL");

    for(unsigned i=1; i<COL; i++) {
        t->set_cell_text(N + 1, i, "TX_CHN%d MB/s", i-1);
    }

    for(unsigned i=1; i<ROW/2; i++) {
        t->set_cell_text(N + 1 + i, 0, "RX_CHN%d", i-1);
    }

    //---------------------------------------

    return t;
}
#endif
//-----------------------------------------------------------------------------

typedef struct counter_t {
    std::vector<u32>    dataVector0;
    std::vector<u32>    dataVector1;
} counter_t;

//-----------------------------------------------------------------------------

typedef struct pcie_speed_t {
    std::vector<float>  dataVector0;
    std::vector<u32>    dataVector1;
} pcie_speed_t;

//-----------------------------------------------------------------------------

template <class T>
void clearCounters(std::vector<T>& param)
{
    for(unsigned i=0; i<param.size(); i++) {
        T& entry = param.at(i);
        entry.dataVector0.clear();
        entry.dataVector1.clear();
    }
    param.clear();
}

//-----------------------------------------------------------------------------

void getCounters(vector<acdsp*>& boardList, std::vector<counter_t>& counters, unsigned tx_number)
{
    clearCounters(counters);

    for(unsigned i=0; i<boardList.size(); i++) {

        acdsp* brd = boardList.at(i);
        trd_check* check = brd->get_trd_check();
        counter_t rd_cnt;

        for(unsigned j=0; j<tx_number; j++) {

            rd_cnt.dataVector0.push_back(check->rd_block_number(j));
            rd_cnt.dataVector1.push_back(check->err_block_number(j));
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
        for(unsigned j=0; j<rd_cnt.dataVector0.size(); j++) {
            if(!t) {
                fprintf(stderr, "\tTX[%.2u]:\t%d\t%d", j, rd_cnt.dataVector0.at(j), rd_cnt.dataVector1.at(j) );
            } else {
                t->set_cell_text(i+1, j+1, "%d / %d", rd_cnt.dataVector0.at(j), rd_cnt.dataVector1.at(j));
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

        for(unsigned j=0; j<rd_cnt0.dataVector0.size(); j++) {

            u32 c0 = rd_cnt0.dataVector0.at(j);
            u32 c1 = rd_cnt1.dataVector0.at(j);

            float rxRate = 1000.0*(((c1-c0)*16.0*32768.0)/(1024.*1024.)/dt);

            rateVector.dataVector0.push_back(rxRate);
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

        for(unsigned j=0; j<rateVector.dataVector0.size(); j++) {

            float rxRate = rateVector.dataVector0.at(j);
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

void show_test_result(vector<acdsp*>& boardList)
{
    std::vector<counter_t>      counters0;
    std::vector<counter_t>      counters1;
    std::vector<pcie_speed_t>   dataRate;

    bool stop_flag = false;
    unsigned N = boardList.size();

#ifdef __linux__
    table *t = create_display_table(boardList);
    if(!t) {
        return;
    }
    //table *t = 0;
    struct timeval start0;
    time_start(&start0);

    while(!stop_flag) {

        struct timeval start1;
        time_start(&start1);

        getCounters(boardList, counters0, 2*N);

        IPC_delay(2000);

        if(!t) {
            fprintf(stderr, "Working time: %.2f\n", time_stop(start0)/1000.);
        } else {
            t->set_status_text("Working time: %.2f", time_stop(start0)/1000.);
        }

        getCounters(boardList, counters1, 2*N);
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

u32 make_addr(u32 bar, u8 hwAddr, u8 fpgaNum)
{
    return (bar + hwAddr*0x10000 + fpgaNum*0x1000);
}

//-----------------------------------------------------------------------------

u32 make_sign(u8 hwAddr, u8 fpgaNum)
{
    u32 chan = (2*(hwAddr-2) + fpgaNum);

    switch(chan) {
    case 0x0: return (0x11110000 | (hwAddr << 8 | fpgaNum));
    case 0x1: return (0x22220000 | (hwAddr << 8 | fpgaNum));
    case 0x2: return (0x33330000 | (hwAddr << 8 | fpgaNum));
    case 0x3: return (0x44440000 | (hwAddr << 8 | fpgaNum));
    case 0x4: return (0x55550000 | (hwAddr << 8 | fpgaNum));
    case 0x5: return (0x66660000 | (hwAddr << 8 | fpgaNum));
    case 0x6: return (0x77770000 | (hwAddr << 8 | fpgaNum));
    case 0x7: return (0x88880000 | (hwAddr << 8 | fpgaNum));
    case 0x8: return (0x99990000 | (hwAddr << 8 | fpgaNum));
    case 0x9: return (0xAAAA0000 | (hwAddr << 8 | fpgaNum));
    case 0xA: return (0xBBBB0000 | (hwAddr << 8 | fpgaNum));
    default: return 0x12345678;
    }
}

//-----------------------------------------------------------------------------

void program_rx(vector<acdsp*>& boardList)
{
    unsigned N = boardList.size();

    fprintf(stderr, " _____________________________________________________________________ \n");
    fprintf(stderr, "|                                                                     |\n");
    fprintf(stderr, "|                         SYSTEM RX CHANNELS                          |\n");
    fprintf(stderr, "|_____________________________________________________________________|\n");

    for(unsigned i=0; i<N; ++i) {

        acdsp *Brdi = boardList.at(i);
        pe_chn_rx* RXi = Brdi->get_chan_rx();
        trd_check* CHECKi = Brdi->get_trd_check();

        AMB_CONFIGURATION cfgRXi;
        Brdi->infoFpga(2, cfgRXi);

        for(unsigned j=0; j<N; ++j) {

            acdsp *Brdj = boardList.at(j);

            U32 signTX0j = make_sign(Brdj->slotNumber(), 0);
            U32 addrTX0j = make_addr(cfgRXi.PhysAddress[2], Brdj->slotNumber(), 0);

            fprintf(stderr, "0x%.8X  ", addrTX0j);

            RXi->set_fpga_addr(2*j, addrTX0j, signTX0j);

            U32 signTX1j = make_sign(Brdj->slotNumber(), 1);
            U32 addrTX1j = make_addr(cfgRXi.PhysAddress[2], Brdj->slotNumber(), 1);

            fprintf(stderr, "0x%.8X  ", addrTX1j);

            RXi->set_fpga_addr(2*j+1, addrTX1j, signTX1j);
        }

        fprintf(stderr, "\n");

        CHECKi->start_check(true);
        RXi->start_rx(true);
    }
}

//-----------------------------------------------------------------------------

void program_tx(vector<acdsp*>& boardList)
{
    unsigned N = boardList.size();
    unsigned M = 2;
    unsigned index = 0;

    fprintf(stderr, " _____________________________________________________________________ \n");
    fprintf(stderr, "|                                                                     |\n");
    fprintf(stderr, "|                         SYSTEM TX CHANNELS                          |\n");
    fprintf(stderr, "|_____________________________________________________________________|\n");

    for(unsigned i=0; i<N; i++) {

        acdsp *Brdi = boardList.at(i);

        for(unsigned j=0; j<M; j++) {

            pe_chn_tx* TX = Brdi->get_chan_tx(j);

            fprintf(stderr, "TX%d: ", j);

            U32 signTXj = make_sign(Brdi->slotNumber(), j);

            for(unsigned k=0; k<N; k++) {

                index = ((k + i*(N-1)) % N);

                acdsp *Brdk = boardList.at(index);
                AMB_CONFIGURATION cfgRXk;
                Brdk->infoFpga(2, cfgRXk);

                U32 addrTXj = make_addr(cfgRXk.PhysAddress[2], Brdi->slotNumber(), j);

                if((j%2) == 0) {

                    fprintf(stderr, "0x%.8X  ---------- ", addrTXj);
                    TX->set_fpga_addr(k, addrTXj, signTXj, 0);

                } else {

                    fprintf(stderr, "----------  0x%.8X ", addrTXj);
                    TX->set_fpga_addr(k, addrTXj, signTXj, 1);
                }
            }

            fprintf(stderr, "\n");
            TX->set_fpga_wait(0);
            TX->start_tx(true);
        }
    }
}

//-----------------------------------------------------------------------------

void stop_all_fpga(vector<acdsp*>& boardList)
{
    for(unsigned i=0; i<boardList.size(); i++) {

        acdsp *Brdi = boardList.at(i);

        pe_chn_tx* TX0i = Brdi->get_chan_tx(0);
        pe_chn_tx* TX1i = Brdi->get_chan_tx(1);

        TX0i->start_tx(false);
        TX1i->start_tx(false);
    }

    for(unsigned i=0; i<boardList.size(); i++) {

        acdsp *Brdi = boardList.at(i);

        pe_chn_rx* RXi = Brdi->get_chan_rx();
        trd_check* CHECKi = Brdi->get_trd_check();

        RXi->start_rx(false);
        CHECKi->start_check(false);
    }
}

//-----------------------------------------------------------------------------

bool start_pcie_test(std::vector<acdsp*>& boardList, struct app_params_t& params)
{
    fprintf(stderr, "DSP_FPGA: %d\n", boardList.size());
    fprintf(stderr, "ADC_FPGA: %d\n", 2*boardList.size());

    program_rx(boardList);
    program_tx(boardList);

    show_test_result(boardList);

    stop_all_fpga(boardList);

    IPC_delay(500);

    return true;
}

//-----------------------------------------------------------------------------
