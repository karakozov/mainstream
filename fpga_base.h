#ifndef __FPGA_BASE_H__
#define __FPGA_BASE_H__

#ifndef __GIPCY_H__
    #include "gipcy.h"
#endif
#ifndef _UTYPES_H_
    #include "utypes.h"
#endif
#ifndef _UTYPES_LINUX_H_
    #include "utypes_linux.h"
#endif
#ifndef _DDWAMBPEX_H_
    #include "ddwambpex.h"
#endif
#ifndef __MAPPER_H__
    #include "mapper.h"
#endif

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

typedef  struct _fpga_block_t {

    u16 number;
    u16 id;
    u16 ver;
    u16 device_id;
    u16 rev_mode;
    u16 pld_ver;
    u16 block_cnt;
    u16 core_id_mod;

} fpga_block_t;

//-----------------------------------------------------------------------------

typedef  struct _fpga_trd_t {

    u16 number;
    u16 id;

} fpga_trd_t;

//-----------------------------------------------------------------------------

class fpga_base
{
public:
    explicit fpga_base(u32 fpgaNumber);
    virtual ~fpga_base();

    bool info(AMB_CONFIGURATION& info);

protected:
    u32 core_reg_peek_dir( u32 trd, u32 reg );
    u32 core_reg_peek_ind( u32 trd, u32 reg );
    void core_reg_poke_dir( u32 trd, u32 reg, u32 val );
    void core_reg_poke_ind( u32 trd, u32 reg, u32 val );

    u32  core_bar_read( u32 bar, u32 offset );
    void core_bar_write( u32 bar, u32 offset, u32 val );
    void core_block_write( u32 nb, u32 reg, u32 val );
    u32  core_block_read( u32 nb, u32 reg );

    u32 core_write_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize);
    u32 core_write_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize);
    u32 core_read_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize);
    u32 core_read_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize);

    IPC_handle              m_fpga;

private:
    Mapper                  m_map;
    AMB_CONFIGURATION       m_info;
    u32*                    m_bar0;
    u32*                    m_bar1;
    u32*                    m_bar2;
    u32*                    m_bar[3];
    u32                     m_fpgaNumber;
    bool                    m_ok;

    fpga_base();
    void openFpga();
    void closeFpga();
    void infoFpga();
    void mapFpga();
    void initBar();
};

#endif // FPGA_BASE_H
