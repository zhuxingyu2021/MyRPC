#ifndef MYRPC_HOOK_SLEEP_H
#define MYRPC_HOOK_SLEEP_H

namespace MyRPC{
    extern thread_local bool enable_hook;

    namespace Initializer {
        extern int _hook_sleep_initializer;

        static int _my_hook_sleep_initializer = _hook_sleep_initializer;
    }
}

#endif //MYRPC_HOOK_SLEEP_H
