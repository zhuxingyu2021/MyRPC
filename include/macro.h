#ifndef MYRPC_MACRO_H
#define MYRPC_MACRO_H

#include "logger.h"

#define MYRPC_DEBUG_ON_LEVEL 1
#define MYRPC_DEBUG_HOOK_LEVEL 3
#define MYRPC_DEBUG_FIBER_POOL_LEVEL 4

#define MYRPC_DEBUG_SYS_CALL

// 设置调试级别
#define MYRPC_DEBUG_LEVEL MYRPC_DEBUG_FIBER_POOL_LEVEL

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL
#define MYRPC_ASSERT(x) if(!(x)) { \
    Logger::error("Assertion failed: {}, in file:{}, line:{}", #x, __FILE__, __LINE__); \
    exit(-1);                              \
}

#define MYRPC_SYS_ASSERT(x)  if(!(x)) { \
    Logger::error("System Error: {}, in file:{}, line:{}, info:{}", #x, __FILE__, __LINE__, strerror(errno)); \
    exit(-1);                              \
}

namespace MyRPC::Initializer {
    extern int _debug_initializer;

    static int _my_debug_initializer = _debug_initializer;
}

#else
#define MYRPC_ASSERT(x) {x;}
#define MYRPC_SYS_ASSERT(x) {x;}
#endif

#endif //MYRPC_MACRO_H
