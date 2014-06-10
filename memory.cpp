
#include "memory.h"
#include "fpga.h"

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

#include <vector>
#include <string>
#include <iostream>

using namespace std;

//-----------------------------------------------------------------------------

Memory::Memory(Fpga *fpga) : m_fpga(fpga)
{
    m_MemTetrNum = 0;
    m_dwPhysMemSize = 0;

    memset(&m_DDR3, 0, sizeof m_DDR3);

    m_DDR3.Mode = 1;
    bool found = m_fpga->trd_number(SDRAMFMC106P_TETR_ID, m_MemTetrNum);
    if(!found)
    {
        found = m_fpga->trd_number(SDRAMDDR3X_TETR_ID, m_MemTetrNum);
        if(found) {
            m_DDR3.Mode = 3;
        } else {
            m_DDR3.Mode = 0;
        }
    }

    if(GetCfgFromSpd()) {
        CheckCfg();
        MemInit(1);
        Info();
    }
}

//-----------------------------------------------------------------------------

Memory::~Memory()
{
}

//-----------------------------------------------------------------------------

u8 Memory::ReadSpdByte(U32 OffsetSPD, U32 CtrlSPD)
{
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_SPDADDR, OffsetSPD);
    IPC_delay(10);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_SPDCTRL, CtrlSPD);
    IPC_delay(10);
    return (u8)m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_SPDDATAL);
}

//-----------------------------------------------------------------------------

bool Memory::GetCfgFromSpd()
{
    SDRAM_SPDCTRL SpdCtrl;
    SpdCtrl.AsWhole = 0;
    SpdCtrl.ByBits.Read = 1;

    unsigned tmpNum = 0;
    bool found = m_fpga->trd_number(SDRAMDDR3X_TETR_ID, tmpNum);

    UCHAR mem_type[SDRAM_MAXSLOTS];
    SpdCtrl.ByBits.Slot = 0;
    mem_type[0] = ReadSpdByte(SDRAMspd_MEMTYPE, SpdCtrl.AsWhole);
    if(!found)
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
    m_DDR3.CapacityMbits = U64(1 << capacity[0]) * 256 * 1024 * 1024;

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

    //fprintf(stderr, "m_dwPhysMemSize = 0x%x [32-bit words]\n", m_dwPhysMemSize);

    return true;
}

//-----------------------------------------------------------------------------

void Memory::MemInit(U32 init)
{
    SDRAM_CFG ddrCfg;

    ddrCfg.AsWhole = 0;
    ddrCfg.ByBits.NumSlots = m_DDR3.ModuleCnt ? (m_DDR3.ModuleCnt - 1) : 0;
    ddrCfg.ByBits.RowAddr = m_DDR3.RowAddrBits - 8;
    ddrCfg.ByBits.ColAddr = m_DDR3.ColAddrBits - 8;
    ddrCfg.ByBits.BankModule = m_DDR3.ModuleBanks - 1;
    ddrCfg.ByBits.BankChip = m_DDR3.ChipBanks >> 3;
    ddrCfg.ByBits.DeviceWidth = 0;
    ddrCfg.ByBits.Registered = 0;
    ddrCfg.ByBits.InitMemCmd = init;

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_CFG, ddrCfg.AsWhole);

    fprintf(stderr, "MODE1 = 0x%x\n", (U16)ddrCfg.AsWhole);
    fprintf(stderr, "Waiting for Memory Initialization  ... \n");
    IPC_delay( 200 );

    U32 status_mem = 0;
    U32 loop = 0;
    do
    {
        status_mem = m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0);
        fprintf( stderr, "%.8d: STATUS 0x%.4X\r", loop, status_mem );
        IPC_delay( 100 );
        loop++;
        if(loop > 20) {
            fprintf(stderr, "\nMemory Initialization ERROR\n");
            return;
        }
    } while( !(status_mem & 0x800));

    fprintf(stderr, "\nMemory Initialization DONE\n");
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

