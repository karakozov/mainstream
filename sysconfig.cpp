
#include "sysconfig.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>

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

    fprintf(stderr, "Found %ld FPGA\n", fpgaList.size());

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
