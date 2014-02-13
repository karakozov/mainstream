#include "pe_chn_tx.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

pe_chn_tx::pe_chn_tx(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaBlock(0, 0x1C, m_tx)) {
        fprintf(stderr, "Not found PE_CHN_TX! ID: 0x%x", 0x1C);
        throw;
    }
    reset();
}

//-------------------------------------------------------------------

pe_chn_tx::~pe_chn_tx()
{
    start_tx(false);
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_addr(u32 chan, u32 dst_fpga_addr, u32 sign)
{
    // set TX signature and channel number
    m_fpga->FpgaBlockWrite(m_tx.number, 0x9, sign);

    // set TX address
    m_fpga->FpgaBlockWrite(m_tx.number, 0xC + chan, (dst_fpga_addr|0x1));
}

//-------------------------------------------------------------------

u32 pe_chn_tx::tx_block_number()
{
    return m_fpga->FpgaBlockRead(m_tx.number, 0x1F);
}

//-------------------------------------------------------------------

u32 pe_chn_tx::tx_overflow()
{
    return m_fpga->FpgaBlockRead(m_tx.number, 0x1E);
}

//-------------------------------------------------------------------

void pe_chn_tx::start_tx(bool start)
{
    u32 ctrl = m_fpga->FpgaBlockRead(m_tx.number, 0x8);

    ctrl |= 0x10; //TX_CHN_ON

    if(start) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, (ctrl | (0x3 << 5)));
    } else {
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, (ctrl & (~(0x3 << 5))));
    }
}

//-------------------------------------------------------------------

void pe_chn_tx::reset()
{
    m_fpga->resetTrd(m_tx.number);
    m_fpga->resetFifo(m_tx.number);
}

//-------------------------------------------------------------------
