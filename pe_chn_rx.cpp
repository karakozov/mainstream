#include "pe_chn_rx.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

pe_chn_rx::pe_chn_rx(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaBlock(0, 0x1D, m_rx)) {
        fprintf(stderr, "Not found PE_CHN_RX! ID: 0x%x", 0x1D);
        throw;
    }
}

//-------------------------------------------------------------------

pe_chn_rx::~pe_chn_rx()
{
    start_rx(false);
}

//-------------------------------------------------------------------

void pe_chn_rx::set_fpga_chan(u32 chan)
{
    // select channel
    m_fpga->FpgaBlockWrite(m_rx.number, 0xA, (chan & 0x7));
}

//-------------------------------------------------------------------

void pe_chn_rx::set_fpga_addr(u32 chan, u32 src_fpga_addr, u32 sign)
{
    set_fpga_chan(chan);

    // set fpga_adc src addr
    m_fpga->FpgaBlockWrite(m_rx.number, 0xC, src_fpga_addr | 0x1);

    // set fpga_adc src sign
    u32 gen_sig = m_fpga->FpgaBlockRead(m_rx.number, 0x9);
    gen_sig |= (sign  << 8);
    m_fpga->FpgaBlockWrite(m_rx.number, 0x9, gen_sig);
}

//-------------------------------------------------------------------

u32 pe_chn_rx::rx_block_number(u32 chan)
{
    set_fpga_chan(chan);
    return m_fpga->FpgaBlockRead(m_rx.number, 0x10);
}

//-------------------------------------------------------------------

u32 pe_chn_rx::sign_err_number(u32 chan)
{
    // select channel
    set_fpga_chan(chan);
    return m_fpga->FpgaBlockRead(m_rx.number, 0x11);
}

//-------------------------------------------------------------------

u32 pe_chn_rx::block_err_number(u32 chan)
{
    set_fpga_chan(chan);
    return m_fpga->FpgaBlockRead(m_rx.number, 0x12);
}

//-------------------------------------------------------------------

void pe_chn_rx::start_rx(bool start)
{
    if(start) {
        m_fpga->FpgaBlockWrite(m_rx.number, 0x8, (0x1 << 5));
    } else {
        m_fpga->FpgaBlockWrite(m_rx.number, 0x8, 0x0);
    }
}

//-------------------------------------------------------------------
