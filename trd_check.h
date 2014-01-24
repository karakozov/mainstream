#ifndef TRD_CHECK_H
#define TRD_CHECK_H

#ifndef _UTYPES_LINUX_H_
    #include "utypes_linux.h"
#endif
#ifndef __FPGA_H__
    #include "fpga.h"
#endif

class Fpga;

typedef union _report_word_t {

    struct {
      u16 read_d0;
      u16 read_d1;
      u16 read_d2;
      u16 read_d3;
      u16 expect_d0;
      u16 expect_d1;
      u16 expect_d2;
      u16 expect_d3;
      u16 index;
      u16 block_d0;
      u16 block_d1;
    } w16;

    struct {
      u64 read;
      u64 expect;
      u16 index;
      u32 block;
    } w64;

} report_word_t;

class trd_check
{
public:
    trd_check(Fpga *fpga);
    virtual ~trd_check();

    u32 rd_block_number(u32 chan);
    u32 wr_block_number(u32 chan);
    u32 ok_block_number(u32 chan);
    u32 err_block_number(u32 chan);

    void show_report(u32 chan);
    void start_check(bool start);

private:
    Fpga            *m_fpga;
    fpga_trd_t       m_check;
    std::vector<report_word_t> m_report;
};

#endif // TRD_CHECK_H
