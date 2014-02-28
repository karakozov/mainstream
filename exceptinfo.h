
#ifndef __EXCEPTION_INFO_H__
#define __EXCEPTION_INFO_H__

#include <string>

typedef  struct _exception_info_t {
    std::string info;
} exception_info_t;

exception_info_t exception_info(const char *fmt, ...);

#endif //__EXCEPTION_INFO_H__

