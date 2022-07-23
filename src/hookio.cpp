#include "hookio.h"
#include "logger.h"
#include "fiberpool.h"
#include "macro.h"

#include <unistd.h>
#include <dlfcn.h>
#include <sys/epoll.h>

namespace MyRPC{
    // 原始的系统调用入口
    ssize_t (*sys_read)(int fd, void *buf, size_t count) = nullptr;
    ssize_t (*sys_write)(int fd, const void *buf, size_t count) = nullptr;
    int (*sys_close)(int fd) = nullptr;

    namespace Initializer {
        int _hook_io_initializer = []() {
            sys_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
            sys_write = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
            sys_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
            return 0;
        }();
    }
}

using namespace MyRPC;

extern "C"{

// 覆盖posix read函数
ssize_t read(int fd, void *buf, size_t count) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to read({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), fd, buf, count);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(fd, EventManager::READ);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                case EPERM:
                    enable_hook = true;
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
                    Logger::debug("Thread:{} Fiber:{} trying to call a non-block read syscall",
                                  FiberPool::GetCurrentThreadId(),
                                  MyRPC::Fiber::GetCurrentId());
#endif
                    break;
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_read(fd, buf, count);
}

// 覆盖posix write函数
ssize_t write(int fd, const void *buf, size_t count) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to write({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), fd, buf, count);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(fd, EventManager::WRITE);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                case EPERM:
                    enable_hook = true;
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
                    Logger::debug("Thread:{} Fiber:{} trying to call a non-block write syscall",
                                  FiberPool::GetCurrentThreadId(),
                                  MyRPC::Fiber::GetCurrentId());
#endif
                    break;
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_write(fd, buf, count);
}

// 覆盖posix close函数
int close(int fd) {
    return sys_close(fd);
}
}
