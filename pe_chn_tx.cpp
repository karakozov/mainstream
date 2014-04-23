#include "pe_chn_tx.h"

#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------

pe_chn_tx::pe_chn_tx(Fpga *fpga) : m_fpga(fpga)
{
    if(!m_fpga->fpgaBlock(0, 0x1C, m_tx)) {
        throw except_info("%s, %d: %s() - Not found PE_CHN_TX! ID: 0x%x\n", __FILE__, __LINE__, __FUNCTION__, 0x1C);
    }
    reset();
}

//-------------------------------------------------------------------

pe_chn_tx::~pe_chn_tx()
{
    start_tx(false);
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_addr(u32 chan, u32 tx_addr, u32 tx_sign, u32 flag)
{
    // set TX signature and channel number
    m_fpga->FpgaBlockWrite(m_tx.number, 0x9, tx_sign);

    // set TX address
    if(flag == 3) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0xC + chan, (tx_addr|0x3));
    }

    if(flag == 1) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0xC + chan, 0x1);
    }

    if(flag == 0) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0xC + chan, 0x0);
    }
}

//-------------------------------------------------------------------

void pe_chn_tx::set_fpga_wait(u32 wait)
{
    m_fpga->FpgaBlockWrite(m_tx.number, 0xB, wait);
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

void pe_chn_tx::start_tx(bool start, unsigned adcMask)
{
    u32 ctrl = m_fpga->FpgaBlockRead(m_tx.number, 0x8);

    if(start) {

        ctrl |= 0x10; //TX_CHN_ON

        // start ADC tetrade
        m_fpga->FpgaRegPokeInd(4, 0x10, adcMask);
        m_fpga->FpgaRegPokeInd(4, 0x17, (0x3 << 4));
        m_fpga->FpgaRegPokeInd(4, 0, 0x2038);

        // start TX channel
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, (ctrl | (0x3 << 5)));

    } else {

        //ctrl &= ~0x10; //TX_CHN_OFF

        // stop TX channel
        m_fpga->FpgaBlockWrite(m_tx.number, 0x8, (ctrl & (~(0x3 << 5))));

        // stop ADC tetrade
        m_fpga->FpgaRegPokeInd(4, 0x10, 0x0);
        m_fpga->FpgaRegPokeInd(4, 0x17, 0x0);
        m_fpga->FpgaRegPokeInd(4, 0, 0x0);
    }
}

//-------------------------------------------------------------------

void pe_chn_tx::reset()
{
    m_fpga->resetTrd(m_tx.number);
    m_fpga->resetFifo(m_tx.number);
    for(unsigned i=0; i<8; i++) {
        m_fpga->FpgaBlockWrite(m_tx.number, 0xC+i, 0);
    }
}

//-------------------------------------------------------------------
