#ifndef MYRPC_HOOK_IO_H
#define MYRPC_HOOK_IO_H

namespace MyRPC{
    extern thread_local bool enable_hook;

    namespace Initializer {
        extern int _hook_io_initializer;

        static int _my_hook_io_initializer = _hook_io_initializer;
    }
}

#endif //MYRPC_HOOK_IO_H
