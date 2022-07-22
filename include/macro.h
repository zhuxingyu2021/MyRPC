#ifndef MYRPC_MACRO_H
#define MYRPC_MACRO_H

#include "logger.h"

#define MYRPC_ASSERT(x) if(!(x)) { \
    Logger::error("Assertion failed: {}, in file:{}, line:{}", #x, __FILE__, __LINE__); \
    exit(-1);                              \
}

#define MYRPC_SYS_ASSERT(x)  if(!(x)) { \
    Logger::error("System Error: {}, in file:{}, line:{}, info:{}", #x, __FILE__, __LINE__, strerror(errno)); \
    exit(-1);                              \
}

#endif //MYRPC_MACRO_H
