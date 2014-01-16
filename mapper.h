
#ifndef __MAPPER_H__
#define __MAPPER_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#include <stdint.h>
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
    explicit Mapper();
    virtual ~Mapper();

    void* mapPhysicalAddress(void* physicalAddress, uint32_t areaSize);
    void* mapPhysicalAddress(size_t physicalAddress, uint32_t areaSize);

private:
    IPC_handle      m_devMem;

    std::vector<struct map_addr_t> m_MappedList;

    int openMemDevice();
    void closeMemDevice();
    void unmapMemory();
};

//-----------------------------------------------------------------------------

#endif //__MAPPER_H__
