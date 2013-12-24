/*
 ****************** File SdramRegs.h *************************
 *
 *  Definitions of tetrad register
 *	structures and constants
 *	for SDRAM
 *
 * (C) InSys by Dorokhin Andrey Jun, 2004
 *
 ************************************************************
*/

#ifndef _SDRAMREGS_H_
#define _SDRAMREGS_H_

// Memory configuration register (+9)
typedef union _SDRAM_CFG {
  ULONG AsWhole; // Memory configuration Register as a Whole Word
  struct { // Memory configuration Register as Bit Pattern
   ULONG NumSlots	: 2, // Number of Busy Slots
	     RowAddr	: 3, // Number of Row Addresses
		 ColAddr	: 3, // Number of Column Addresses
		 BankModule : 1, // Number of Physical Banks on DIMM
		 BankChip	: 1, // Number of Device Banks
		 DeviceWidth : 1, // Primary SDRAM Device Width (1 - x4)
		 Registered	: 1, // Registered Module
		 Reserved1	: 1, // Reserved
		 InitMemCmd	: 1, // Init memory command
		 Reserved2	: 2; // Reserved
  } ByBits;
} SDRAM_CFG, *PSDRAM_CFG;

// Memory Status Direct Register
typedef union _SDRAM_STATUS {
	ULONG AsWhole; // Memory Status Register as a Whole Word
	struct { // Memory Status Register as Bit Pattern
		ULONG	CmdRdy		: 1, // (+0) Ready to do command
				FifoRdy		: 1, // (+1) Ready FIFO 
				Empty		: 1, // (+2) Empty FIFO
				AlmostEmpty	: 1, // (+3) Almost Empty FIFO
				HalfFull	: 1, // (+4) Half Full FIFO
				AlmostFull	: 1, // (+5) Almost Full FIFO
				Full		: 1, // (+6) Full FIFO
				Overflow	: 1, // (+7) Overflow FIFO
				Underflow	: 1, // (+8) Underflow FIFO
				Reserved1	: 2, // (+9) Reserved
				InitMem		: 1, // (+11) Initialization memory
				PassMem		: 1, // (+12) Passed end of memory
				AcqComplete	: 1, // (+13) Acquisition complete
				Reserved	: 7; // Reserved
	} ByBits;
} SDRAM_STATUS, *PSDRAM_STATUS;

typedef enum _SrdamStatusRegBits {
  SDRAM_statINITMEM		= 1 << 11,
  SDRAM_statPASSMEM		= 1 << 12,
  SDRAM_statACQCOMPLETE	= 1 << 13
} SrdamStatusRegBits;

// Memory access mode register (+10)
typedef union _SDRAM_MODE {
  ULONG AsWhole; // Memory access mode Register as a Whole Word
  struct { // Memory access mode Register as Bit Pattern
   ULONG ReadMode	: 1,  // Read Memory Mode : 0 - auto, 1 - random
	     ReadEnbl	: 1,  // Random read memory enable
	     Reserved0	: 2,  // Reserved
	     FifoMode	: 1,  // FIFO mode enable
		 Reserved	: 10; // Reserved
  } ByBits;
} SDRAM_MODE, *PSDRAM_MODE;

// Memory mode3 register (+11)
typedef union _SDRAM_MODE3 {
  ULONG AsWhole; // Mode3 Register as a Whole Word
  struct { // Mode3 Register as Bit Pattern
   ULONG SelIn		: 4,  // 0 - from ADC, 1 - from FDSP, 2 - from HOST
	     MaskEnbl	: 1,  // 
	     FullSpeed	: 1,  // 
	     Reserved0	: 2,  // Reserved
	     MaslId		: 4,  // 
		 SelOut		: 4; //  0 - to HOST, 1 - to DAC
  } ByBits;
} SDRAM_MODE3, *PSDRAM_MODE3;

