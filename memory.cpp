
#include "memory.h"
#include "fpga.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

using namespace std;

//-----------------------------------------------------------------------------

Memory::Memory(Fpga *fpga) : m_fpga(fpga)
{
    m_MainTetrNum = 0;
    m_MemTetrNum = 0;
    m_MemTetrModif = 0;
    m_dwPhysMemSize = 0;

    memset(&m_DDR3, 0, sizeof m_DDR3);

    m_DDR3.Mode = 0x01;
    m_MemTetrNum = m_fpga->trd_number(SDRAMFMC106P_TETR_ID);
    if(m_MemTetrNum == (U32)-1)
    {
        m_MemTetrNum = m_fpga->trd_number(SDRAMDDR3X_TETR_ID);
            m_DDR3.Mode = 3;
        if(m_MemTetrNum == (U32)-1)
            m_DDR3.Mode = 0;
    }

    CheckCfg();
    MemInit(1);
}

//-----------------------------------------------------------------------------

Memory::~Memory()
{
}

//-----------------------------------------------------------------------------

u8 Memory::ReadSpdByte(U32 OffsetSPD, U32 CtrlSPD)
{
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_SPDADDR, OffsetSPD);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_SPDCTRL, CtrlSPD);
    return (u8)m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_SPDDATAL);
}

//-----------------------------------------------------------------------------

bool Memory::GetCfgFromSpd()
{
    SDRAM_SPDCTRL SpdCtrl;
    SpdCtrl.AsWhole = 0;
    SpdCtrl.ByBits.Read = 1;

    int flg_3x = m_fpga->trd_number(SDRAMDDR3X_TETR_ID);

    UCHAR mem_type[SDRAM_MAXSLOTS];
    SpdCtrl.ByBits.Slot = 0;
    mem_type[0] = ReadSpdByte(SDRAMspd_MEMTYPE, SpdCtrl.AsWhole);
    if(flg_3x == -1)
    {
        SpdCtrl.ByBits.Slot = 1;
        mem_type[1] = ReadSpdByte(SDRAMspd_MEMTYPE, SpdCtrl.AsWhole);
    }
    else
    {
        UCHAR val = ReadSpdByte(DDR3spd_ROWCOLADDR, SpdCtrl.AsWhole);
        if(mem_type[0] == 0 && val != 0)
            mem_type[0] = SDRAMmt_DDR3;
        mem_type[1] = 0;
    }

    for(int i = 0; i < SDRAM_MAXSLOTS; i++)
        if(mem_type[i] == SDRAMmt_DDR3)
            m_DDR3.ModuleCnt++;
    if(!m_DDR3.ModuleCnt)
        return false;

    int Status = 0;
    UCHAR val;
    UCHAR row_addr[SDRAM_MAXSLOTS], col_addr[SDRAM_MAXSLOTS], module_banks[SDRAM_MAXSLOTS], module_width[SDRAM_MAXSLOTS],
          chip_width[SDRAM_MAXSLOTS], chip_banks[SDRAM_MAXSLOTS], capacity[SDRAM_MAXSLOTS];
    for(int i = 0; i < m_DDR3.ModuleCnt; i++)
    {
        SpdCtrl.ByBits.Slot = i;
        val = ReadSpdByte(DDR3spd_ROWCOLADDR, SpdCtrl.AsWhole);
        row_addr[i] = (val >> 3) & 0x7;
        col_addr[i] = val & 0x7;
        val = ReadSpdByte(DDR3spd_MODBANKS, SpdCtrl.AsWhole);
        module_banks[i] = (val >> 3) & 0x7;
        chip_width[i] = val & 0x7;
        val = ReadSpdByte(DDR3spd_CHIPBANKS, SpdCtrl.AsWhole);
        chip_banks[i] = (val >> 4) & 0x7;
        capacity[i] = val & 0xF;
        val = ReadSpdByte(DDR3spd_WIDTH, SpdCtrl.AsWhole);
        module_width[i] = val & 0x7;


        if(i && ((row_addr[i] != row_addr[i-1]) ||
                 (col_addr[i] != col_addr[i-1]) ||
                 (module_banks[i] != module_banks[i-1]) ||
                 (chip_banks[i] != chip_banks[i-1]) ||
                 (module_width[i] != module_width[i-1]) ||
                 (chip_width[i] != chip_width[i-1]) ||
                 (capacity[i] != capacity[i-1])
                ))
        {
            Status = -2;
        }
    }
    if(-2 == Status)
        m_DDR3.ModuleCnt--;

    m_DDR3.MemType = mem_type[0];
    m_DDR3.RowAddrBits = row_addr[0] + 12;
    m_DDR3.ColAddrBits = col_addr[0] + 9;
    m_DDR3.ModuleBanks = module_banks[0] + 1;
    m_DDR3.PrimWidth = 8 << module_width[0];
    m_DDR3.ChipBanks = 8 << chip_banks[0];
    m_DDR3.ChipWidth = 4 << chip_width[0];
    m_DDR3.CapacityMbits = __int64(1 << capacity[0]) * 256 * 1024 * 1024;

    return true;
}

