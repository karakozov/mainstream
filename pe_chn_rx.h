#ifndef PE_CHN_RX_H
#define PE_CHN_RX_H

#ifndef _UTYPES_LINUX_H_
    #include "utypes_linux.h"
#endif
#ifndef __FPGA_H__
    #include "fpga.h"
#endif

class Fpga;

class pe_chn_rx
{
public:
    pe_chn_rx(Fpga *fpga);
    virtual ~pe_chn_rx();

    void set_fpga_addr(u32 chan, u32 tx_addr, u32 tx_sign);

    u32 rx_block_number(u32 chan);
    u32 sign_err_number(u32 chan);
    u32 block_err_number(u32 chan);

    void start_rx(bool start);

private:
    Fpga            *m_fpga;
    fpga_block_t    m_rx;

    void reset();
};

#endif // PE_CHN_RX_H
