
#include "mapper.h"

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

Mapper::Mapper()
{
    m_MappedList.clear();
    openMemDevice();
}

//-----------------------------------------------------------------------------

Mapper::~Mapper()
{
    unmapMemory();
    closeMemDevice();
}

//-----------------------------------------------------------------------------

int Mapper::openMemDevice()
{
    char name[256];
    m_devMem = IPC_openDevice(name, "mem", -1);
    if(!m_devMem) {
        fprintf(stderr, "%s(): Error in IPC_openDevice()\n", __FUNCTION__);
        throw;
    }

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
        fprintf(stderr, "%s(): Error in IPC_mapPhysAddr()\n", __FUNCTION__);
        throw;
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
        fprintf(stderr, "%s(): Error in IPC_mapPhysAddr()\n", __FUNCTION__);
        throw;
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
