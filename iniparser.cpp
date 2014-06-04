
#include "iniparser.h"

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
    char Buffer[128];
    char iniFilePath[1024];

    if(argc > 2) {
        fprintf(stderr, "usage: %s [alternate ini file name. mainstream.ini by default]\n", argv[0]);
        return false;
    }

    memset(&param, 0, sizeof(param));

    string iniFileName = "mainstream.ini";

    IPC_getCurrentDir(iniFilePath, sizeof(iniFilePath)/sizeof(char));
    strcat(iniFilePath, "/");
    strcat(iniFilePath, iniFileName.c_str());

    fprintf(stderr, "inifile = %s\n", iniFilePath);

    res = IPC_getPrivateProfileString(SECTION_NAME, "boardMask", "0x1F", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: boardMask - not found. Use default value\n");
    }
    param.boardMask = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "fpgaMask", "0x3", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: fpgaMask - not found. Use default value\n");
    }
    param.fpgaMask = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "dmaChannel", "0x0", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaChannel - not found. Use default value\n");
    }
    param.dmaChannel = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "adcMask", "0xF", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcMask - not found. Use default value\n");
    }
    param.adcMask = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "adcStart", "0x3", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcStart - not found. Use default value\n");
    }
    param.adcStart = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "dmaBlockSize", "0x10000", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockSize - not found. Use default value\n");
    }
    param.dmaBlockSize = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "dmaBlockCount", "0x4", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBlockCount - not found. Use default value\n");
    }
    param.dmaBlockCount = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "dmaBuffersCount", "16", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: dmaBuffersCount - not found. Use default value\n");
    }
    param.dmaBuffersCount = strtol(Buffer,0,10);

    res = IPC_getPrivateProfileString(SECTION_NAME, "testMode", "0x0", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: testMode - not found. Use default value\n");
    }
    param.testMode = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "syncMode", "0x3", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncMode - not found. Use default value\n");
    }
    param.syncMode = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "syncSelClkOut", "0x1", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncSelClkOut - not found. Use default value\n");
    }
    param.syncSelClkOut = strtol(Buffer,0,16);

    res = IPC_getPrivateProfileString(SECTION_NAME, "syncFd", "448000000", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncFd - not found. Use default value\n");
    }
    param.syncFd = (float)strtod(Buffer,0);

    res = IPC_getPrivateProfileString(SECTION_NAME, "syncFo", "56000000", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: syncFo - not found. Use default value\n");
    }
    param.syncFo = (float)strtod(Buffer,0);

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

