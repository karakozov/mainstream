
#include "isvi.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#endif
#include <fcntl.h>
#include <signal.h>

#include <vector>
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------

using namespace std;

//-----------------------------------------------------------------------------

void WriteFlagSinc(const char *fileName, int newData, int newHeader)
{
    IPC_handle handle = NULL;
    int val[2] = {0, 0};

    IPC_str fullName[256];
    IPC_getCurrentDir(fullName, sizeof(fullName));
    strcat(fullName, "//");
    strcat(fullName, fileName);

    while( !handle ) {
        handle = IPC_openFile(fullName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    val[0] = newData;
    val[1] = newHeader;
    int res = IPC_writeFile(handle, val, sizeof(val));
    if(res < 0) {
        fprintf( stderr, "Error write flag sync\r\n" );
    }
    IPC_closeFile(handle);
}

//-----------------------------------------------------------------------------

int  ReadFlagSinc(const char *fileName)
{
    IPC_handle handle = NULL;
    int flg;

    IPC_str fullName[256];
    IPC_getCurrentDir(fullName, sizeof(fullName));
    strcat(fullName, "//");
    strcat(fullName, fileName);

    while( !handle ) {
        handle = IPC_openFile(fullName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    }
    int res = IPC_readFile(handle, &flg, sizeof(flg));
    if(res < 0) {
        fprintf( stderr, "Error read flag sync\r\n" );
    }
    IPC_closeFile(handle);

    //fprintf( stderr, "%s(): FLG = 0x%x\n", __func__, flg );

    return flg;
}

//-----------------------------------------------------------------------------

bool lockDataFile(const char* fname, int counter)
{
    if(counter==0) {
        WriteFlagSinc(fname, 0xffffffff,0xffffffff);
    } else {
        WriteFlagSinc(fname, 0xffffffff,0x0);
    }

    for(int ii=0;;ii++) {

        int rr = ReadFlagSinc(fname);
#if USE_SIGNAL
        if(g_StopFlag) {
#else
        if(IPC_kbhit()) {
#endif
            break;
        }
        if(rr==0) {
            fprintf(stderr, "%s(): rr = 0x%x -- ii = 0x%x\n", __FUNCTION__, rr, ii);
            break;
        }
        if((rr==(int)0xffffffff) && (ii>0x10000)) {
            fprintf(stderr, "%s(): rr = 0x%x -- ii = 0x%x\n", __FUNCTION__, rr, ii);
            break;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname)
{
    IPC_str fullName[256];
    IPC_getCurrentDir(fullName, sizeof(fullName));
    strcat(fullName, "//");
    strcat(fullName, fname);

    IPC_handle fd = IPC_openFile(fullName, IPC_CREATE_FILE | IPC_FILE_RDWR);
    if(!fd) {
        throw except_info("%s, %d: %s() - Error in IPC_openFile(%s)\n", __FILE__, __LINE__, __FUNCTION__, fullName);
    }

    fprintf(stderr, "%s(): %s - Ok\n", __FUNCTION__, fullName);

    return fd;
}

//-----------------------------------------------------------------------------

bool createFlagFile(const char *fname)
{
    IPC_str fullName[256];
    IPC_getCurrentDir(fullName, sizeof(fullName));
    strcat(fullName, "//");
    strcat(fullName, fname);

    IPC_handle fd = IPC_openFile(fullName, IPC_OPEN_FILE | IPC_FILE_RDWR);
    if(!fd) {
        throw except_info("%s, %d: %s() - Error in IPC_openFile(%s)\n", __FILE__, __LINE__, __FUNCTION__, fullName);
    }

    int val[2] = {0, 0};
    int res = IPC_writeFile(fd, val, sizeof(val));
    if(res < 0) {
        fprintf( stderr, "Error write flag sync\r\n" );
    }

    IPC_closeFile(fd);

    return true;
}

//-----------------------------------------------------------------------------

bool createIsviHeader(std::string& hdr, unsigned char hwAddr, unsigned char hwFpgaNum, struct app_params_t& params)
{
    char str[128];
    hdr.clear();

    unsigned BufSize = 0;
    unsigned NumOfChannel = 0;
    for(int i=0; i<4; i++) {
        if( params.adcMask & (0x1 << i)) {
            NumOfChannel += 1;
        }
    }

    switch(params.testMode) {
    case 0: BufSize = params.dmaBlockSize * params.dmaBlockCount; break;
    case 1: BufSize = params.dmaBuffersCount * params.dmaBlockSize * params.dmaBlockCount; break;
    default: BufSize = params.dmaBlockSize * params.dmaBlockCount; break;
    }

    snprintf(str, sizeof(str), "DEVICE_NAME_________ AC_ADC_%d%d\r\n", hwAddr, hwFpgaNum);      hdr += str;
    snprintf(str, sizeof(str), "NUMBER_OF_CHANNELS__ %d\r\n", NumOfChannel);                    hdr += str;
    snprintf(str, sizeof(str), "NUMBERS_OF_CHANNELS_ 0,1,2,3\r\n");                             hdr += str;
    snprintf(str, sizeof(str), "NUMBER_OF_SAMPLES___ %d\r\n", BufSize / 4 / 2);                 hdr += str;
    snprintf(str, sizeof(str), "SAMPLING_RATE_______ %d\r\n", (int)((1.0e+6)*params.syncFd));   hdr += str;
    snprintf(str, sizeof(str), "BYTES_PER_SAMPLES___ 2\r\n");                                   hdr += str;
    snprintf(str, sizeof(str), "SAMPLES_PER_BYTES___ 1\r\n");                                   hdr += str;
    snprintf(str, sizeof(str), "IS_COMPLEX_SIGNAL?__ NO\r\n");                                  hdr += str;

    snprintf(str, sizeof(str), "SHIFT_FREQUENCY_____ 0.0,0.0,0.0,0.0\r\n");                     hdr += str;
    snprintf(str, sizeof(str), "GAINS_______________ 1.0,1.0,1.0,1.0\r\n");                     hdr += str;
    snprintf(str, sizeof(str), "VOLTAGE_OFFSETS_____ 0.0,0.0,0.0,0.0\r\n");                     hdr += str;
    snprintf(str, sizeof(str), "VOLTAGE_RANGE_______ 1\r\n");                                   hdr += str;
    snprintf(str, sizeof(str), "BIT_RANGE___________ 16\r\n");                                  hdr += str;

    return true;
}

//-----------------------------------------------------------------------------


#ifdef __linux__
void time_start(struct timeval *start)
{
    gettimeofday(start, 0);
}

//-----------------------------------------------------------------------------

long time_stop(struct timeval start)
{
    struct timeval stop;
    struct timeval dt;

    gettimeofday(&stop, 0);

    dt.tv_sec = stop.tv_sec - start.tv_sec;
    dt.tv_usec = stop.tv_usec - start.tv_usec;

    if(dt.tv_usec<0) {
        dt.tv_sec--;
        dt.tv_usec += 1000000;
    }

    return dt.tv_sec*1000 + dt.tv_usec/1000;
}
#else

#include <math.h>

void time_start(struct timeval *start)
{
	LARGE_INTEGER Frequency, StartPerformCount;
	int bHighRes = QueryPerformanceFrequency (&Frequency);
	QueryPerformanceCounter (&StartPerformCount);
	double msTime = (double)(StartPerformCount.QuadPart) / (double)Frequency.QuadPart * 1.E3;
	start->tv_sec =  (long)floor(msTime + 0.5); // save msec
}

//-----------------------------------------------------------------------------

long time_stop(struct timeval start)
{
	LARGE_INTEGER Frequency, StopPerformCount;
	QueryPerformanceCounter (&StopPerformCount);
	int bHighRes = QueryPerformanceFrequency (&Frequency);

	double msTime = (double)(StopPerformCount.QuadPart) / (double)Frequency.QuadPart * 1.E3;

	long msStopTime = (long)floor(msTime + 0.5);
	long msStartTime = start.tv_sec;

	return (msStopTime - msStartTime);
}

#endif
//-----------------------------------------------------------------------------
