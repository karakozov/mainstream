
#include "iniparser.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
//#include <pthread.h>
//#include <sys/time.h>
//#include <sys/mman.h>
//#include <getopt.h>

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
        fprintf(stderr, "usage: mainstream [alternate ini file name. mainstream.ini by default]\n");
        return false;
    }

    memset(&param, 0, sizeof(param));

    string s = argv[0];
    string iniFileName = "";
    if(argc == 1) {
        iniFileName = s + ".ini";
    } else {
        iniFileName = argv[1];
    }

    IPC_getCurrentDir(iniFilePath, sizeof(iniFilePath)/sizeof(char));
    strcat(iniFilePath, "/");
    strcat(iniFilePath, iniFileName.c_str());

    fprintf(stderr, "inifile = %s\n", iniFilePath);

    res = IPC_getPrivateProfileString(SECTION_NAME, "fpgaNumber", "0x2", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: fpgaNumber - not found. Use default value\n");
    }
    param.fpgaNumber = strtol(Buffer,0,16);

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

    res = IPC_getPrivateProfileString(SECTION_NAME, "adcFreq", "350000000", Buffer, sizeof(Buffer), iniFilePath);
    if(!res) {
        fprintf(stderr, "Parameter: adcFreq - not found. Use default value\n");
    }
    param.adcFreq = (float)strtod(Buffer,0);

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

    return true;
}

//-----------------------------------------------------------------------------

void showParams(struct app_params_t& param)
{
    fprintf(stderr, "fpgaNumber:        0x%d\n", param.fpgaNumber);
    fprintf(stderr, "dmaChannel:        0x%d\n", param.dmaChannel);
    fprintf(stderr, "adcMask:           0x%x\n", param.adcMask);
    fprintf(stderr, "adcFreq:           %f\n", param.adcFreq);
    fprintf(stderr, "dmaBlockSize:      0x%x\n", param.dmaBlockSize);
    fprintf(stderr, "dmaBlockCount:     0x%x\n", param.dmaBlockCount);
    fprintf(stderr, "dmaBuffersCount:   0x%x\n", param.dmaBuffersCount);
    fprintf(stderr, "testMode:          0x%x\n", param.testMode);
}