// Start Address for DAQ & read in auto mode (16-17)
typedef union _SDRAM_STARTADDR {
  DWORD AsWhole;
  struct {
    USHORT	LoWord,
			HiWord;
  } ByWord;
} SDRAM_STARTADDR, *PSDRAM_STARTADDR;

// End Address for DAQ & read in auto mode (14-15)
typedef union _SDRAM_ENDADDR {
  DWORD AsWhole;
  struct {
    USHORT	LoWord,
			HiWord;
  } ByWord;
} SDRAM_ENDADDR, *PSDRAM_ENDADDR;

// Start Address for read (20-21)
typedef union _SDRAM_READADDR {
  DWORD AsWhole;
  struct {
    USHORT	LoWord,
			HiWord;
  } ByWord;
} SDRAM_READADDR, *PSDRAM_READADDR;

// Post Trigger Counter (18-19)
typedef union _SDRAM_TRIGCNT {
  DWORD AsWhole;
  struct {
    USHORT	LoWord,
			HiWord;
  } ByWord;
} SDRAM_TRIGCNT, *PSDRAM_TRIGCNT;

// SPD (Serial Presence-Detect) control register (direct - 0x204)
typedef union _SDRAM_SPDCTRL {
  ULONG AsWhole; // SPD control Register as a Whole Word
  struct { // SPD control Register as Bit Pattern
   ULONG Write	: 1,  // Read Memory Mode : 0 - auto, 1 - random
	     Read	: 1,  // Random read memory enable
		 PgMode	: 1,  // 1 - page mode flag
		 Res	: 5,  // Reserved
		 Slot	: 3,  // 000 - slot 0, 001 - slot 1
		 Res1	: 5; // Reserved
  } ByBits;
} SDRAM_SPDCTRL, *PSDRAM_SPDCTRL;

#define SDRAM_MAXSLOTS 2
#define SDRAM_SPDSIZE 128

// Fundamental memory type (into SPD, byte offset = 2)
typedef enum _SDRAM_MEMTYPE {
	SDRAMmt_FPM =	0x01,
	SDRAMmt_EDO =	0x02,
	SDRAMmt_SDR =	0x04,
	SDRAMmt_DDR =	0x07,
	SDRAMmt_DDR2 =	0x08,
    SDRAMmt_DDR3 =	0x0B
} SDRAM_MEMTYPE;

// SPD structure
typedef struct _SDRAM_SPDCONTENT {
	UCHAR	UseSpdBytes;	// Number of bytes used - 128 by Micron
	UCHAR	TotalSpdBytes;	// Total number of SPD memory bytes (08 - 256 bytes)
	UCHAR	MemType;		// Memory Type: 01 - Fast Page Mode, 02 - EDO, 04 - SDRAM, 07 - DDR SDRAM, 08 - DDR2 SDRAM
	UCHAR	RowAddr;		// Number of row addresses (11-14)
	UCHAR	ColAddr;		// Number of column addresses (8-13)
	UCHAR	NumBanks;		// Number of banks
	UCHAR	LoDataWidth;	// Module data width
	UCHAR	HiDataWidth;	// Module data width (continue)
	UCHAR	Voltage;		// Module voltage interface levels
	UCHAR	CycleTime;		// SDRAM cycle time
	UCHAR	AccesClk;		// SDRAM acces from clock
	UCHAR	CfgType;		// DIMM configuration type (non-parity, ECC)
	UCHAR	Refresh;		// Refresh Rate/Type
	UCHAR	PrimaryWidth;	// Primary DDR SDRAM Width
	UCHAR	ErrorCheckWidth;// Error Checking DDR SDRAM Width
	UCHAR	MinClkDelay;	// Minimum Clock Delay Back to Back Random Column Address
	UCHAR	BurstLength;	// Burst Lengths Supported
	UCHAR	NumBanksChip;	// Number of banks in each DDR SDRAM device
	UCHAR	CasLatency;		// CAS# Latencies Supported
	UCHAR	Reserve1[109];
} SDRAM_SPDCONTENT, *PSDRAM_SPDCONTENT;

