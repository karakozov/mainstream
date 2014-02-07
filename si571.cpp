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
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#include <errno.h>

#include <vector>
#include <string>
#include <iostream>

#include "gipcy.h"
#include "si571.h"
#include "utypes.h"

//-----------------------------------------------------------------------------

Si571::Si571(i2c* iic)
{
    m_iic = iic;
    m_nGenRef = 10000000.0;
    m_dGenRefMax = 1400000000.0;
    Si57x_GetFXTAL();
}

//-----------------------------------------------------------------------------

Si571::~Si571()
{
}

//-----------------------------------------------------------------------------

int	Si571::Si57x_SetRate( float *pRate, float dGenFxtal, uint32_t *paRegData )
{
    //
    // Функция формирует коды регистров, позволяющих сформировать
    // частоту входного сигнала
    //
    uint32_t	rfreqLo, rfreqHi, hsdivCode, n1Code;
    int			status = 0;

    //
    // Скорректировать частоту, если необходимо
    //
    if( *pRate < 10000000.0 )
    {
        *pRate = 10000000.0;
        status = 1;
    }
    if( (*pRate > 945000000.0) && (*pRate<970000000.0) )
    {
        *pRate = 945000000.0;
        status = 1;
    }
    if( (*pRate > 1134000000.0) && (*pRate<1212500000.0) )
    {
        *pRate = 1134000000.0;
        status = 1;
    }

    //
    // Вычислить коэффициенты
    //
    if( 0 > Si57x_CalcDiv( *pRate, dGenFxtal, &rfreqLo, &rfreqHi, &hsdivCode, &n1Code ) )
        return -1;

    paRegData[7]  = paRegData[13] = (hsdivCode << 5) | (n1Code >> 2);
    paRegData[8]  = paRegData[14] = (n1Code << 6) | (rfreqHi >> 4);
    paRegData[9]  = paRegData[15] = (rfreqHi << 4) | (rfreqLo >> 24);
    paRegData[10] = paRegData[16] = (rfreqLo >> 16) & 0xFF;
    paRegData[11] = paRegData[17] = (rfreqLo >> 8) & 0xFF;
    paRegData[12] = paRegData[18] = rfreqLo & 0xFF;

    return status;
}

//-----------------------------------------------------------------------------

int	Si571::Si57x_GetRate( float *pRate, float dGenFxtal, uint32_t *paRegData )
{
    //
    // Функция по кодам регистров вычисляет и возвращает
    // частоту выходного сигнала.
    //
    uint32_t	rfreqLo, rfreqHi, hsdivCode, n1Code;
    float		freqTmp;
    float		dRFreq, dHsdiv, dN1;

    //
    // Восстановить коэффициенты
    //
    rfreqLo  = 0xFF & paRegData[12];
    rfreqLo |= (0xFF & paRegData[11]) << 8;
    rfreqLo |= (0xFF & paRegData[10]) << 16;
    rfreqLo |= (0xF & paRegData[9]) << 24;
    //fprintf(stderr, "%s(): rfreqLo = 0x%x\n", __FUNCTION__, rfreqLo);
    rfreqHi  = (0xF0 & paRegData[9]) >> 4;
    rfreqHi |= (0x3F & paRegData[8]) << 4;
    //fprintf(stderr, "%s(): rfreqHi = 0x%x\n", __FUNCTION__, rfreqHi);
    hsdivCode    = (0xE0 & paRegData[7]) >> 5;
    //fprintf(stderr, "%s(): hsdivCode = 0x%x\n", __FUNCTION__, hsdivCode);
    n1Code   = (0xC0 & paRegData[8]) >> 6;
    n1Code  |= (0x1F & paRegData[7]) << 2;
    //fprintf(stderr, "%s(): n1Code = 0x%x\n", __FUNCTION__, n1Code);
    dRFreq   = (float)rfreqLo;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dRFreq  /= 1024.0 * 1024.0 * 256.0;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dRFreq  += (float)rfreqHi;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dHsdiv   = (float)( hsdivCode + 4 );
    //fprintf(stderr, "%s(): dHsdiv = %f\n", __FUNCTION__, dHsdiv);
    //dN1      = (n1Code==1) ? 1.0 : (float)( 0xFE & (n1Code+1));
    dN1      = (n1Code==0) ? 1.0 : (float)( 0xFE & (n1Code+1));
    //fprintf(stderr, "%s(): dN1 = %f\n", __FUNCTION__, dN1);

    //
    // Рассчитать частоту
    //
    freqTmp  = dGenFxtal;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);
    freqTmp /= dHsdiv * dN1;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);
    freqTmp *= dRFreq;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);
    *pRate   = freqTmp;

    return 0;
}

