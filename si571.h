
#ifndef __SI571_H__
#define __SI571_H_

//-----------------------------------------------------------------------------

#include "i2c.h"

#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------

#define SPDdev_GEN 0x55

//-----------------------------------------------------------------------------

class Si571 {

public:
    Si571(i2c *iic = 0);
    virtual ~Si571();

    int	SetRate(float *pRate);
    int	GetRate(float *pRate);

private:
    i2c* m_iic;
    float m_dGenFxtal;
    float m_dGenRefMax;
    int m_nGenRef;
    uint32_t regData[20];

    int	Si57x_SetRate( float *pRate, float dGenFxtal, uint32_t *paRegData );
    int	Si57x_GetRate( float *pRate, float dGenFxtal, uint32_t *paRegData );
    int	Si57x_CalcDiv( float dRate, float dFXtal, uint32_t *pRfreqLo, uint32_t *pRfreqHi, uint32_t *pHsdivCode, uint32_t *pN1Code );
    int	Si57x_CalcFxtal( float *pGenFxtal, float dGenRef, uint32_t *paRegData );
    int Si57x_GetFXTAL(void);

    int SpdWrite(int addr, int reg, uint32_t val);
    int SpdRead(int addr, int reg, uint32_t* val);
};

#endif //_SI571_H
