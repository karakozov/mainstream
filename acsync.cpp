
#include "fpga.h"
#include "acsync.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------

acsync::acsync(Fpga *fpga) : m_fpga(fpga)
{
    m_exit = false;
    m_FD = 0;
    m_adf_regs.addr = 0;
    m_adf_regs.reg0 = 0;
    m_adf_regs.reg1 = 0;
    m_adf_regs.reg2 = 0;
    m_adf_regs.reg3 = 0;
    m_FVCO_ADF4002 = 56;
    m_slotNumber = 0;

    fprintf(stderr, "====================== Open ACSYNC =====================\n");

    if(!m_fpga->fpgaTrd(0, 0xB1, m_sync_trd)) {
        throw except_info("%s, %d: %s() - Error AC_SYNC tetrade not found.\n", __FILE__, __LINE__, __FUNCTION__);
    }

    U08 hwAddr = 0xff;
    U08 fpgaNum = 0xff;
    bool okhw = m_fpga->FpgaHwAddress(hwAddr, fpgaNum);
    if(okhw) {
        if(!m_slotNumber) {
            m_slotNumber = hwAddr;
        }
    }

    fillCxDx();

    PowerON(true);

    fprintf(stderr, "========================================================\n");
}

//-----------------------------------------------------------------------------

acsync::~acsync()
{
    PowerON(false);
}

//-----------------------------------------------------------------------------

