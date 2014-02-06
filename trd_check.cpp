#include "trd_check.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

trd_check::trd_check(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaTrd(0, 0xCC, m_check)) {
        fprintf(stderr, "Not found TRD_CHN_CHECK! ID: 0x%x", 0xCC);
        throw;
    }
}

//-------------------------------------------------------------------

trd_check::~trd_check()
{
    start_check(false);
}

//-------------------------------------------------------------------

u32 trd_check::rd_block_number(u32 chan)
{
    u32 val = 0;
    u16 rd_l = 0;
    u16 rd_h = 0;

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4000);
    rd_l = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4001);
    rd_h = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    val = (rd_h << 16) | rd_l;

    return val;
}

//-------------------------------------------------------------------

u32 trd_check::wr_block_number(u32 chan)
{
    u32 val = 0;
    u16 wr_l = 0;
    u16 wr_h = 0;

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4006);
    wr_l = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4007);
    wr_h = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    val = (wr_h << 16) | wr_l;

    return val;
}

//-------------------------------------------------------------------

u32 trd_check::ok_block_number(u32 chan)
{
    u32 val = 0;
    u16 ok_l = 0;
    u16 ok_h = 0;

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4002);
    ok_l = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4003);
    ok_h = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    val = (ok_h << 16) | ok_l;

    return val;
}

//-------------------------------------------------------------------

u32 trd_check::err_block_number(u32 chan)
{
    u32 val = 0;
    u16 err_l = 0;
    u16 err_h = 0;

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4004);
    err_l = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, 0x4005);
    err_h = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);

    val = (err_h << 16) | err_l;

    return val;
}

//-------------------------------------------------------------------
#define REPORT_WORD      16
#define VALID_WORDS      11
//-------------------------------------------------------------------

void trd_check::show_report(u32 chan)
{
    u16 word[REPORT_WORD];
    report_word_t val;

    memset(word, 0, REPORT_WORD * sizeof(u16));

    for(unsigned i=0; i<VALID_WORDS; i++) {
      m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, chan * REPORT_WORD + i);
      word[i] = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);
    }

    val.w16.read_d0 = word[0];
    val.w16.read_d1 = word[1];
    val.w16.read_d2 = word[2];
    val.w16.read_d3 = word[3];

    val.w16.expect_d0 = word[4];
    val.w16.expect_d1 = word[5];
    val.w16.expect_d2 = word[6];
    val.w16.expect_d3 = word[7];

    val.w16.index = word[8];

    val.w16.block_d0 = word[9];
    val.w16.block_d1 = word[10];

    val = val;
}

//-------------------------------------------------------------------

void trd_check::start_check(bool start)
{
    if(start) {
        m_fpga->FpgaRegPokeInd(m_check.number, 0x0, 0x2038);
    } else {
        m_fpga->FpgaRegPokeInd(m_check.number, 0x0, 0x0);
    }
}

//-------------------------------------------------------------------