//-----------------------------------------------------------------------------

int	Si571::Si57x_CalcDiv( float dRate, float dFXtal, uint32_t *pRfreqLo, uint32_t *pRfreqHi, uint32_t *pHsdivCode, uint32_t *pN1Code )
{
    //
    // Вспомогательная функция.
    //
    float		dRFreq, dHsdiv, dN1;
    unsigned	ii;

    //
    // Вычислить коэффициенты HS_DIV, N1
    //
    dHsdiv = dN1 = 0.0;

    if( dRate>=1212500000.0 )
    {
        dHsdiv = 4;
        dN1 = 1;
    }
    else if( dRate>=970000000.0 )
    {
        dHsdiv = 5;
        dN1 = 1;
    }
    else
    {
        float		dDcoMin = 4850000000.0;
        float		dDcoMax = 5670000000.0;
        float		adHsdiv[] = { 4.0, 5.0, 6.0, 7.0, 9.0, 11.0 };
        float		dFreqDco, dFreqTmp;
        int32_t			n1;

        dFreqDco = dDcoMax;
        for( n1=1; n1<=128; n1++ )
        {
            if( (n1>1) && (n1&0x1) )
                continue;				// только четные n1
            for( ii=0; ii<sizeof(adHsdiv)/sizeof(adHsdiv[0]); ii++ )
            {
                dFreqTmp = dRate * adHsdiv[ii] * (float)n1;
                if( (dFreqTmp>=dDcoMin) && (dFreqTmp<=dFreqDco) )
                {
                    dFreqDco = dFreqTmp;
                    dHsdiv = adHsdiv[ii];
                    dN1 = (float)n1;
                }
            }
        }

    }
    if( (dHsdiv==0.0) || (dN1==0.0) )
        return -1;

    //
    // Вычислить коэффициент RFREQ
    //
    dRFreq  = dRate * dHsdiv * dN1;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dRFreq /= dFXtal;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);

    //
    // Преобразовать коэффициенты в коды
    //
    *pRfreqHi = (uint32_t)dRFreq;
    *pRfreqLo = (uint32_t)( (dRFreq - (float)(*pRfreqHi)) * 1024.0 * 1024.0 * 256.0 );
    *pHsdivCode = (uint32_t)(dHsdiv-4.0);
    *pN1Code = (uint32_t)(dN1-1.0);
    //fprintf( stderr, "\n" );
    //fprintf( stderr, "%s(): freq = %f Hz, xtal = %f Hz\n", __FUNCTION__, dRate, dFXtal );
    //fprintf( stderr, "%s(): N1 = %f, HSDIV = %f, RF = %f\n", __FUNCTION__, dN1, dHsdiv, dRFreq );

    return 0;
}

//-----------------------------------------------------------------------------

int	Si571::Si57x_CalcFxtal( float *pGenFxtal, float dGenRef, uint32_t *paRegData )
{
    int32_t		rfreqLo, rfreqHi, hsdivCode, n1Code;
    float		freqTmp;
    float		dRFreq, dHsdiv, dN1;

    rfreqLo  = 0xFF & paRegData[12];
    rfreqLo |= (0xFF & paRegData[11]) << 8;
    rfreqLo |= (0xFF & paRegData[10]) << 16;
    rfreqLo |= (0xF & paRegData[9]) << 24;
    //fprintf(stderr, "%s(): rfreqLo = 0x%x\n", __FUNCTION__, rfreqLo);

    rfreqHi  = (0xF0 & paRegData[9]) >> 4;
    rfreqHi |= (0x3F & paRegData[8]) << 4;
    //fprintf(stderr, "%s(): rfreqHi = 0x%x\n", __FUNCTION__, rfreqHi);

    hsdivCode    = (0xE0 & paRegData[7]) >> 5;
    //fprintf(stderr, "%s(): hsdivCode = 0x%x\n", __FUNCTION__, hsdivCode);

    n1Code   = (0xC0 & paRegData[8]) >> 6;
    n1Code  |= (0x1F & paRegData[7]) << 2;
    //fprintf(stderr, "%s(): n1Code = 0x%x\n", __FUNCTION__, n1Code);

    dRFreq   = (float)rfreqLo;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dRFreq  /= 1024.0 * 1024.0 * 256.0;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dRFreq  += (float)rfreqHi;
    //fprintf(stderr, "%s(): dRFreq = %f\n", __FUNCTION__, dRFreq);
    dHsdiv   = (float)( hsdivCode + 4 );
    //fprintf(stderr, "%s(): dHsdiv = %f\n", __FUNCTION__, dHsdiv);
    dN1      = (n1Code==1) ? 1.0 : (float)( 0xFE & (n1Code+1));
    //fprintf(stderr, "%s(): dN1 = %f\n", __FUNCTION__, dN1);

    freqTmp  = dGenRef;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);
    freqTmp /= dRFreq;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);
    freqTmp *= dHsdiv * dN1;
    //fprintf(stderr, "%s(): freqTmp = %f\n", __FUNCTION__, freqTmp);

    *pGenFxtal = freqTmp;

    return 0;
}

