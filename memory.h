
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "gipcy.h"
#include "utypes.h"
#include "sdramregs.h"

//-----------------------------------------------------------------------------

const int SDRAMFMC106P_TETR_ID = 0x8F;  // tetrad id SDRAM of FMC106P
const int SDRAMDDR3X_TETR_ID = 0x9B;    // tetrad id SDRAM of FMC106P - autor DSMV

//-----------------------------------------------------------------------------

typedef struct _DDR3SDRAMSRV_CFG {
    U8      Mode;			// флаги возможностей ввода из памяти в хост (0x01)/ вывода в память из хоста (0x02)
    U8      Res;			// резерв
    U8      MemType;		// тип памяти
    U8      SlotCnt;		// количество установленных слотов
    U8      ModuleCnt;		// количество установленных DIMM-модулей (занятых слотов)
    U8      RowAddrBits;	// количество разрядов адреса строк : 12-16 (Byte 5 - SDRAM Addressing: Bits 5~3 - Row Address Bits)
    U8      ColAddrBits;	// количество разрядов адреса столбцов : 9-12 (Byte 5 - SDRAM Addressing: Bits 2~0 - Column Address Bits)
    U8      ModuleBanks;	// количество банков в модуле : 1-4 (Byte 7 - Module Organization: Bits 5~3 - Number of Ranks)
    U8      ChipBanks;		// количество банков в микросхемах : 8,16,32,64 (Byte 4 - SDRAM Density and Banks: Bits 6~4 - Bank Address Bits)
    __int64	CapacityMbits;	// объём модуля в мегабитах : 256,512,1024,2048,4096,8192,16384 (Byte 4 - SDRAM Density and Banks: Bits 3~0 - Total SDRAM capacity, in megabits)
    U8      ChipWidth;		// количество разрядов в микросхемах : 4,8,16,32 (Byte 7 - Module Organization: Bits 2~0 - SDRAM Device Width)
    U8      PrimWidth;		// количество разрядов в модуле : 8,16,32,64 (Byte 8 - Module Memory Bus Width: Bits 2~0 - Primary bus width, in bits)
} DDR3SDRAMSRV_CFG;

//-----------------------------------------------------------------------------

class Fpga;

//-----------------------------------------------------------------------------

class Memory {

public:
    explicit Memory(Fpga *fpga);
    virtual ~Memory();

    bool setMemory(U32 mem_mode, U32 PretrigMode, U32& PostTrigSize, U32& Buf_size);
    bool AcqComplete();
    void Enable(bool enable);
    //U32 GetData(void *Buffer, U32 BufferSize);
    u8 ReadSpdByte(U32 OffsetSPD, U32 CtrlSPD);
    bool GetCfgFromSpd();
    bool CheckCfg();
    void MemInit(U32 init);
    int SetSel(U32& sel);
    int GetSel(U32& sel);
    void Info(bool more = false);
    void SetTarget(U32 trd, U32 target);
    void SetFifoMode(U32 mode);
    void SetReadMode(U32 mode);
    void SetMemSize(U32& active_size);
    void SetPostTrig(U32& trig_cnt);
    void SetStartAddr(U32 start_addr);
    void SetReadAddr(U32 read_addr);
    U32 GetStartAddr();
    U32 GetTrigCnt();
    U32 GetEndAddr();
    U32 GetReadAddr();
    bool PassMemEnd();
    void FlagClear();
    void PrepareDDR3();

private:

    class Fpga*     m_fpga;
    U32             m_MemTetrNum;
    U32             m_dwPhysMemSize; // physical memory size on device in 32-bit words
    U32             m_mode;

    DDR3SDRAMSRV_CFG m_DDR3;
};

//-----------------------------------------------------------------------------

#endif //__MEMORY_H__
