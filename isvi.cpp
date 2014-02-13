
#include "isvi.h"

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

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
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

    while( !handle ) {
        handle = IPC_openFile(fileName, IPC_OPEN_FILE | IPC_FILE_RDWR);
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
        } break;
        if((rr==(int)0xffffffff) && (ii>0x10000)) {
        } break;
    }

    return true;
}

//-----------------------------------------------------------------------------

IPC_handle createDataFile(const char *fname)
{
    //IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR | IPC_FILE_DIRECT);
    IPC_handle fd = IPC_openFile(fname, IPC_CREATE_FILE | IPC_FILE_RDWR);
    if(!fd) {
        fprintf(stderr, "%s(): Error create file: %s\n", __FUNCTION__, fname);
        throw;
    }

    fprintf(stderr, "%s(): %s - Ok\n", __FUNCTION__, fname);

    return fd;
}

//-----------------------------------------------------------------------------

bool createFlagFile(const char *fname)
{
    //IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR | IPC_FILE_DIRECT);
    IPC_handle fd = IPC_openFile(fname, IPC_FILE_RDWR);
    if(!fd) {
        fprintf(stderr, "%s(): Error create file: %s\n", __FUNCTION__, fname);
        throw;
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
#endif
//-----------------------------------------------------------------------------
