#include "fpga_base.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif

#include <fcntl.h>
#include <signal.h>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

fpga_base::fpga_base(u32 fpgaNumber) : m_fpgaNumber(fpgaNumber)
{
    if(!openFpga()) {
        throw exception_info("%s, %d: %s() - Error open FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    fprintf(stderr, "====================== Open FPGA%d ======================\n", m_fpgaNumber);

#ifdef __linux__
    m_map = new Mapper();
#else
    m_map = new Mapper(m_fpga);
#endif

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

bool fpga_base::openFpga()
{
    char name[256];

    m_fpga = IPC_openDevice(name, AmbDeviceName, m_fpgaNumber);
    if(!m_fpga) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void fpga_base::closeFpga()
{
    fprintf(stderr, "Close FPGA%d\n", m_fpgaNumber);
    if(m_map) delete m_map;
    IPC_closeDevice(m_fpga);
}

//-----------------------------------------------------------------------------

void fpga_base::infoFpga()
{
    AMB_CONFIGURATION fpgaInfo;

    memset(&fpgaInfo, 0, sizeof(fpgaInfo));

    int res = IPC_ioctlDevice( m_fpga, IOCTL_AMB_GET_CONFIGURATION, &fpgaInfo, sizeof(AMB_CONFIGURATION), &fpgaInfo, sizeof(AMB_CONFIGURATION));
    if(res < 0) {
        throw exception_info("%s, %d: %s() - Error get FPGA%d information.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    m_info = fpgaInfo;
}

//-----------------------------------------------------------------------------

bool fpga_base::info(AMB_CONFIGURATION& info)
{
    info = m_info;
    return m_ok;
}

//-----------------------------------------------------------------------------

void fpga_base::mapFpga()
{
    int res_count = 0;
    int map_count = 0;

    for(unsigned i=0; i<sizeof(m_info.PhysAddress)/sizeof(m_info.PhysAddress[0]); i++) {

        if(m_info.PhysAddress[i] && m_info.Size[i]) {

            ++res_count;
#ifdef __linux__
            m_info.VirtAddress[i] = m_map->mapPhysicalAddress(m_info.PhysAddress[i], m_info.Size[i]);
#endif
            if(m_info.VirtAddress[i]) {

                fprintf(stderr, "BAR%d: 0x%x -> %p\n", i, m_info.PhysAddress[i], m_info.VirtAddress[i]);
                ++map_count;
            }
        }
    }

    if(map_count != res_count) {
        fprintf(stderr, "Not all resources was mapped\n");
        throw exception_info("%s, %d: %s() - Error map all BARs for FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }

    m_ok = true;
}

//-----------------------------------------------------------------------------

void fpga_base::initBar()
{
    m_bar0 = m_bar1 = m_bar2 = 0;

    if(m_info.VirtAddress[0]) {
        m_bar0 = (u32*)m_info.VirtAddress[0];
        m_bar[0] = m_bar0;
    }
    if(m_info.VirtAddress[1]) {
        m_bar1 = (u32*)m_info.VirtAddress[1];
        m_bar[1] = m_bar1;
    }
    if(m_info.VirtAddress[2]) {
        m_bar2 = (u32*)m_info.VirtAddress[2];
        m_bar[2] = m_bar2;
    }
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_reg_peek_dir( u32 trd, u32 reg )
{
    if( (trd>15) || (reg>3) )
        return -1;

    u32 offset = trd*0x4000 + reg*0x1000;

    return m_bar1[offset/4];
}

//-----------------------------------------------------------------------------

void fpga_base::core_reg_poke_dir( u32 trd, u32 reg, u32 val )
{
    if( (trd>15) || (reg>3) )
        return;

    u32 offset = trd*0x4000 + reg*0x1000;

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

        //fprintf(stderr, "status = 0x%x\r", status);

        if( ii>100 )
            IPC_delay( 1 );
        if( ii>200 ) {
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

        //fprintf(stderr, "status = 0x%x\r", status);

        if( ii>100 )
            IPC_delay( 1 );
        if( ii>200 ) {
            return;
        }
    }

    m_bar1[CmdData/4] = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_bar_read( u32 bar, u32 offset )
{
    u32 *base = m_bar[bar];
    return base[2*offset];
}

//-----------------------------------------------------------------------------

void fpga_base::core_bar_write( u32 bar, u32 offset, u32 val )
{
    u32 *base = m_bar[bar];
    base[2*offset] = val;
}

//-----------------------------------------------------------------------------

void fpga_base::core_block_write( u32 nb, u32 reg, u32 val )
{
    if( (nb>7) || (reg>31) )
        return;

    *(m_bar0 + nb*64 + reg*2) = val;
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_block_read( u32 nb, u32 reg )
{
    if( (nb>7) || (reg>31) )
        return -1;

    u32 ret = *(m_bar0 + nb*64 + reg*2);

    if( reg<8 )
        ret &= 0xFFFF;

    return ret;
}

//-----------------------------------------------------------------------------
/*
void fpga_base::core_block_write( u32 nb, u32 reg, u32 val )
{
    if( (nb>7) || (reg>31) )
        return;

    core_bar_write(0, nb*32 + reg, val);
}

//-----------------------------------------------------------------------------

u32  fpga_base::core_block_read( u32 nb, u32 reg )
{
    if( (nb>7) || (reg>31) )
        return -1;

    u32 ret = core_bar_read(0, nb*32 + reg);
    if( reg<8 )
        ret &= 0xFFFF;

    return ret;
}
*/
//-----------------------------------------------------------------------------

u32 fpga_base::core_write_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                0,
                0);
    if(res < 0){
        throw exception_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_write_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_WRITE_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                NULL,
                0);
    if(res < 0) {
        throw exception_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_read_reg_buf(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_BUF,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw exception_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------

u32 fpga_base::core_read_reg_buf_dir(u32 TetrNum, u32 RegNum, void* RegBuf, u32 RegBufSize)
{
    AMB_BUF_REG reg_buf = { 0, TetrNum, RegNum, RegBuf, RegBufSize };

    int res = IPC_ioctlDevice(
                m_fpga,
                IOCTL_AMB_READ_REG_BUF_DIR,
                &reg_buf,
                sizeof(AMB_BUF_REG),
                &reg_buf,
                sizeof(AMB_BUF_REG));
    if(res < 0) {
        throw exception_info("%s, %d: %s() - Error ioctl FPGA%d.\n", __FILE__, __LINE__, __FUNCTION__, m_fpgaNumber);
    }
    return 0;
}

//-----------------------------------------------------------------------------
