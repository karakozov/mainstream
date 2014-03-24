#include "pe_chn_rx.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

pe_chn_rx::pe_chn_rx(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaBlock(0, 0x1D, m_rx)) {
        throw except_info("%s, %d: %s() - Not found PE_CHN_RX! ID: 0x%x\n", __FILE__, __LINE__, __FUNCTION__, 0x1D);
    }
    reset();
}

//-------------------------------------------------------------------

pe_chn_rx::~pe_chn_rx()
{
    start_rx(false);
}

//-------------------------------------------------------------------

void pe_chn_rx::set_fpga_addr(u32 chan, u32 tx_addr, u32 tx_sign)
{
    // select channel
    //fprintf(stderr, "REG: 0xA DATA 0x%x\n", (chan & 0x7));
    m_fpga->FpgaBlockWrite(m_rx.number, 0xA, (chan & 0x7));

    // set fpga_adc src addr
    //fprintf(stderr, "REG: 0xC DATA 0x%x\n", src_fpga_addr | 0x1);
    m_fpga->FpgaBlockWrite(m_rx.number, 0xC, tx_addr | 0x3);

    // set fpga_adc src sign
    //fprintf(stderr, "REG: 0x9 DATA 0x%x\n", sign);
    m_fpga->FpgaBlockWrite(m_rx.number, 0x9, tx_sign);

    fprintf(stderr, "\n");
    fprintf(stderr, "ENTRY %.2d: | ADDR 0x%.8X\t | SIGN 0x%.8X\n", chan, tx_addr | 3, tx_sign);
    fprintf(stderr, "_____________________________________________________________________\n");
}

//-------------------------------------------------------------------

u32 pe_chn_rx::rx_block_number(u32 chan)
{
    m_fpga->FpgaBlockWrite(m_rx.number, 0xA, (chan & 0x7));
    return m_fpga->FpgaBlockRead(m_rx.number, 0x10);
}

//-------------------------------------------------------------------

u32 pe_chn_rx::sign_err_number(u32 chan)
{
    m_fpga->FpgaBlockWrite(m_rx.number, 0xA, (chan & 0x7));
    return m_fpga->FpgaBlockRead(m_rx.number, 0x12);
}

//-------------------------------------------------------------------

u32 pe_chn_rx::block_err_number(u32 chan)
{
    m_fpga->FpgaBlockWrite(m_rx.number, 0xA, (chan & 0x7));
    return m_fpga->FpgaBlockRead(m_rx.number, 0x13);
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

void pe_chn_rx::reset()
{
    m_fpga->resetTrd(m_rx.number);
    m_fpga->resetFifo(m_rx.number);
}

//-------------------------------------------------------------------
