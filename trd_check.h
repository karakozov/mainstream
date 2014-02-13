#ifndef TRD_CHECK_H
#define TRD_CHECK_H

#ifndef _UTYPES_LINUX_H_
    #include "utypes_linux.h"
#endif
#ifndef __FPGA_H__
    #include "fpga.h"
#endif

class Fpga;

//-------------------------------------------------------------------

#define REPORT_WORD_COUNT     16
#define REPORT_WORD_SIZE      16
#define REPORT_WORD_FIELDS    11

//-------------------------------------------------------------------

typedef union _report_word_t {

    struct {
      U16 read_d0;
      U16 read_d1;
      U16 read_d2;
      U16 read_d3;
      U16 expect_d0;
      U16 expect_d1;
      U16 expect_d2;
      U16 expect_d3;
      U16 index;
      U16 block_d0;
      U16 block_d1;
    } w16;

    struct {
      U64 read;
      U64 expect;
      U16 index;
      U32 block;
    } w64;

    U16 fields[REPORT_WORD_FIELDS];

} report_word_t;

//-------------------------------------------------------------------

class trd_check
{
public:
    trd_check(Fpga *fpga);
    virtual ~trd_check();

    u32 rd_block_number(u32 chan);
    u32 wr_block_number(u32 chan);
    u32 ok_block_number(u32 chan);
    u32 err_block_number(u32 chan);

    bool show_report(u32 chan, u32 chx_data);
    bool read_report_word(u32 chan, u32 chx_data, u32 word_num, report_word_t& word);
    void start_check(bool start);

private:
    Fpga            *m_fpga;
    fpga_trd_t       m_check;
    std::vector<report_word_t> m_report;
};

#endif // TRD_CHECK_H
