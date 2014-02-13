#ifndef __ISVI_H__
#define __ISVI_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);
void time_start(struct timeval *start);
long time_stop(struct timeval start);

#endif // __ISVI_H__