void Memory::Info(bool more)
{
    fprintf(stderr, "DDR3 SIZE: 0x%x [%d MB]\n", m_dwPhysMemSize, (m_dwPhysMemSize * 4) / (1024 * 1024));

    if(more) {

        U32 mode1 = m_fpga->FpgaRegPeekInd(m_MemTetrNum, 0x9);
        U32 status = (m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0) & (0x1 << 11));

        fprintf(stderr, "DDR3 STATUS: 0x%x\n", status ? 1 : 0);
        fprintf(stderr, "SLOT ----- 0x%x\n", mode1 & 0x3);
        fprintf(stderr, "RAS: ----- 0x%x\n", (mode1 >> 2) & 0x7);
        fprintf(stderr, "CAS: ----- 0x%x\n", (mode1 >> 5) & 0x7);
        fprintf(stderr, "MD_BANK: - 0x%x\n", (mode1 >> 8) & 0x1);
        fprintf(stderr, "SD_BANK: - 0x%x\n", (mode1 >> 9) & 0x1);
    }
}

//-----------------------------------------------------------------------------

void Memory::SetTarget(U32 trd, U32 target)
{
    U32 mode1  = m_fpga->FpgaRegPeekInd(trd, 0x9);
    mode1 |= (target << 0);
    m_fpga->FpgaRegPokeInd(trd, 0x9, mode1);
}

//-----------------------------------------------------------------------------

void Memory::SetFifoMode(U32 mode)
{
    U32 mode2 = m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_MODE);
    mode2 |= (mode << 4);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_MODE, mode2);

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_ENDADDRL, 0);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_ENDADDRH, 0);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_STADDRL, 0);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_STADDRH, 0);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_RDADDRL, 0);
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_RDADDRH, 0);
}

//-----------------------------------------------------------------------------

void Memory::SetReadMode(U32 mode)
{
    SDRAM_MODE mode10;

    mode10.AsWhole = m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_MODE);
    mode10.ByBits.ReadMode = mode;
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_MODE, mode10.AsWhole);
}

//-----------------------------------------------------------------------------

U32 Memory::GetEndAddr()
{
    U32 end_addr = (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_ENDADDRL) & 0xffff);
    end_addr |= (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_ENDADDRH) << 16);
    return end_addr;
}

//-----------------------------------------------------------------------------

U32 Memory::GetTrigCnt()
{
    U32 trig_cnt = (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_TRIGCNTL) & 0xffff);
    trig_cnt |= (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_TRIGCNTH) << 16);
    return trig_cnt;
}

//-----------------------------------------------------------------------------

void Memory::SetPostTrig(U32& trig_cnt)
{
    U32 start_addr = GetStartAddr();
    U32 end_addr = GetEndAddr();
    U32 active_size = end_addr - start_addr + 1;
    U32 max_trig_cnt = active_size - 16;
    if(trig_cnt > max_trig_cnt)
        trig_cnt = max_trig_cnt;
    trig_cnt = (trig_cnt >> 1) << 1; // align on 2 words

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_TRIGCNTL, (trig_cnt & 0xffff));
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_TRIGCNTH, (trig_cnt >> 16));
}

//-----------------------------------------------------------------------------

void Memory::SetStartAddr(U32 start_addr)
{
    U32 active_size = GetEndAddr() - start_addr + 1;

    if(start_addr + active_size > m_dwPhysMemSize)
        start_addr = m_dwPhysMemSize - active_size;

    start_addr = (start_addr >> 8) << 8; // align on 1024 bytes (256 32-bit words)

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_STADDRL, (start_addr & 0xffff));
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_STADDRH, (start_addr >> 16));

    // trig_cnt
    U32 tirg_cnt = GetTrigCnt();
    if(tirg_cnt > active_size - 16)
    {
        tirg_cnt = active_size - 16;
        SetPostTrig(tirg_cnt);
    }
}

//-----------------------------------------------------------------------------

U32 Memory::GetStartAddr()
{
    U32 start_addr = (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_STADDRL) & 0xffff);
    start_addr |= (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_STADDRH) << 16);
    return start_addr;
}

//-----------------------------------------------------------------------------

void Memory::SetMemSize(U32& active_size)
{
    U32 start_addr = GetStartAddr();

    if(active_size > 0xffffff00)
        active_size = 0xffffff00;
    if(active_size & 0xff)
        active_size = ((active_size >> 8) + 1) << 8;
    if(start_addr + active_size > m_dwPhysMemSize)
        active_size = m_dwPhysMemSize - start_addr;
    active_size = (active_size >> 8) << 8; // align on 1024 bytes (256 32-bit words)

    U32 end_addr = start_addr + active_size - 1;

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_ENDADDRL, (end_addr & 0xffff));
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_ENDADDRH, (end_addr >> 16));

    fprintf(stderr, "start_addr = 0x%x\n", start_addr);
    fprintf(stderr, "end_addr = 0x%x\n", end_addr);
    fprintf(stderr, "active_size = 0x%x\n", active_size);

    // trig_cnt
    U32 tirg_cnt = GetTrigCnt();
    if(tirg_cnt > active_size - 16)
    {
        tirg_cnt = active_size - 16;
        SetPostTrig(tirg_cnt);
    }
}

