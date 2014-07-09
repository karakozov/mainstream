
#include "iniparser.h"
#include "utypes.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

bool getParams(int argc, char *argv[], struct app_params_t& param)
{
    int res = 0;
    IPC_str Buffer[128];
    IPC_str iniFilePath[1024];

    if(argc > 2) {
        fprintf(stderr, "usage: %s [alternate ini file name. mainstream.ini by default]\n", argv[0]);
        return false;
    }

    memset(&param, 0, sizeof(param));

    IPC_str *iniFileName = _BRDC("mainstream.ini");

    IPC_getCurrentDir(iniFilePath, sizeof(iniFilePath)/sizeof(char));
    BRDC_strcat(iniFilePath, _BRDC("/"));
    BRDC_strcat(iniFilePath, iniFileName);

    fprintf(stderr, "inifile = %s\n", iniFilePath);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("boardMask"), _BRDC("0x1F"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: boardMask - not found. Use default value\n");
    }
    param.boardMask = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("fpgaMask"), _BRDC("0x3"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: fpgaMask - not found. Use default value\n");
    }
    param.fpgaMask = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaChannel"), _BRDC("0x0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaChannel - not found. Use default value\n");
    }
    param.dmaChannel = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("adcMask"), _BRDC("0xF"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcMask - not found. Use default value\n");
    }
    param.adcMask = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("adcStart"), _BRDC("0x3"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcStart - not found. Use default value\n");
    }
    param.adcStart = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("adcStartInv"), _BRDC("0x1"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcStartInv - not found. Use default value\n");
    }
    param.adcStartInv = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBlockSize"), _BRDC("0x10000"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockSize - not found. Use default value\n");
    }
    param.dmaBlockSize = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBlockCount"), _BRDC("0x4"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockCount - not found. Use default value\n");
    }
    param.dmaBlockCount = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("dmaBuffersCount"), _BRDC("16"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBuffersCount - not found. Use default value\n");
    }
    param.dmaBuffersCount = BRDC_strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("testMode"), _BRDC("0x0"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: testMode - not found. Use default value\n");
    }
    param.testMode = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncMode"), _BRDC("0x3"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncMode - not found. Use default value\n");
    }
    param.syncMode = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncSelClkOut"), _BRDC("0x1"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncSelClkOut - not found. Use default value\n");
    }
    param.syncSelClkOut = BRDC_strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncFd"), _BRDC("448000000"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncFd - not found. Use default value\n");
    }
    param.syncFd = float(BRDC_strtod(Buffer,0));

    res = IPC_getPrivateProfileString(SECTION_NAME, _BRDC("syncFo"), _BRDC("56000000"), Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncFo - not found. Use default value\n");
    }
    param.syncFo = float(BRDC_strtod(Buffer,0));

    return true;
}

//-----------------------------------------------------------------------------

void showParams(struct app_params_t& param)
{
    fprintf(stderr, " \n");
    fprintf(stderr, "boardMask:         0x%x\n", param.boardMask);
    fprintf(stderr, "fpgaMask:          0x%x\n", param.fpgaMask);
    fprintf(stderr, "dmaChannel:        0x%x\n", param.dmaChannel);
    fprintf(stderr, "adcMask:           0x%x\n", param.adcMask);
    fprintf(stderr, "adcStart:          0x%x\n", param.adcStart);
    fprintf(stderr, "adcStartInv:       0x%x\n", param.adcStartInv);
    fprintf(stderr, "dmaBlockSize:      0x%x\n", param.dmaBlockSize);
    fprintf(stderr, "dmaBlockCount:     0x%x\n", param.dmaBlockCount);
    fprintf(stderr, "dmaBuffersCount:   0x%x\n", param.dmaBuffersCount);
    fprintf(stderr, "testMode:          0x%x\n", param.testMode);
    fprintf(stderr, "syncMode:          0x%x\n", param.syncMode);
    fprintf(stderr, "syncSelClkOut:     0x%x\n", param.syncSelClkOut);
    fprintf(stderr, "syncFd:            %f\n",   param.syncFd);
    fprintf(stderr, "syncFo:            %f\n",   param.syncFo);
    fprintf(stderr, " \n");
}

//-----------------------------------------------------------------------------

