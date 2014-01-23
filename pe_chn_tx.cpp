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
}

//-------------------------------------------------------------------

pe_chn_tx::~pe_chn_tx()
{
    start_tx(false);
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_addr(int devnum, u32 dst_fpga_addr)
{
    m_fpga->FpgaBlockWrite(m_tx.number, 0xC + devnum, (dst_fpga_addr|0x1));
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_sign(u32 sign)
{
    u32 gen_sig = m_fpga->FpgaBlockRead(m_tx.number, 0x9);

    gen_sig |= (sign  << 8);

    m_fpga->FpgaBlockWrite(m_tx.number, 0x9, gen_sig);
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_chan(u32 chan)
{
    u32 gen_sig = m_fpga->FpgaBlockRead(m_tx.number, 0x9);

    gen_sig |= (chan & 0xff);

    m_fpga->FpgaBlockWrite(m_tx.number, 0x9, gen_sig);
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_test(u32 mode)
{
    m_fpga->FpgaBlockWrite(m_tx.number, 0x9, mode);
}

//-------------------------------------------------------------------

u32 pe_chn_tx::tx_block_number()
{
    return m_fpga->FpgaBlockRead(m_tx.number, 0x1F);
}

//-------------------------------------------------------------------

void pe_chn_tx::start_tx(bool start)
{
    if(start) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, (0x3 << 5));
    } else {
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, 0x0);
    }
}

//-------------------------------------------------------------------

void pe_chn_tx::write_test_block(u32 dst_fpga_addr, u32 count)
{
    u32* rx_bar = (u32*)dst_fpga_addr;
    for(u32 i=0; i<count; i++) {
        u32 val = ((i+1) << 24) | ((i+1) << 16) | ((i+1) << 8) | ((i+1) << 0);
        rx_bar[i] = val;
    }
}

//-------------------------------------------------------------------
