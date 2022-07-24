#ifndef MYRPC_HOOKIO_H
#define MYRPC_HOOKIO_H

namespace MyRPC{
    extern thread_local bool enable_hook;

    namespace Initializer {
        extern int _hook_io_initializer;

        static int _my_hook_io_initializer = _hook_io_initializer;
    }
}

#endif //MYRPC_HOOKIO_H