//-----------------------------------------------------------------------------

void Memory::SetReadAddr(U32 read_addr)
{
    if(read_addr > m_dwPhysMemSize)
        read_addr = m_dwPhysMemSize;
    //if(2 == m_MemTetrModif) // ADS10x2G
    //        read_addr = (read_addr >> 11) << 11; // align on 8192 bytes (1024 64-bit words)
    //else
    read_addr = (read_addr >> 8) << 8; // align on 1024 bytes (256 32-bit words)

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_RDADDRL, (read_addr & 0xffff));
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_RDADDRH, (read_addr >> 16));
}

//-----------------------------------------------------------------------------

U32 Memory::GetReadAddr()
{
    U32 read_addr = (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_RDADDRL) & 0xffff);
    read_addr |= (m_fpga->FpgaRegPeekInd(m_MemTetrNum, SDRAMnr_RDADDRH) << 16);
    return read_addr;
}


//-----------------------------------------------------------------------------

bool Memory::AcqComplete()
{
    SDRAM_STATUS status;
    status.AsWhole = m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0x0);
    return status.ByBits.AcqComplete;
}

//-----------------------------------------------------------------------------

void Memory::FlagClear()
{
    m_fpga->FpgaRegPokeInd(m_MemTetrNum, SDRAMnr_FLAGCLR, 0x2000);
}

//-----------------------------------------------------------------------------

void Memory::Enable(bool enable)
{
    U32 mode0 = m_fpga->FpgaRegPeekInd(m_MemTetrNum, 0);

    if(enable) {
        mode0 |= (1 << 5);
    } else {
        mode0 &= ~(1 << 5);
    }

    m_fpga->FpgaRegPokeInd(m_MemTetrNum, 0, mode0);
}

//-----------------------------------------------------------------------------

bool Memory::PassMemEnd()
{
    SDRAM_STATUS status;
    status.AsWhole = m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0x0);
    return status.ByBits.PassMem;
}

//-----------------------------------------------------------------------------

// Board Status Direct Register
typedef union _ADM2IF_STATUS {
    U32 AsWhole; // Board Status Register as a Whole Word
    struct { // Status Register as Bit Pattern
        U32   CmdRdy          : 1, // Ready to do command
            FifoRdy         : 1, // Ready FIFO
            Empty           : 1, // Empty FIFO
            AlmostEmpty     : 1, // Almost Empty FIFO
            HalfFull        : 1, // Half Full FIFO
            AlmostFull      : 1, // Almost Full FIFO
            Full            : 1, // Full FIFO
            Overflow        : 1, // Overflow FIFO
            Underflow       : 1, // Underflow FIFO
            Reserved        : 7; // Reserved
    } ByBits;
} ADM2IF_STATUS, *PADM2IF_STATUS;

//-----------------------------------------------------------------------------
/*
U32 Memory::GetData(void *Buffer, U32 BufferSize)
{
    U32 HalfFifoSize = (m_fpga->FpgaRegPeekInd(m_MemTetrNum, 0x104) << 2);
    U32 num = BufferSize / HalfFifoSize;
    U32 tail_bytes = BufferSize % HalfFifoSize;
    U8* buf = (U8*)Buffer;
    U32 total = 0;

    ADM2IF_STATUS status;
    for(U32 i = 0; i < num; i++)
    {
        // кручусь пока флаг HalfFull=1, то есть пока пол-FIFO не заполнено
        // когда флаг HalfFull=0, то есть пол-FIFO заполнено, то читаю
        do
        {
            status.AsWhole = m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0);

        } while(status.ByBits.HalfFull);

        m_fpga->FpgaReadRegBufDir(m_MemTetrNum,0x1,buf,HalfFifoSize);

        buf += HalfFifoSize;
        total += HalfFifoSize;
    }
    if(tail_bytes)
    {
        do
        {
            status.AsWhole = m_fpga->FpgaRegPeekDir(m_MemTetrNum, 0);

        } while(status.ByBits.HalfFull);

        m_fpga->FpgaReadRegBufDir(m_MemTetrNum,0x1,buf,tail_bytes);

        total += tail_bytes;
    }

    return total;
}
*/
//-----------------------------------------------------------------------------

