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

    void set_fpga_addr(int devnum, u32 src_fpga_addr);
    void set_fpga_sign(u32 sign);
    void set_fpga_chan(u32 chan);
    void set_fpga_test(u32 mode);
    u32 rx_block_number();
    u32 sign_err_number();
    u32 block_err_number();

    void start_rx(bool start);

private:
    Fpga            *m_fpga;
    fpga_block_t    m_rx;
};

#endif // PE_CHN_RX_H
