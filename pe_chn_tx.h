#ifndef PE_CHN_TX_H
#define PE_CHN_TX_H

#ifndef _UTYPES_LINUX_H_
    #include "utypes_linux.h"
#endif
#ifndef __FPGA_H__
    #include "fpga.h"
#endif

class Fpga;

class pe_chn_tx
{
public:
    pe_chn_tx(Fpga *fpga);
    virtual ~pe_chn_tx();

    void set_fpga_addr(int devnum, u32 dst_fpga_addr);
    void set_fpga_sign(u32 sign);
    void set_fpga_chan(u32 chan);
    void set_fpga_test(u32 mode);
    u32 tx_block_number();

    void start_tx(bool start);

private:
    Fpga            *m_fpga;
    fpga_block_t    m_tx;
};

#endif // PE_CHN_TX_H
