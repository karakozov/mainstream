#ifndef PCIE_TEST_H
#define PCIE_TEST_H

#include "acdsp.h"
#include "acsync.h"

#include <vector>

//-----------------------------------------------------------------------------

#define ADC_TRD             4
#define MEM_TRD             5
#define STREAM_BLK_SIZE     0x10000
#define STREAM_BLK_NUM      4

//-----------------------------------------------------------------------------

#define ADC_SAMPLE_FREQ         (500000000.0)
#define ADC_CHANNEL_MASK        (0xF)
#define ADC_START_MODE          (0x3 << 4)
#define ADC_MAX_CHAN            (0x4)

//-----------------------------------------------------------------------------
#define USE_SIGNAL 0
//-----------------------------------------------------------------------------

bool start_pcie_test(std::vector<acdsp*>& boards, struct app_params_t& params);
unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync, u32 boradMask);
unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber, unsigned from = 0);
void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync);
void delete_fpga_list(std::vector<Fpga*>& fpgaList);

#endif // PCIE_TEST_H
