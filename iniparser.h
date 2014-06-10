#ifndef __INIPARSER_H__
#define __INIPARSER_H__

#include "gipcy.h"

#define SECTION_NAME "VARIABLE"

struct app_params_t {
    unsigned boardMask;
    unsigned fpgaMask;
    unsigned dmaChannel;
    unsigned adcMask;
    unsigned adcStart;
    unsigned adcStartInv;
    unsigned dmaBlockSize;
    unsigned dmaBlockCount;
    unsigned dmaBuffersCount;
    unsigned testMode;
    unsigned syncMode;
    unsigned syncSelClkOut;
    float syncFd;
    float syncFo;
};

bool getParams(int argc, char *argv[], struct app_params_t& param);
void showParams(struct app_params_t& param);

#endif // __INIPARSER_H__
