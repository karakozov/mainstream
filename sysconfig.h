
#ifndef _SYSCONFIG_H_
#define _SYSCONFIG_H_

#include "fpga.h"
#include "acdsp.h"
#include "acsync.h"

#include <vector>

//-----------------------------------------------------------------------------

unsigned create_fpga_list(std::vector<Fpga*>& fpgaList, unsigned fpgaNumber, unsigned from);
void delete_fpga_list(std::vector<Fpga*>& fpgaList);
unsigned create_board_list(std::vector<Fpga*>& fpgaList, std::vector<acdsp*>& boardList, acsync** sync_board, u32 boradMask);
void delete_board_list(std::vector<acdsp*>& boardList, acsync* sync_board);

//-----------------------------------------------------------------------------

#endif //_SYSCONFIG_H_

