#ifndef __ISVI_H__
#define __ISVI_H__

#ifndef __GIPCY_H__
#include "gipcy.h"
#endif

IPC_handle createDataFile(const char *fname);
bool createFlagFile(const char *fname);
bool lockDataFile(const char* fname, int counter);

#endif // __ISVI_H__
