#include "trd_check.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

trd_check::trd_check(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaTrd(0, 0xB0, m_check)) {
        fprintf(stderr, "Not found TRD_CHN_CHECK! ID: 0x%x", 0xB0);
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

bool trd_check::read_report_word(u32 chan, u32 chx_data, u32 word_num, report_word_t& word)
{
    u32 word_offset = chx_data * 0x1000 + word_num * REPORT_WORD_SIZE;

    if((chx_data > 1) || (word_num > REPORT_WORD_COUNT)) {
        return false;
    }

    memset(&word, 0, sizeof(word));

    for(int i=0; i<REPORT_WORD_FIELDS; i++) {
        m_fpga->FpgaRegPokeInd(m_check.number, 0x10 + chan, word_offset + i);
        word.fields[i] = m_fpga->FpgaRegPeekInd(m_check.number, 0x208 + chan);
    }
/*
    fprintf(stderr, "0x%.4x%.4x: "
                    "0x%.4x - "
                    "readed: 0x%.4x%.4x%.4x%.4x "
                    "expect: 0x%.4x%.4x%.4x%.4x\n",
            word.w16.block_d1, word.w16.block_d0,
            word.w16.index,
            word.w16.read_d3, word.w16.read_d2, word.w16.read_d1, word.w16.read_d0,
            word.w16.expect_d3, word.w16.expect_d2, word.w16.expect_d1, word.w16.expect_d0);
*/
    return true;
}

//-------------------------------------------------------------------

bool trd_check::show_report(u32 chan, u32 chx_data)
{
    fprintf(stderr, "CHANNEL %d. BITS: %s\n", chan, chx_data ? "127 - 64" : "63 - 0" );

    for(unsigned i=0; i<REPORT_WORD_COUNT; i++) {

        report_word_t w;

        if(!read_report_word(chan, chx_data, i, w)) {
            return false;
        }

        fprintf(stderr, "%.8d: 0x%.4x expect - 0x%.16llx  readed - 0x%.16llx\n", w.w64.block, w.w64.index, w.w64.expect, w.w64.read);
    }

    return true;
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