//-----------------------------------------------------------------------------

bool Memory::CheckCfg()
{
    if(!m_DDR3.ModuleCnt)
        return false;
    if(m_DDR3.RowAddrBits < 12 || m_DDR3.RowAddrBits > 16)
        return false;
    if(m_DDR3.ColAddrBits < 9 || m_DDR3.ColAddrBits > 12)
        return false;
    if(m_DDR3.ModuleBanks < 1 || m_DDR3.ModuleBanks > 4)
        return false;
    if(m_DDR3.ChipBanks != 8 &&
        m_DDR3.ChipBanks != 16 &&
        m_DDR3.ChipBanks != 32 &&
        m_DDR3.ChipBanks != 64)
            return false;
    if(m_DDR3.ChipWidth != 4 &&
        m_DDR3.ChipWidth != 8 &&
        m_DDR3.ChipWidth != 16 &&
        m_DDR3.ChipWidth != 32)
            return false;
    if(m_DDR3.PrimWidth != 8 &&
        m_DDR3.PrimWidth != 16 &&
        m_DDR3.PrimWidth != 32 &&
        m_DDR3.PrimWidth != 64)
            return false;

    m_dwPhysMemSize =	(ULONG)(((m_DDR3.CapacityMbits >> 3) * (__int64)m_DDR3.PrimWidth / m_DDR3.ChipWidth * m_DDR3.ModuleBanks *
                        m_DDR3.ModuleCnt) >> 2); // in 32-bit Words

    fprintf(stderr, "m_dwPhysMemSize = 0x%x [32-bit words]\n", m_dwPhysMemSize);

    return true;
}

//-----------------------------------------------------------------------------

void Memory::MemInit(U32 init)
{
    SDRAM_CFG ddrCfg;

    ddrCfg.ByBits.NumSlots = m_DDR3.ModuleCnt ? (m_DDR3.ModuleCnt - 1) : 0;
    ddrCfg.ByBits.RowAddr = m_DDR3.RowAddrBits - 8;
    ddrCfg.ByBits.ColAddr = m_DDR3.ColAddrBits - 8;
    ddrCfg.ByBits.BankModule = m_DDR3.ModuleBanks - 1;
    ddrCfg.ByBits.BankChip = m_DDR3.ChipBanks >> 3;
    ddrCfg.ByBits.DeviceWidth = 0;
    ddrCfg.ByBits.Registered = 0;
    ddrCfg.ByBits.InitMemCmd = init;

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_CFG, ddrCfg.AsWhole);
}

//-----------------------------------------------------------------------------

int Memory::SetSel(U32& sel)
{
    SDRAM_MODE3 Mode3;

    Mode3.AsWhole = m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_MODE3);
    Mode3.ByBits.SelIn = sel & 0xFF;
    Mode3.ByBits.SelOut = (sel >> 8) & 0xFF;

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_MODE3, Mode3.AsWhole);

    return 0;
}

//-----------------------------------------------------------------------------

int Memory::GetSel(U32& sel)
{
    SDRAM_MODE3 Mode3;

    Mode3.AsWhole = m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_MODE3);
    sel = Mode3.ByBits.SelIn;
    sel += Mode3.ByBits.SelOut << 8;

    return 0;
}

//-----------------------------------------------------------------------------
