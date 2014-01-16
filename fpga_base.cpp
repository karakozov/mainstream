#include "fpga_base.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

//-----------------------------------------------------------------------------

fpga_base::fpga_base(u32 fpgaNumber) : m_fpgaNumber(fpgaNumber)
{
    openFpga();
    infoFpga();
    mapFpga();
    initBar();
}

//-----------------------------------------------------------------------------

fpga_base::~fpga_base()
{
    closeFpga();
}

//-----------------------------------------------------------------------------

void fpga_base::openFpga()
{
    char name[256];
    m_fpga = IPC_openDevice(name, AmbDeviceName, m_fpgaNumber);
    if(!m_fpga) {
        fprintf(stderr, "Error open FPGA%d\n", m_fpgaNumber);
        throw;
    }
    fprintf(stderr, "Open FPGA%d\n", m_fpgaNumber);
}

//-----------------------------------------------------------------------------

void fpga_base::closeFpga()
{
    fprintf(stderr, "Close FPGA%d\n", m_fpgaNumber);
    IPC_closeDevice(m_fpga);
}

//-----------------------------------------------------------------------------

void fpga_base::infoFpga()
{
    AMB_CONFIGURATION fpgaInfo;

    memset(&fpgaInfo, 0, sizeof(fpgaInfo));

    int res = IPC_ioctlDevice( m_fpga, IOCTL_AMB_GET_CONFIGURATION, &fpgaInfo, sizeof(AMB_CONFIGURATION), &fpgaInfo, sizeof(AMB_CONFIGURATION));
    if(res < 0) {
        fprintf(stderr, "Error get FPGA%d information\n", m_fpgaNumber);
        throw;
    }

    m_info = fpgaInfo;
}

//-----------------------------------------------------------------------------

void fpga_base::mapFpga()
{
    int res_count = 0;
    int map_count = 0;

    for(unsigned i=0; i<sizeof(m_info.PhysAddress)/sizeof(m_info.PhysAddress[0]); i++) {

        if(m_info.PhysAddress[i] && m_info.Size[i]) {

            ++res_count;

            m_info.VirtAddress[i] = m_map.mapPhysicalAddress(m_info.PhysAddress[i], m_info.Size[i]);

            if(m_info.VirtAddress[i]) {

                fprintf(stderr, "BAR%d: 0x%x -> %p\n", i, m_info.PhysAddress[i], m_info.VirtAddress[i]);
                ++map_count;
            }
        }
    }

    if(map_count != res_count) {
        fprintf(stderr, "Not all resources was mapped\n");
        throw;
    }
}

//-----------------------------------------------------------------------------

void fpga_base::initBar()
{
    m_bar0 = m_bar1 = m_bar2 = 0;

    if(m_info.VirtAddress[0]) {
        m_bar0 = (u32*)m_info.VirtAddress[0];
    }
    if(m_info.VirtAddress[1]) {
        m_bar1 = (u32*)m_info.VirtAddress[1];
    }
    if(m_info.VirtAddress[2]) {
        m_bar2 = (u32*)m_info.VirtAddress[2];
    }
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_reg_peek_dir( u32 trd, u32 reg )
{
    if( (trd>15) || (reg>3) )
        return -1;

    u32 offset = trd*0x4000 + reg*0x1000;
    u32 ret = *(m_bar1 + offset/4);

    return ret;
}

//-----------------------------------------------------------------------------

void fpga_base::core_reg_poke_dir( u32 trd, u32 reg, u32 val )
{
    if( (trd>15) || (reg>3) )
        return;

    u32 offset = trd*0x4000+reg*0x1000;

    m_bar1[offset/4]=val;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_reg_peek_ind( u32 trd, u32 reg )
{
    if( (trd>15) || (reg>0x3FF) )
        return -1;

    u32 status;
    u32 Address = trd*0x4000;
    u32 Status  = Address;
    u32 CmdAdr  = Address + 0x2000;
    u32 CmdData = Address + 0x3000;
    u32 ret;

    m_bar1[CmdAdr/4] = reg;

    for( int ii=0; ; ii++ ) {

        status = m_bar1[Status/4];
        if( status & 1 )
            break;

        fprintf(stderr, "status = 0x%x\r", status);

        if( ii>1000 )
            IPC_delay( 1 );
        if( ii>2000 ) {
            return 0xFFFF;
        }
    }

    ret = m_bar1[CmdData/4];
    ret &= 0xFFFF;

    return ret;
}

//-----------------------------------------------------------------------------

void fpga_base::core_reg_poke_ind( u32 trd, u32 reg, u32 val )
{
    if( (trd>15) || (reg>0x3FF) )
        return;

    u32 status;
    u32 Address = trd*0x4000;
    u32 Status  = Address;
    u32 CmdAdr  = Address + 0x2000;
    u32 CmdData = Address + 0x3000;

    m_bar1[CmdAdr/4] = reg;

    for( int ii=0; ; ii++ ) {

        status = m_bar1[Status/4];
        if( status & 1 )
            break;

        fprintf(stderr, "status = 0x%x\r", status);

        if( ii>1000 )
            IPC_delay( 1 );
        if( ii>2000 ) {
            return;
        }
    }

    m_bar1[CmdData/4] = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_bar0_read( u32 offset )
{
    return m_bar0[2*offset];
}

//-----------------------------------------------------------------------------

void fpga_base::core_bar0_write( u32 offset, u32 val )
{
    m_bar0[2*offset] = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_bar1_read( u32 offset )
{
    return m_bar1[2*offset];
}

//-----------------------------------------------------------------------------

void fpga_base::core_bar1_write( u32 offset, u32 val )
{
    m_bar1[2*offset] = val;
}

//-----------------------------------------------------------------------------

void fpga_base::core_block_write( u32 nb, u32 reg, u32 val )
{
    if( (nb>7) || (reg>31) )
        return;

    *(m_bar0+nb*64+reg*2)=val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_block_read( u32 nb, u32 reg )
{
    if( (nb>7) || (reg>31) )
        return -1;

    u32 ret = 0;

    ret=*(m_bar0+nb*64+reg*2);
    if( reg<8 )
        ret&=0xFFFF;

    return ret;
}

//-----------------------------------------------------------------------------
