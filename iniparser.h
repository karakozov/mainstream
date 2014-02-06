#ifndef __INIPARSER_H__
#define __INIPARSER_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

#define SECTION_NAME "VARIABLE"

struct app_params_t {
    unsigned fpgaNumber;
    unsigned dmaChannel;
    unsigned adcMask;
    float adcFreq;
    unsigned dmaBlockSize;
    unsigned dmaBlockCount;
    unsigned dmaBuffersCount;
    unsigned testMode;
};

struct sync_params_t {
    unsigned sync_mode;
    float sync_fd;
    float sync_fo;
    unsigned sync_selclk0;
    unsigned sync_selclk1;
    unsigned sync_selclk2;
    unsigned sync_selclk3;
    unsigned sync_selclkout;
    unsigned sync_enpow_ocxo;
    unsigned sync_reg3;
    unsigned sync_reg2;
    unsigned sync_reg0;
    unsigned sync_reg1;
    unsigned sync_div0;
    unsigned sync_div1;
    unsigned sync_div2;
    unsigned sync_div3;
};

bool getParams(int argc, char *argv[], struct app_params_t& param);
void showParams(struct app_params_t& param);

bool getSyncParams(int argc, char *argv[], struct sync_params_t& param);
void showSyncParams(struct sync_params_t& param);

#endif // __INIPARSER_H__