// Numbers of Command Registers
typedef enum _SDRAM_NUM_SPD_BYTES {
	SDRAMspd_MEMTYPE	= 2,	// Memory Type: 01 - Fast Page Mode, 02 - EDO, 04 - SDRAM, 07 - DDR SDRAM, 08 - DDR2 SDRAM
	SDRAMspd_ROWADDR	= 3,	// Number of row addresses (11-14)
	SDRAMspd_COLADDR	= 4,	// Number of column addresses (8-13)
	SDRAMspd_MODBANKS	= 5,	// Number of banks on module
	SDRAMspd_WIDTH		= 13,	// Primary DDR SDRAM Width
	SDRAMspd_CHIPBANKS	= 17,	// Number of banks in each DDR SDRAM device
	SDRAMspd_CASLAT		= 18,	// CAS# Latencies Supported
    SDRAMspd_ATTRIB		= 21	// Various module attributes
} SDRAM_NUM_SPD_BYTES;

// Numbers of Command Registers
typedef enum _DDR3_NUM_SPD_BYTES {
	DDR3spd_MEMTYPE		= 2,	// DRAM Device Type: 0B - DDR3 SDRAM
	DDR3spd_CHIPBANKS	= 4,	// SDRAM Density and Banks: Bits 6~4 - Bank Address, Bits 3~0 - Total SDRAM capacity, in megabits
	DDR3spd_ROWCOLADDR	= 5,	// SDRAM Addressing: Bits 5~3 - Row Address (12-16), Bits 2~0 - Column Address (9-12)
	DDR3spd_MODBANKS	= 7,	// Module Organization: Bits 5~3 - Number of Ranks, Bits 2~0 - SDRAM Device width
	DDR3spd_WIDTH		= 8,	// Module Memory Bus Width: Bits 2~0 - Primary bus width
    DDR3spd_CASLAT		= 16	// Minimum CAS Latency Time
} DDR3_NUM_SPD_BYTES;

// Numbers of Command Registers
typedef enum _SDRAM_NUM_CMD_REGS {
	SDRAMnr_CFG			= 9,
	SDRAMnr_MODE		= 10, // 0x0A
	SDRAMnr_MODE3		= 11, // 0x0B
	SDRAMnr_ENDADDRL	= 14, // 0x0E
	SDRAMnr_ENDADDRH	= 15, // 0x0F
	SDRAMnr_STADDRL		= 16, // 0x10
	SDRAMnr_STADDRH		= 17, // 0x11
	SDRAMnr_TRIGCNTL	= 18, // 0x12
	SDRAMnr_TRIGCNTH	= 19, // 0x13
	SDRAMnr_RDADDRL		= 20, // 0x14
    SDRAMnr_RDADDRH		= 21 // 0x15
} SDRAM_NUM_CMD_REGS;

// Numbers of Constant Registers
typedef enum _SDRAM_NUM_DIRECT_REGS {
	SDRAMnr_FLAGCLR		= 0x200,
	SDRAMnr_ACQCNTL		= 0x202,
	SDRAMnr_ACQCNTH		= 0x203,
	SDRAMnr_SPDCTRL		= 0x204,
	SDRAMnr_SPDADDR		= 0x205,
	SDRAMnr_SPDDATAL	= 0x206,
	SDRAMnr_SPDDATAH	= 0x207,
	SDRAMnr_PRTEVENTLO	= 0x20A,	// start event  by pretrigger mode: word number into buffer (low part)
    SDRAMnr_PRTEVENTHI	= 0x20B	// start event  by pretrigger mode: word number into buffer (high part)
//	SDRAMnr_SYNCEVENTL	= 0x204,
//	SDRAMnr_SYNCEVENTH	= 0x205,
} SDRAM_NUM_DIRECT_REGS;

#endif //_SDRAMREGS_H_

//
// End of file
//
