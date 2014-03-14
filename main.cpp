
#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include "pcie_test.h"
#include "acdsp.h"
#include "acsync.h"
#include "iniparser.h"
#include "fpga_base.h"
#include "exceptinfo.h"
#include "fpga.h"
#include "isvi.h"

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

std::vector<Fpga*> fpgaList;
std::vector<acdsp*> boardList;
acsync *sync_board = 0;

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct app_params_t params;

    if(!getParams(argc, argv, params)) {
        fprintf(stderr, "Error get parameters from file: %s\n", argv[1]);
        return -1;
    }

    showParams(params);

#if USE_SIGNAL
    signal(SIGINT, stop_exam);
#else
    IPC_initKeyboard();
#endif

    try {

        create_fpga_list(fpgaList, 10, 0);
        create_board_list(fpgaList, boardList, &sync_board, params.boardMask);

        if(sync_board) {
            sync_board->PowerON(true);
            sync_board->progFD(params.syncMode, params.syncSelClkOut, params.syncFd, params.syncFo);
        }

        if(params.testMode == 3) {

            start_pcie_test(boardList, params);
            IPC_delay(1000);

        } else {

            for(unsigned i=0; i<boardList.size(); ++i) {

                acdsp* brd = boardList.at(i);

                //---------------------------------------------------- DATA FROM STREAM ADC

                if(params.fpgaMask != 0) {

                    if(params.testMode == 0)
                        brd->dataFromAdc(params);

                    //---------------------------------------------------- DDR3 FPGA AS MEMORY

                    if(params.testMode == 1)
                        brd->dataFromMemAsMem(params);

                    //---------------------------------------------------- DDR3 FPGA AS FIFO

                    if(params.testMode == 2)
                        brd->dataFromMemAsFifo(params);

                    //----------------------------------------------------

                } else {

                    //---------------------------------------------------- DATA FROM MAIN STREAM

                    if(params.testMode == 0)
                        brd->dataFromMain(params);

                    //---------------------------------------------------- DDR3 FPGA AS MEMORY

                    if(params.testMode == 1)
                        brd->dataFromMainToMemAsMem(params);

                    //---------------------------------------------------- DDR3 FPGA AS FIFO

                    if(params.testMode == 2)
                        brd->dataFromMainToMemAsFifo(params);

                    //----------------------------------------------------
                }
            }

        }

        delete_board_list(boardList,sync_board);
        delete_fpga_list(fpgaList);
    }
    catch(except_info_t err) {
        fprintf(stderr, "%s", err.info.c_str());
        delete_board_list(boardList,sync_board);
        delete_fpga_list(fpgaList);
    }
    catch(...) {
        fprintf(stderr, "Exception was generated in the program. Exit from application.\n");
        delete_board_list(boardList,sync_board);
        delete_fpga_list(fpgaList);
    }

#if !USE_SIGNAL
    IPC_cleanupKeyboard();
#endif

    fprintf(stderr, "STOP\n");

    return 0;
}

//-----------------------------------------------------------------------------
