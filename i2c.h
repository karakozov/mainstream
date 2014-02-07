
#ifndef __I2C_H__
#define __I2C_H__

//-----------------------------------------------------------------------------

#ifdef __linux__
#include "i2c-dev.h"
#endif

#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

class i2c {

public:
    i2c();
    i2c(int bus);
    virtual ~i2c();

    int write(int slaveAddress, int reg, uint8_t value);
    int write(int slaveAddress, uint8_t value);
    int read(int slaveAddress, int reg = -1);

private:
    int  m_deviceFile;
    int  m_force;

    int open_i2c_dev(int i2cbus, char *filename, size_t size);
    int set_slave_addr(int address);
};

#endif //__I2C_H__