//-----------------------------------------------------------------------------

int	Si571::SetRate(float *pRate)
{
    int			regAdr;

    //
    // Скорректировать частоту, если необходимо
    //
    if( *pRate > m_dGenRefMax )
    {
        *pRate = m_dGenRefMax;
    }

    Si57x_SetRate( pRate, m_dGenFxtal, regData );

    //
    // Запрограммировать генератор
    //
    SpdWrite( SPDdev_GEN, 137, 0x10 );		// Freeze DCO
    for( regAdr=7; regAdr<=18; regAdr++ )
        SpdWrite( SPDdev_GEN, regAdr, regData[regAdr] );
    SpdWrite( SPDdev_GEN, 137, 0x0 );		// Unfreeze DCO
    SpdWrite( SPDdev_GEN, 135, 0x50 );		// Assert the NewFreq bit

    return 0;
}

//-----------------------------------------------------------------------------

int	Si571::GetRate(float *pRate)
{
    int			regAdr;

    *pRate = 0.0;

    //
    // Считать регистры Si570/Si571
    //
    for( regAdr=7; regAdr<=12; regAdr++ )
        SpdRead( SPDdev_GEN, regAdr, &regData[regAdr] );

    Si57x_GetRate( pRate, m_dGenFxtal, regData );
    fprintf(stderr, "Si571 regs 7-12: %x, %x, %x, %x, %x, %x\n", regData[7], regData[8], regData[9], regData[10], regData[11], regData[12] );

    return 0;
}

//-----------------------------------------------------------------------------

int Si571::Si57x_GetFXTAL(void)
{
    // Определение частоты кварца для генератора Si570/Si571
    //
    m_dGenFxtal = 0.0;
    uint32_t regAdr, regData[20];

    //
    // Подать питание на Si570/Si571
    //
    SpdWrite( SPDdev_GEN, 135, 0x80 );		// Reset
    IPC_delay(300);

    //
    // Считать регистры Si570/Si571
    //
    for( regAdr=7; regAdr<=12; regAdr++ )
        SpdRead( SPDdev_GEN, regAdr, &regData[regAdr] );

    //
    // Рассчитать частоту кварца
    //
    Si57x_CalcFxtal( &m_dGenFxtal, m_nGenRef, regData );

    fprintf(stderr, "Si571 regs 7-12: %x, %x, %x, %x, %x, %x\n", regData[7], regData[8], regData[9], regData[10], regData[11], regData[12] );
    fprintf(stderr, ">> XTAL = %f kHz\n", m_dGenFxtal/1000.0 );
    fprintf(stderr, ">> GREF = %f kHz\n", ((float)(m_nGenRef))/1000.0 );

    return 0;
}

//-----------------------------------------------------------------------------

int Si571::SpdWrite(int addr, int reg, uint32_t val)
{
    if(!m_iic) return -1;
    return m_iic->write(addr, reg, val);
}

//-----------------------------------------------------------------------------

int Si571::SpdRead(int addr, int reg, uint32_t* val)
{
    if(!m_iic) return -1;
    *val = m_iic->read(addr, reg);
    return 0;
}

//-----------------------------------------------------------------------------
