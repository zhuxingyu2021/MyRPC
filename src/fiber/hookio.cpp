#include "fiber/hookio.h"
#include "logger.h"
#include "fiber/fiberpool.h"
#include "macro.h"

#include <unistd.h>
#include <dlfcn.h>

namespace MyRPC{
    // 原始的系统调用入口
    // 文件系统调用
    ssize_t (*sys_read)(int fd, void *buf, size_t count) = nullptr;
    ssize_t (*sys_write)(int fd, const void *buf, size_t count) = nullptr;
    int (*sys_close)(int fd) = nullptr;

    // socket调用
    int (*sys_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen) = nullptr;
    int (*sys_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = nullptr;
    ssize_t (*sys_recv)(int sockfd, void *buf, size_t len, int flags) = nullptr;
    ssize_t (*sys_send)(int sockfd, const void *buf, size_t len, int flags) = nullptr;

    namespace Initializer {
        int _hook_io_initializer = []() {
            sys_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
            sys_write = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
            sys_close = (int (*)(int))dlsym(RTLD_NEXT, "close");

            sys_accept = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
            sys_connect = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
            sys_send = (ssize_t (*)(int, const void *, size_t, int))dlsym(RTLD_NEXT, "send");
            sys_recv = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_NEXT, "recv");
            return 0;
        }();
    }
}// namespace MyRPC

using namespace MyRPC;

// 覆盖posix read函数
extern "C" ssize_t read(int fd, void *buf, size_t count) {
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
extern "C" ssize_t write(int fd, const void *buf, size_t count) {
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
extern "C" int close(int fd) {
    return sys_close(fd);
}

// 覆盖posix accept函数
extern "C" int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to accept({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, *addrlen);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::READ);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_accept(sockfd, addr, addrlen);
}

// 覆盖posix connect函数
extern "C" int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
    Logger::debug("Thread:{} Fiber:{} trying to connect({}, {}, {})", FiberPool::GetCurrentThreadId(),
                                MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, addrlen);
#endif

        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::WRITE);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_connect(sockfd, addr, addrlen);
}

// 覆盖posix recv函数
extern "C" ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to recv({}, {}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::READ);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_recv(sockfd, buf, len, flags);

}

// 覆盖posix send函数
extern "C" ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to send({}, {}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags);
#endif

        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::WRITE);
        if(!err){
            Fiber::Block();
        }
        else {
            switch (errno) {
                default:
                    MYRPC_SYS_ASSERT(false);
            }
        }
    }
    return sys_send(sockfd, buf, len, flags);
}
