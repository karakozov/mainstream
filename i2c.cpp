// ***************************************************************
//  Si571.cpp   version:  1.0   ·  date: 12/05/2012
//  -------------------------------------------------------------
//  Вспомогательные функции для управления генератором Si570/Si571
//  -------------------------------------------------------------
//  Copyright (C) 2012 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <errno.h>
#endif
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

#include "i2c.h"

//-----------------------------------------------------------------------------

i2c::i2c()
{
    m_deviceFile = -1;
    m_force = 1;

    char deviceName[256];
#ifdef __linux__
    if( open_i2c_dev(1, deviceName, sizeof(deviceName)) < 0) {
        throw;
    }
#endif
}

//-----------------------------------------------------------------------------

i2c::i2c(int bus)
{
    m_deviceFile = -1;
    m_force = 1;

    char deviceName[256];
#ifdef __linux__
    if( open_i2c_dev(bus, deviceName, sizeof(deviceName)) < 0) {
        throw;
    }
#endif
}

//-----------------------------------------------------------------------------

i2c::~i2c()
{
#ifdef __linux__
    close(m_deviceFile);
#endif
}

//-----------------------------------------------------------------------------

int i2c::open_i2c_dev(int i2cbus, char *filename, size_t size)
{
    m_deviceFile = -1;
#ifdef __linux__
    snprintf(filename, size, "/dev/i2c/%d", i2cbus);
    filename[size - 1] = '\0';
    m_deviceFile = open(filename, O_RDWR);

    if (m_deviceFile < 0 && (errno == ENOENT || errno == ENOTDIR)) {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        m_deviceFile = open(filename, O_RDWR);
    }

    if (m_deviceFile < 0) {
        if (errno == ENOENT) {
            fprintf(stderr, "Error: Could not open file "
                "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                i2cbus, i2cbus, strerror(ENOENT));
        } else {
            fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
            if (errno == EACCES)
                fprintf(stderr, "Run as root?\n");
        }
    }
#endif
    return m_deviceFile;
}

//-----------------------------------------------------------------------------

int i2c::set_slave_addr(int address)
{
    /* With force, let the user read from/write to the registers
       even when a driver is also running */
#ifdef __linux__
    if (ioctl(m_deviceFile, m_force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
        fprintf(stderr,
            "Error: Could not set address to 0x%02x: %s\n",
            address, strerror(errno));
        return -errno;
    }
#endif

    return 0;
}

//-----------------------------------------------------------------------------

int i2c::write(int slaveAddress, int reg, uint8_t value)
{
    if( set_slave_addr(slaveAddress) < 0 ) {
        throw;
    }
#ifdef __linux__
    return i2c_smbus_write_byte_data(m_deviceFile, reg, value);
#else
    return 0;
#endif
}

//-----------------------------------------------------------------------------

int i2c::write(int slaveAddress, uint8_t value)
{
    if( set_slave_addr(slaveAddress) < 0 ) {
        throw;
    }
#ifdef __linux__
    return i2c_smbus_write_byte(m_deviceFile, value);
#else
    return 0;
#endif
}

//-----------------------------------------------------------------------------

int i2c::read(int slaveAddress, int reg)
{
    if( set_slave_addr(slaveAddress) < 0 ) {
        throw;
    }
#ifdef __linux__
    if(reg >= 0) {
        if (i2c_smbus_write_byte(m_deviceFile, reg) < 0) {
            fprintf(stderr, "i2c_smbus_write_byte() failed\n");
        }
    }
    int val = i2c_smbus_read_byte(m_deviceFile);

    //fprintf(stderr, "0x%02x\n", val);

    return val;
#else
    return 0;
#endif
}

//-----------------------------------------------------------------------------
