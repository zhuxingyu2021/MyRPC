#ifndef MYRPC_HOOK_H
#define MYRPC_HOOK_H

namespace MyRPC{
    extern thread_local bool enable_hook;

    namespace Initializer {
        extern int _hook_initializer;

        static int _my_hook_initializer = _hook_initializer;
    }
}

#endif //MYRPC_HOOK_H
