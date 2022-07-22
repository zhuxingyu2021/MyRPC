#include "macro.h"

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL

namespace MyRPC::Initializer{
    int _debug_initializer = [](){
        spdlog::set_level(spdlog::level::debug);
        return 0;
    }();
}

#endif
