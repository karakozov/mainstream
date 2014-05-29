
#ifndef __MAPPER_H__
#define __MAPPER_H__

#include "gipcy.h"
#include "utypes.h"

#include "exceptinfo.h"

#ifdef __linux__
#include <stdint.h>
#endif
#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

struct map_addr_t {
    void *virtualAddress;
    size_t physicalAddress;
    size_t areaSize;
};

//-----------------------------------------------------------------------------

class Mapper {

public:
    explicit Mapper(IPC_handle handle = 0);
    virtual ~Mapper();

    void* mapPhysicalAddress(void* physicalAddress, uint32_t areaSize);
    void* mapPhysicalAddress(size_t physicalAddress, uint32_t areaSize);

private:
    IPC_handle      m_devMem;
	bool			extHandle;

    std::vector<struct map_addr_t> m_MappedList;

    int openMemDevice();
    void closeMemDevice();
    void unmapMemory();
};

//-----------------------------------------------------------------------------

#endif //__MAPPER_H__
