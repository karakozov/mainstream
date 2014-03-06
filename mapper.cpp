
#include "mapper.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

using namespace std;

//-----------------------------------------------------------------------------

Mapper::Mapper(IPC_handle handle)
{
    m_MappedList.clear();
    if(handle) {
        extHandle = true;
        m_devMem = handle;
    } else {
        extHandle = false;
        openMemDevice();
    }
}

//-----------------------------------------------------------------------------

Mapper::~Mapper()
{
    unmapMemory();
	if(!extHandle)
		closeMemDevice();
}

//-----------------------------------------------------------------------------

int Mapper::openMemDevice()
{
#ifdef __linux__
    m_devMem = IPC_openDeviceRaw("/dev/mem");
    if(!m_devMem) {
        throw except_info("%s, %d: %s() - Error open /dev/mem.\n", __FILE__, __LINE__, __FUNCTION__);
    }
#endif
    return 0;
}

//-----------------------------------------------------------------------------

void Mapper::closeMemDevice()
{
    IPC_closeDevice(m_devMem);
}

//-----------------------------------------------------------------------------

void* Mapper::mapPhysicalAddress(void* physicalAddress, uint32_t areaSize)
{
    struct map_addr_t map = {0, (size_t)physicalAddress, areaSize};

    int res = IPC_mapPhysAddr(m_devMem, &map.virtualAddress, map.physicalAddress, map.areaSize);
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error in IPC_mapPhysAddr().\n", __FILE__, __LINE__, __FUNCTION__);
    }

    m_MappedList.push_back(map);

    return map.virtualAddress;
}

//-----------------------------------------------------------------------------

void* Mapper::mapPhysicalAddress(size_t physicalAddress, uint32_t areaSize)
{
    struct map_addr_t map = {0, physicalAddress, areaSize};

    int res = IPC_mapPhysAddr(m_devMem, &map.virtualAddress, map.physicalAddress, map.areaSize);
    if(res < 0) {
        throw except_info("%s, %d: %s() - Error in IPC_mapPhysAddr().\n", __FILE__, __LINE__, __FUNCTION__);
    }

    m_MappedList.push_back(map);

    return map.virtualAddress;
}

//-----------------------------------------------------------------------------

void Mapper::unmapMemory()
{
    for(unsigned i=0; i<m_MappedList.size(); i++) {
        struct map_addr_t map = m_MappedList.at(i);
        if(map.virtualAddress) {
            IPC_unmapPhysAddr(m_devMem, map.virtualAddress, map.areaSize);
            map.virtualAddress = 0;
            map.physicalAddress = 0;
            map.areaSize = 0;
        }
    }
    m_MappedList.clear();
}

//-----------------------------------------------------------------------------