bool Memory::setMemory(U32 mem_mode, U32 PretrigMode, U32& PostTrigSize, U32& Buf_size)
{
    // будем осуществлять сбор данных в память
    SetTarget(4, 2);

    if(mem_mode == 2)
    {	// память используется как FIFO
        SetFifoMode(1);
    }
    else
    {
        // память используется как память
        SetFifoMode(0);

        // можно применять только автоматический режим
        SetReadMode(0);

        // установить адрес записи
        SetStartAddr(0);

        // установить адрес чтения
        SetReadAddr(0);

        // получить размер активной зоны в 32-разрядных словах
        U32 mem_size = (Buf_size >> 2);

        SetMemSize(mem_size);

        Buf_size = (mem_size << 2); // получить фактический размер активной зоны в байтах
        fprintf(stderr, "SDRAM buffer size: %d bytes\n", Buf_size);

        if(PretrigMode == 3)
        {
            U32 post_size = (PostTrigSize >> 2);

            SetPostTrig(post_size);

            PostTrigSize = (post_size << 2);

            fprintf(stderr, "Post-trigger size: %d bytes\n", PostTrigSize);
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

#define TRDIND_MODE0            0x0
#define TRDIND_MODE1            0x9
#define TRDIND_MODE2            0xA
#define TRDIND_SPD_CTRL         0x204
#define TRDIND_SPD_ADDR         0x205
#define TRDIND_SPD_DATA         0x206

int SdramFullSpeed = 1;
int SdramFifoOutRestart = 1;
int SdramSource = 1;
int SdramTestSequence = 0x100;
int SdramFifoMode = 1;
int SdramAzBase = 0x0;
int CntBlockInBuffer = 4;
int SizeBlockOfWords = 0x1000;

//-----------------------------------------------------------------------------

void Memory::PrepareDDR3()
{
    U32 ii;
    U32		spd_addr[]={ 4, 5, 7, 8 };
    U32		spd_data[sizeof(spd_addr)/sizeof(U32)];

    U32		DDR2_trdNo = m_MemTetrNum;

    m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0, 0x3);	// MODE0
    m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0, 0x0);	// MODE0

    for(ii=0;ii<sizeof(spd_addr)/sizeof(U32);ii++)
    {
        m_fpga->FpgaRegPokeInd(DDR2_trdNo, TRDIND_SPD_ADDR, spd_addr[ii]);
        m_fpga->FpgaRegPokeInd(DDR2_trdNo, TRDIND_SPD_CTRL, 2);

        spd_data[ii] = m_fpga->FpgaRegPeekInd(DDR2_trdNo, TRDIND_SPD_DATA );
        fprintf( stderr, "SPD[%d] = 0x%.2X \n", ii, spd_data[ii] );
    }

    //----------------------------------------------------------------------------

    {
        U32 mode1=0;
        U32 columns;
        U32 rows;
        U32 banks;
        U32 ranks;
        U32 size;
        U32 primary_bus;
        U32 sdram_width = 0;

        switch( (spd_data[0]>>4) & 0x7 )
        {
        case 0: banks = 3; break;
        case 1: banks = 4; break;
        case 2: banks = 5; break;
        case 3: banks = 6; break;
        default: banks=0; break;
        }

        switch(spd_data[0] & 0xF )
        {
        case 0:	size=256; break;
        case 1:	size=512; break;
        case 2:	size=1024; break;
        case 3:	size=2048; break;
        case 4:	size=4096; break;
        case 5:	size=8192; break;
        case 6:	size=16384; break;
        default: size=0;

        }

        switch( (spd_data[1]>>3) & 0x7 )
        {
        case 0: rows=12; break;
        case 1: rows=13; break;
        case 2: rows=14; break;
        case 3: rows=15; break;
        case 4: rows=16; break;
        default: rows=0; break;
        }

        switch( (spd_data[1] & 0x7 ) )
        {
        case 0: columns=9; break;
        case 1: columns=10; break;
        case 2: columns=11; break;
        case 3: columns=12; break;
        default: columns=0; break;
        }


        switch( (spd_data[2]>>3) & 0x7 )
        {
        case 0: ranks=1; break;
        case 1: ranks=2; break;
        case 2: ranks=3; break;
        case 3: ranks=4; break;
        default: ranks=0; break;
        }

        switch( (spd_data[2]) & 0x7 )
        {
        case 0: sdram_width=4; break;
        case 1: sdram_width=8; break;
        case 2: sdram_width=16; break;
        case 3: sdram_width=32; break;
        default: ranks=0; break;
        }
        switch( (spd_data[3]) & 0x7 )
        {
        case 0: primary_bus=8; break;
        case 1: primary_bus=16; break;
        case 2: primary_bus=32; break;
        case 3: primary_bus=64; break;
        default: primary_bus=0; break;
        }

        mode1=((rows-8)<<2)&0x1C;
        mode1|=	((columns-8)<<5) & 0xE0;
        mode1|= ((ranks-1)<<8) ;
        mode1|= (1<<9);
        mode1|=0x2000;

        m_fpga->FpgaRegPokeInd(DDR2_trdNo, TRDIND_MODE1, mode1);	// CONF_REG

        U32 total_size=size/8*primary_bus/sdram_width*ranks;
        fprintf(stderr, "\nCOLUMNS \t\t%d\nROWS    \t\t%d\nBANKS   \t\t%d\nRANKS   \t\t%d\nSDRAM SIZE   \t\t%d \nSDRAM WITH \t\t%d\nPRIMARY BUS \t\t%d\nTOTAL SIZE \t\t%d [MB]\n",
                columns, rows, banks, ranks, size, sdram_width, primary_bus, total_size );

        fprintf(stderr, "\nMODE1 :\t\t0x%X\n\n", mode1);
    }

    //----------------------------------------------------------------------------

    {
        U32 mode3 = 0;
        if( SdramSource )
            mode3 |= 1;
        if( SdramFullSpeed )
            mode3 |= 0x20;
        if( SdramFifoOutRestart )
            mode3 |= 0x100;

        m_fpga->FpgaRegPokeInd( DDR2_trdNo, 0x0B, mode3 );    //
    }

    m_fpga->FpgaRegPokeInd( DDR2_trdNo, 0x0C, SdramTestSequence  );   // 0x100 -

    //-----------------------------------------------------------------------------

    fprintf(stderr, "Waiting for Memory Initialization  ... \r\n");
    IPC_delay( 200 );
    {
        U32 status;
        U32 loop=0;
        do
        {
            status = m_fpga->FpgaRegPeekDir(DDR2_trdNo, 0);
            fprintf( stderr, "%10d STATUS: %.4X \r", loop, status ); IPC_delay( 300 ); loop++;
        } while( !(status & 0x800));
    }
    fprintf(stderr, "\r\nMemory Initialization DONE\r\n");

    {
        if( 1==SdramFifoMode )
        { //  FIFO
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x10, 0 );
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x11, 0 );

            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0E, 0 );
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0F, 0 );

            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x14, 0 );
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x15, 0 );

            //pBrd->RegPokeInd(DDR2_trdNo, TRDIND_MODE2, 0x10);
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, TRDIND_MODE2, 0x14 ); //  FIFO

            fprintf(stderr, "\r\n FIFO\r\n");
        }
        else
        {

            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x10, SdramAzBase & 0xFFFF );
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x11, SdramAzBase >> 16 );

            U32 size = CntBlockInBuffer * SizeBlockOfWords; //     32-
            U32 AzEnd = SdramAzBase + size - 1;

            fprintf(stderr, "\r\n   \r\n" );
            fprintf(stderr, "\r\n  : 0x%.8X\r\n", SdramAzBase  );
            fprintf(stderr, "\r\n   : 0x%.8X\r\n", AzEnd  );
            fprintf(stderr, "\r\n  : 0x%.8X [MB]\r\n\n", size/(256*1024)  );

            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0E, AzEnd & 0xFFFF );
            m_fpga->FpgaRegPokeInd(DDR2_trdNo, 0x0F, AzEnd >> 16 );
        }

        m_fpga->FpgaRegPokeInd( DDR2_trdNo, TRDIND_MODE0, 2 );  //  FIFO

        if( SdramFifoOutRestart )
        {
            U32 mode3=0;
            mode3 = m_fpga->FpgaRegPeekInd( DDR2_trdNo, 0x0B );
            m_fpga->FpgaRegPokeInd( DDR2_trdNo, 0x0B, mode3 | 0x200 );    //   FIFO
            m_fpga->FpgaRegPokeInd( DDR2_trdNo, 0x0B, mode3 );
        }

        m_fpga->FpgaRegPokeInd( DDR2_trdNo, TRDIND_MODE0, 0 );
    }
}

//-----------------------------------------------------------------------------

