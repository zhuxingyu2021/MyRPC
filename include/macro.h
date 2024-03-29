#ifndef MYRPC_MACRO_H
#define MYRPC_MACRO_H

#include "logger.h"

#define MYRPC_DEBUG_ON_LEVEL 1
#define MYRPC_DEBUG_RPC_LEVEL 2
#define MYRPC_DEBUG_NET_LEVEL 3
#define MYRPC_DEBUG_LOCK_LEVEL 4
#define MYRPC_DEBUG_HOOK_LEVEL 5
#define MYRPC_DEBUG_EPOLL_LEVEL 6
#define MYRPC_DEBUG_FIBER_POOL_LEVEL 7
#define MYRPC_DEBUG_FIBER_LEVEL 8

// 设置调试级别
#define MYRPC_DEBUG_LEVEL 8

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
#endif // MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL

#define MYRPC_ASSERT_EXCEPTION(x, throw) if(!(x)) { \
     Logger::warn("Assertion failed, in file:{}, line:{}", __FILE__, __LINE__); \
     throw; \
}

#define MYRPC_NO_IMPLEMENTATION_ERROR() { \
Logger::critical("No Implementation Error! In file {}, line: {}", __FILE__, __LINE__); \
exit(-1); \
}

#define MYRPC_CRITIAL_ERROR(msg) { \
Logger::critical("{}! In file {}, line: {}",(msg),  __FILE__, __LINE__); \
exit(-1);                                   \
}

#endif //MYRPC_MACRO_H