Fpga* acsync::FPGA(unsigned fpgaNum)
{
    return m_fpga;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                     REGISTER ACCESS LEVEL INTERFACE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void acsync::RegPokeInd(U32 fpgaNum, S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA(fpgaNum)->FpgaRegPokeInd(TetrNum, RegNum, RegVal);
}

//-----------------------------------------------------------------------------

U32 acsync::RegPeekInd(U32 fpgaNum, S32 TetrNum, S32 RegNum)
{
    return FPGA(fpgaNum)->FpgaRegPeekInd(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------

void acsync::RegPokeDir(U32 fpgaNum, S32 TetrNum, S32 RegNum, U32 RegVal)
{
    FPGA(fpgaNum)->FpgaRegPokeDir(TetrNum,RegNum,RegVal);
}

//-----------------------------------------------------------------------------

U32 acsync::RegPeekDir(U32 fpgaNum, S32 TetrNum, S32 RegNum)
{
    return FPGA(fpgaNum)->FpgaRegPeekDir(TetrNum, RegNum);
}

//-----------------------------------------------------------------------------

void acsync::setExitFlag(bool exit)
{
    m_exit = exit;
}

//-----------------------------------------------------------------------------

bool acsync::exitFlag()
{
    if(IPC_kbhit())
        return true;
    return m_exit;
}

//-----------------------------------------------------------------------------

void acsync::selclkMode1(U32 FO)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    U32 selclk = RegPeekInd(0, m_sync_trd.number, 0xF);

    mode1 &= ~(ENPOW_OCXO | ENx5 | ENx8);
    selclk &= ~0x1FF;

    switch(FO) {
    case 10:
    case 56:
    case 120:{
        mode1 |= ENPOW_OCXO;
        selclk |= (SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    }

    selclk |= SELCLKOUT;

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);
    RegPokeInd(0, m_sync_trd.number, 0xF, selclk);
}

//-----------------------------------------------------------------------------

void acsync::selclkMode2(U32 FO)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    U32 selclk = RegPeekInd(0, m_sync_trd.number, 0xF);

    mode1 &= ~(ENPOW_OCXO | ENx5 | ENx8);
    selclk &= ~0x1FF;

    switch(FO) {
    case 10: {
        mode1 |= (ENx5 | ENx8);
        selclk |= (SELCLK0 | SELCLK1 | SELCLK2 | SELCLK5 | SELCLK6);
    } break;
    case 56: {
        mode1 |= ENx8;
        selclk |= (SELCLK0 | SELCLK2 | SELCLK4 | SELCLK6);
    } break;
    case 120: {
        mode1 |= ENx8;
        selclk |= (SELCLK0 | SELCLK2 | SELCLK4 | SELCLK6);
    } break;
    }

    selclk |= SELCLKOUT;

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);
    RegPokeInd(0, m_sync_trd.number, 0xF, selclk);
}

//-----------------------------------------------------------------------------

void acsync::selclkMode3(U32 FO)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    U32 selclk = RegPeekInd(0, m_sync_trd.number, 0xF);

    mode1 &= ~(ENPOW_OCXO | ENx5 | ENx8);
    selclk &= ~0x1FF;

    switch(FO) {
    case 10: {
        mode1 |= ENx5;
        selclk |= (SELCLK0 | SELCLK1 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    case 56: {
        selclk |= (SELCLK1 | SELCLK2 | SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    case 120: {
        selclk |= (SELCLK1 | SELCLK2 | SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    case 400: {
        selclk |= (SELCLK1 | SELCLK2 | SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    case 448: {
        selclk |= (SELCLK1 | SELCLK2 | SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    case 480: {
        selclk |= (SELCLK1 | SELCLK2 | SELCLK3 | SELCLK4 | SELCLK5 | SELCLK7);
    } break;
    }

    selclk |= SELCLKOUT;

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);
    RegPokeInd(0, m_sync_trd.number, 0xF, selclk);
}

//-----------------------------------------------------------------------------

void acsync::selclkMode4(U32 FO)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    U32 selclk = RegPeekInd(0, m_sync_trd.number, 0xF);

    mode1 &= ~(ENPOW_OCXO | ENx5 | ENx8);
    mode1 |= (ENx5 | ENx8);
    selclk = ~0x1FF;

    selclk |= (SELCLK1 | SELCLK2 | SELCLK4 | SELCLK5 | SELCLK6);
    selclk &= ~SELCLKOUT;

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);
    RegPokeInd(0, m_sync_trd.number, 0xF, selclk);
}

//-----------------------------------------------------------------------------

void acsync::FreqMultipler1(U32 mode, U32 FO)
{
    switch(mode) {
    case 0: selclkMode1(FO); break;
    case 1: selclkMode2(FO); break;
    case 2: selclkMode3(FO); break;
    case 3: selclkMode4(FO); break;
    }
}

//-----------------------------------------------------------------------------

U32 acsync::gcd(U32 freqA, U32 freqB)
{
    U32 c;
    while (freqB) {
        c = freqA % freqB;
        freqA = freqB;
        freqB = c;
    }
    return abs(freqA);
}

//-----------------------------------------------------------------------------

void acsync::calcADF4002(float FO, U32 Fvco, U32 variant)
{
    if(FO) {
        float Fd = gcd(U32(FO), Fvco);
        U32 R = floor((FO/Fd)+0.5);
        U32 N = floor((Fvco/Fd)+0.5);

        fprintf(stderr, "Fd = %f\n", Fd);
        fprintf(stderr, "R = %d\n", R);
        fprintf(stderr, "N = %d\n", N);

        U8 ablw = 1;
        U16 ref14 = (R & 0x3FFF);
        m_adf_regs.reg0 = (ablw << 16) | (ref14 << 2) | 0x0;

        U8 gpgain = 1;
        U16 ref13 = (N & 0x1FFF);
        m_adf_regs.reg1 = (gpgain << 21) | (ref13 << 8) | 0x1;

        m_adf_regs.reg2 = variant ? 0x1F90C2 : 0x1F90A2;
        m_adf_regs.reg3 = variant ? 0x1F90C3 : 0x1F90A3;
    } else {
        m_adf_regs.reg0 = 0x010014;
        m_adf_regs.reg1 = 0x201C01;
        m_adf_regs.reg2 = 0x1F90C6;
        m_adf_regs.reg3 = 0x1F90C7;
    }

    fprintf(stderr, "3: 0x%X\n", m_adf_regs.reg3);
    fprintf(stderr, "2: 0x%X\n", m_adf_regs.reg2);
    fprintf(stderr, "0: 0x%X\n", m_adf_regs.reg0);
    fprintf(stderr, "1: 0x%X\n", m_adf_regs.reg1);
}

//-----------------------------------------------------------------------------

void acsync::progADF4002(float FO, U32 Fvco, U32 variant)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    mode1 |= 0x2;
    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);

    calcADF4002(FO, Fvco, variant);

    writeADF4002(3, m_adf_regs.reg3 | (0x4 << 4) );
    writeADF4002(2, m_adf_regs.reg2 | (0x4 << 4));
    writeADF4002(0, m_adf_regs.reg0);
    writeADF4002(1, m_adf_regs.reg1);
}

//-----------------------------------------------------------------------------

void acsync::writeADF4002(U16 reg, U32 data)
{
    IPC_delay(10);
    RegPokeInd(0, m_sync_trd.number, 0xA, 0x4); IPC_delay(10);
    RegPokeInd(0, m_sync_trd.number, 0xC, reg); IPC_delay(10);
    RegPokeInd(0, m_sync_trd.number, 0xD, (data & 0xffff)); IPC_delay(10);
    RegPokeInd(0, m_sync_trd.number, 0xE, ((data >> 16) & 0xff)); IPC_delay(10);
    RegPokeInd(0, m_sync_trd.number, 0xB, (m_adf_regs.addr << 4) |0x2); IPC_delay(10);
    IPC_delay(100);
}

//-----------------------------------------------------------------------------

void acsync::FreqMultiplerDivider(U32 mode, float FD, float FO)
{
    U8 code_c4 = 1;
    U8 code_c5 = 1;
    U8 code_d1 = 1;
    U8 code_d2 = 1;

    switch(mode) {
    case 0: getMultCxDxMode0(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    case 1: getMultCxDxMode1(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    case 2: getMultCxDxMode2(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    default: return;
    }

    fprintf(stderr, "code_c4 = 0x%x\n", code_c4);
    fprintf(stderr, "code_c5 = 0x%x\n", code_c5);
    fprintf(stderr, "code_d1 = 0x%x\n", code_d1);
    fprintf(stderr, "code_d2 = 0x%x\n", code_d2);

    U8 C4 = getCxDxScale(code_c4);
    U8 C5 = getCxDxScale(code_c5);
    U8 D1 = getCxDxScale(code_d1);
    U8 D2 = getCxDxScale(code_d2);

    U32 div01 = ((C5 << 6) | C4);
    U32 div23 = ((D2 << 6) | D1);

    fprintf(stderr, "DIV01 = 0x%x\n", div01);
    fprintf(stderr, "DIV23 = 0x%x\n", div23);

    RegPokeInd(0, m_sync_trd.number, 0x10, div01);
    RegPokeInd(0, m_sync_trd.number, 0x11, div23);
}

//-----------------------------------------------------------------------------

void acsync::GetCxDxEncoded(U32 mode, float FD, float FO, U8 &C4, U8 &C5, U8 &D1, U8 &D2, U32& DIV01, U32& DIV23)
{
    U8 code_c4 = 1;
    U8 code_c5 = 1;
    U8 code_d1 = 1;
    U8 code_d2 = 1;

    switch(mode) {
    case 0: getMultCxDxMode0(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    case 1: getMultCxDxMode1(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    case 2: getMultCxDxMode2(FD, FO, code_c4, code_c5, code_d1, code_d2); break;
    default: return;
    }

    fprintf(stderr, "code_c4 = 0x%x\n", code_c4);
    fprintf(stderr, "code_c5 = 0x%x\n", code_c5);
    fprintf(stderr, "code_d1 = 0x%x\n", code_d1);
    fprintf(stderr, "code_d2 = 0x%x\n", code_d2);

    C4 = getCxDxScale(code_c4);
    C5 = getCxDxScale(code_c5);
    D1 = getCxDxScale(code_d1);
    D2 = getCxDxScale(code_d2);

    DIV01 = ((C5 << 6) | C4);
    DIV23 = ((D2 << 6) | D1);

    fprintf(stderr, "DIV01 = 0x%x\n", DIV01);
    fprintf(stderr, "DIV23 = 0x%x\n", DIV23);
}

//-----------------------------------------------------------------------------

U08 acsync::getCxDxScale(int code)
{
    if((code <= 0) || (code > 17)) {
        throw except_info("%s, %d: %s() - Invalid divider/multipler code: %d\n\n", __FILE__, __LINE__, __FUNCTION__, code);
    }

    fprintf(stderr, "m_Cx[%d] = 0x%X\n", code-1, m_Cx[code-1]);

    return m_Cx[code-1];
}

//-----------------------------------------------------------------------------

void acsync::getMultCxDxMode0(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2)
{
    fprintf(stderr, "%s(): FD = %f, FO = %f\n", __FUNCTION__, FD, FO);

    if(FD == 400.0 && FO == 10.0) {

        C4 = 10; C5 = 10;
        D1 = 7;  D2 = 2;

    } else if(FD == 448.0 && FO == 56.0) {

        C4 = 8; C5 = 8;
        D1 = 4; D2 = 2;

    } else if(FD >= 488.72 && FO == 56.0) {

        C4 = 12;  C5 = 8;
        D1 = 11; D2 = 1;

    } else if(FD == 480.0 && FO == 120) {

        C4 = 10; C5 = 6;
        D1 = 7;  D2 = 1;

    }

    fprintf(stderr, "%s(): C4 = %d, C5 = %d, D1 = %d, D2 = %d\n", __FUNCTION__, C4, C5, D1, D2);
}

//-----------------------------------------------------------------------------

void acsync::getMultCxDxMode1(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2)
{
    fprintf(stderr, "%s(): FD = %f, FO = %f\n", __FUNCTION__, FD, FO);

    if(FD == 400.0 && FO == 10.0) {

        C4 = 5; C5 = 2;
        D1 = 5; D2 = 2;

    } else if(FD == 448.0 && FO == 56.0) {

        C4 = 5; C5 = 2;
        D1 = 5; D2 = 2;

    } else if(FD >= 488.72 && FO == 56.0) {

        C4 = 6;  C5 = 2;
        D1 = 11; D2 = 1;

    } else if(FD == 480.0 && FO == 120) {

        C4 = 5; C5 = 2;
        D1 = 5; D2 = 2;

    }

    fprintf(stderr, "%s(): C4 = %d, C5 = %d, D1 = %d, D2 = %d\n", __FUNCTION__, C4, C5, D1, D2);
}

//-----------------------------------------------------------------------------

void acsync::getMultCxDxMode2(float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2)
{
    fprintf(stderr, "%s(): FD = %f, FO = %f\n", __FUNCTION__, FD, FO);

    if(FO == 10.0) {

        if(FD == 400.0) {

            C4 = 16; C5 = 5;
            D1 = 5; D2 = 2;

        }
    }

    if(FO == 56.0) {

        if(FD == 400.0) {

            C4 = 10; C5 = 10;
            D1 = 7;  D2 = 2;

        } else if(FD == 448.0) {

            C4 = 8; C5 = 8;
            D1 = 4; D2 = 2;

        } else if(FD >= 488.72) {

            C4 = 12; C5 = 8;
            D1 = 11; D2 = 1;

        } else if(FD == 480.0) {

            C4 = 10; C5 = 6;
            D1 = 7;  D2 = 1;

        }
    }

    if(FO == 120.0) {

        if(FD == 480.0) {

            C4 = 8; C5 = 5;
            D1 = 5; D2 = 2;
        }
    }

    if(FO == 400.0) {

        if(FD == 400.0) {

            C4 = 2; C5 = 5;
            D1 = 5; D2 = 2;
        }
    }

    if(FO == 448.0) {

        if(FD == 448.0) {

            C4 = 2; C5 = 5;
            D1 = 5; D2 = 2;
        }
        if(FD >= 488.72) {

            D1 = 11; D2 = 1;
            C4 = 6; C5 = 2;
        }
    }

    if(FO == 480.0) {

        if(FD == 480.0) {

            C4 = 5; C5 = 2;
            D1 = 5; D2 = 2;
        }
    }

    fprintf(stderr, "%s(): C4 = %d, C5 = %d, D1 = %d, D2 = %d\n", __FUNCTION__, C4, C5, D1, D2);
}

//-----------------------------------------------------------------------------

void acsync::fillCxDx()
{
    m_Cx[0] = 0x20;
    m_Cx[1] = 0x01;
    m_Cx[2] = 0x03;
    m_Cx[3] = 0x04;

    m_Cx[4] = 0x06;
    m_Cx[5] = 0x08;
    m_Cx[6] = 0x0A;
    m_Cx[7] = 0x0C;

    m_Cx[8] = 0x0E;
    m_Cx[9] = 0x10;
    m_Cx[10] = 0x12;
    m_Cx[11] = 0x14;

    m_Cx[12] = 0x16;
    m_Cx[13] = 0x18;
    m_Cx[14] = 0x1A;
    m_Cx[15] = 0x1C;

    m_Cx[16] = 0x1E;
}

//-----------------------------------------------------------------------------

bool acsync::checkFrequencyParam(U32 mode, float FD, float FO)
{
    bool ok = false;

    //-----------------------------
    fprintf(stderr, "%s(): MODE = 0x%x, FO = %f, FD = %f\n", __FUNCTION__, mode, FO, FD);
    //-----------------------------

    if(mode == 0) {

        if(FO == 10.0) {
            if(FD == 400.0) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 56.0) {
            if(FD == 448.0 || FD >= 488.72) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 120.0) {
            if(FD == 480.0) {
                ok = true;
                goto finished;
            }
        }
    }

    //-----------------------------

    if(mode == 1) {

        if(FO == 10.0) {
            if(FD == 400.0) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 56.0) {
            if(FD == 448.0 || FD >= 488.72) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 120.0) {
            if(FD == 480.0) {
                ok = true;
                goto finished;
            }
        }
    }

    //-----------------------------

    if(mode == 2) {

        if(FO == 10.0) {
            if(FD == 400.0) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 56.0) {
            if(FD == 400.0 || FD == 448.0 || FD == 480.0 || FD >= 488.72) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 120.0) {
            if(FD == 480.0) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 400.0) {
            if(FD == 400.0) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 448.0) {
            if(FD == 448.0 || FD >= 488.72 ) {
                ok = true;
                goto finished;
            }
        }

        if(FO == 480.0) {
            if(FD == 480.0) {
                ok = true;
                goto finished;
            }
        }
    }

    //-----------------------------

    if(mode == 3) {
        ok = true;
        goto finished;
    }

finished:
    if(ok) {
        m_FD = FD;
    } else {
        m_FD = 0.0;
        fprintf(stderr, "Unsupported frequency combinaton! FO = %f, FD = %f\n", FO, FD);
    }

    return ok;
}

//-----------------------------------------------------------------------------

bool acsync::progFD(U32 mode, float FD, float FO)
{
    if(!checkFrequencyParam(mode, FD, FO)) {
        fprintf(stderr, "checkFrequencyParam() error!\n");
        return false;
    }

    FreqMultipler1(mode, FO);

    if(mode == 0) {
        progADF4002(FO, m_FVCO_ADF4002);
    }

    FreqMultiplerDivider(mode, FD, FO);

    fprintf(stderr, "%s(): MODE = 0x%x, FO = %f, FD = %f\n", __FUNCTION__, mode, FO, FD);

    IPC_delay(5);

    return true;
}

//-----------------------------------------------------------------------------

void acsync::PowerON(bool on)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    if(on)
        mode1 |= (ENPOW_PLL | ENPOW_CLKSM);
    else
        mode1 &= ~(ENPOW_PLL | ENPOW_CLKSM);

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);

    IPC_delay(500);
}

//-----------------------------------------------------------------------------

void acsync::ResetSync(bool on)
{
    U32 mode1 = RegPeekInd(0, m_sync_trd.number, 0x9);
    if(on) {
        mode1 &= ~ST_IN_RST;
    } else {
        mode1 |= ST_IN_RST;
    }

    RegPokeInd(0, m_sync_trd.number, 0x9, mode1);
}

//-----------------------------------------------------------------------------

void acsync::getCxDxValues(U32 mode, float FD, float FO, U8& C4, U8& C5, U8& D1, U8& D2)
{
    switch(mode) {
    case 0: return getMultCxDxMode0(FD, FO, C4, C5, D1, D2);
    case 1: return getMultCxDxMode1(FD, FO, C4, C5, D1, D2);
    case 2: return getMultCxDxMode2(FD, FO, C4, C5, D1, D2);
    case 3: C4 = 0; C5 = 0; D1 = 0; D2 = 0; break;
    }
}

//-----------------------------------------------------------------------------
