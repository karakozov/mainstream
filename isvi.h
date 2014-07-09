#ifndef __ISVI_H__
#define __ISVI_H__

#include "iniparser.h"
#include "gipcy.h"

#include <string>

IPC_handle createDataFile(const IPC_str *fname);
bool createFlagFile(const IPC_str *fname);
bool createIsviHeader(std::string& hdr,
                      unsigned char hwAddr,
                      unsigned char hwFpgaNum,
                      struct app_params_t& params);
bool lockDataFile(const IPC_str *name, int counter);
void time_start(struct timeval *start);
long time_stop(struct timeval start);

#endif // __ISVI_H__
