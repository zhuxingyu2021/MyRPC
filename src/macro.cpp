#include "macro.h"

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL

namespace MyRPC{
    SpinLock _global_logger_spinlock;
    namespace Initializer {
        int _debug_initializer = []() {
            spdlog::set_level(spdlog::level::debug);
            return 0;
        }();
    } // namespace Initializer
}// namespace MyRPC

#endif
