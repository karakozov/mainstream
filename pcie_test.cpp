#include "pcie_test.h"
#include "sysconfig.h"
#include "iniparser.h"
#include "isvi.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

#ifndef USE_GUI
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
#ifndef USE_GUI
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
#endif
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
#ifndef USE_GUI
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
#endif

//-----------------------------------------------------------------------------

#ifndef USE_GUI
void show_test_result(vector<acdsp*>& boardList)
{
    std::vector<counter_t>      counters0;
    std::vector<counter_t>      counters1;
    std::vector<pcie_speed_t>   dataRate;

    bool stop_flag = false;
    unsigned N = boardList.size();

    table *t = create_display_table(boardList);
    if(!t) {
        return;
    }

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
    }
    if(t) delete t;
//    for(unsigned i=0; i<N; ++i) {
//
//        acdsp *Brdi = boardList.at(i);
//        trd_check* CHECKi = Brdi->get_trd_check();
//        CHECKi->show_report(0, 0);
//        CHECKi->show_report(0, 1);
//    }
}
#endif

//-----------------------------------------------------------------------------

static u32 make_addr(u32 bar, u8 hwAddr, u8 fpgaNum)
{
    return (bar + hwAddr*0x10000 + fpgaNum*0x1000);
}

//-----------------------------------------------------------------------------

static u32 make_sign(u8 hwAddr, u8 fpgaNum)
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

typedef struct board_tx {
    vector<u32> tx[2];
} board_tx;

//-----------------------------------------------------------------------------

static int parse_line(string &str, vector<u32> &data)
{
    data.clear();

    // заменяем символы табуляции в строке на пробелы
    for(unsigned i=0; i<str.length(); i++) {
        if(str.at(i) == '\t' || str.at(i) == '\n')
            str[i] = ' ';
    }

    int start = 0;
    int stop = 0;

    do {

        start = str.find_first_not_of(" ", stop);
        stop = str.find_first_of(" ", start);

        if(start == -1)
            break;

        char *offset = 0;
        string s = str.substr(start,stop - start);

        u32 value = 0;

        if(strstr(s.c_str(), "*")) {
          value = 254;
        } else if(strstr(s.c_str(), "-")) {
          value = 255;
        } else {
          value = strtod(s.c_str(), &offset);
        }

        data.push_back(value);

    } while (1);

    return data.size();
}

//------------------------------------------------------------------------------

static bool read_tx_config(string fileName, unsigned N, vector<board_tx>& boardTxList)
{
    boardTxList.clear();

    ifstream ifs(fileName.c_str());
    if(!ifs.is_open()) {
        return false;
    }

    for(unsigned i=0; i<N; i++) {

        board_tx brdTx;

        for(unsigned j=0; j<2; j++) {
            string str = "";
            getline(ifs, str);
            vector<u32> data;
            if(parse_line(str, data)) {
                brdTx.tx[j] = data;
            }
        }

        boardTxList.push_back(brdTx);

        if(!ifs)
            break;
    }

    return true;
}

//-----------------------------------------------------------------------------

void program_tx(vector<acdsp*>& boardList, string fileName)
{
    vector<board_tx> boardTxList;
    unsigned N = boardList.size();
    unsigned M = 2;
    unsigned index = 0;

    fprintf(stderr, " _____________________________________________________________________ \n");
    fprintf(stderr, "|                                                                     |\n");
    fprintf(stderr, "|                         SYSTEM TX CHANNELS                          |\n");
    fprintf(stderr, "|_____________________________________________________________________|\n");

    if( read_tx_config(fileName, N, boardTxList) ) {

        for(unsigned i=0; i<N; i++) {

            acdsp *Brdi = boardList.at(i);
            board_tx& brdTXi = boardTxList.at(i);

            for(unsigned j=0; j<M; j++) {

                pe_chn_tx* TX = Brdi->get_chan_tx(j);
                U32 signTXj = make_sign(Brdi->slotNumber(), j);
                vector<u32>& indexTX = brdTXi.tx[j];

                fprintf(stderr, "TX%d: ", j);

                for(unsigned k=0; k<indexTX.size(); k++) {

                    index = indexTX.at(k);

                    if(index == 255) {

                      fprintf(stderr, " ---------- " );
                      TX->set_fpga_addr(k, 0, 0, 1);

                    } else if(index == 254) {

                      fprintf(stderr, " ********** " );
                      TX->set_fpga_addr(k, 0, 0, 0);

                    } else {

                      acdsp *Brdk = boardList.at(index);
                      AMB_CONFIGURATION cfgRXk;
                      Brdk->infoFpga(2, cfgRXk);

                      U32 addrTXj = make_addr(cfgRXk.PhysAddress[2], Brdi->slotNumber(), j);

                      fprintf(stderr, " 0x%.8X ", addrTXj);
                      TX->set_fpga_addr(k, addrTXj, signTXj, 3);
                    }
                }

                fprintf(stderr, "\n");
                TX->set_fpga_wait(1*900);
                TX->start_tx(true);
            }
        }

    } else {

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
                        TX->set_fpga_addr(k, addrTXj, signTXj, 3);

                    } else {

                        fprintf(stderr, "----------  0x%.8X ", addrTXj);
                        TX->set_fpga_addr(k, addrTXj, signTXj, 3);
                    }
                }

                fprintf(stderr, "\n");
                TX->set_fpga_wait(1*900);
                TX->start_tx(true);
            }
        }
    }
}

//-----------------------------------------------------------------------------

void start_all_fpga(vector<acdsp*>& boardList)
{
    reverse(boardList.begin(),boardList.end());
    program_rx(boardList);
    program_tx(boardList, "tx.channels");
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
    reverse(boardList.begin(),boardList.end());
}

//-----------------------------------------------------------------------------
#ifndef USE_GUI
bool start_pcie_test(std::vector<acdsp*>& boardList, struct app_params_t& params)
{
    fprintf(stderr, "DSP_FPGA: %ld\n", boardList.size());
    fprintf(stderr, "ADC_FPGA: %ld\n", 2*boardList.size());

    start_all_fpga(boardList);
    show_test_result(boardList);
    stop_all_fpga(boardList);

    IPC_delay(500);

    return true;
}
#endif
//-----------------------------------------------------------------------------
