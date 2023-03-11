#include "fiber/hook_io.h"
#include "fiber/timeout_io.h"
#include "fiber/fiber_pool.h"
#include "logger.h"
#include "macro.h"
#include "fd_raii.h"

#include <unistd.h>
#include <dlfcn.h>
#include <sys/timerfd.h>

namespace MyRPC{
    // 原始的系统调用入口
    // 文件系统调用
    ssize_t (*sys_read)(int fd, void *buf, size_t count) = nullptr;
    ssize_t (*sys_write)(int fd, const void *buf, size_t count) = nullptr;
    int (*sys_close)(int fd) = nullptr;

    // socket调用
    int (*sys_accept)(int sockfd, sockaddr *addr, socklen_t *addrlen) = nullptr;
    int (*sys_connect)(int sockfd, const sockaddr *addr, socklen_t addrlen) = nullptr;
    ssize_t (*sys_recv)(int sockfd, void *buf, size_t len, int flags) = nullptr;
    ssize_t (*sys_send)(int sockfd, const void *buf, size_t len, int flags) = nullptr;
    ssize_t (*sys_readv)(int fd, const struct iovec *iov, int iovcnt) = nullptr;
    ssize_t (*sys_writev)(int fd, const struct iovec *iov, int iovcnt) = nullptr;

    namespace Initializer {
        int _hook_io_initializer = []() {
            sys_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
            sys_write = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
            sys_close = (int (*)(int))dlsym(RTLD_NEXT, "close");

            sys_accept = (int (*)(int, sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
            sys_connect = (int (*)(int, const sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
            sys_send = (ssize_t (*)(int, const void *, size_t, int))dlsym(RTLD_NEXT, "send");
            sys_recv = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_NEXT, "recv");

            sys_readv = (ssize_t (*)(int, const struct iovec *, int)) dlsym(RTLD_NEXT, "readv");
            sys_writev = (ssize_t (*)(int, const struct iovec *, int)) dlsym(RTLD_NEXT, "writev");
            return 0;
        }();
    }
}// namespace MyRPC

using namespace MyRPC;

#define IO_BEGIN if (enable_hook) { \
enable_hook = false;                \


#define IO_WAIT(fd, event)  auto err = FiberPool::GetEventManager()->AddIOEvent(fd, EventManager::event); \
    if(!err){ \
        Fiber::Block(); \
    } \
    else { \
        switch (errno) { \
            case EPERM: \
            enable_hook = true; \
            Logger::debug("Thread: {}, Fiber: {} trying to call a non-block read syscall", \
            FiberPool::GetCurrentThreadId(), Fiber::GetCurrentId()); \
            break; \
            default: \
            MYRPC_SYS_ASSERT(false); \
        } \
    } \
}                                                                                                    \


// 覆盖posix read函数
extern "C" ssize_t read(int fd, void *buf, size_t count) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to read({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), fd, buf, count);
#endif
    IO_WAIT(fd, READ);
    return sys_read(fd, buf, count);
}

// 覆盖posix write函数
extern "C" ssize_t write(int fd, const void *buf, size_t count) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to write({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), fd, buf, count);
#endif
    IO_WAIT(fd, WRITE);
    return sys_write(fd, buf, count);
}

// 覆盖posix close函数
extern "C" int close(int fd) {
    if(enable_hook){
        FiberPool::GetEventManager()->RemoveIO(fd);
    }
    return sys_close(fd);
}

// 覆盖posix accept函数
extern "C" int accept(int sockfd, sockaddr *addr, socklen_t *addrlen) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to accept({}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, (void*)addrlen);
#endif
    IO_WAIT(sockfd, READ);
    return sys_accept(sockfd, addr, addrlen);
}

// 覆盖posix connect函数
extern "C" int connect(int sockfd, const sockaddr *addr, socklen_t addrlen) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
    Logger::debug("Thread: {}, Fiber: {} trying to connect({}, {}, {})", FiberPool::GetCurrentThreadId(),
                                MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, addrlen);
#endif
    IO_WAIT(sockfd, WRITE);
    return sys_connect(sockfd, addr, addrlen);
}

// 覆盖posix recv函数
extern "C" ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to recv({}, {}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags);
#endif
    IO_WAIT(sockfd, READ);
    return sys_recv(sockfd, buf, len, flags);
}

// 覆盖posix send函数
extern "C" ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to send({}, {}, {}, {})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags);
#endif
    IO_WAIT(sockfd, WRITE);
    return sys_send(sockfd, buf, len, flags);
}

// 覆盖posix readv函数
extern "C" ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to readv({}, 0x{:x}, {})", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), fd, (void*)iov, iovcnt);
#endif

    IO_WAIT(fd, READ);
    return sys_readv(fd, iov, iovcnt);
}

// 覆盖posix writev
extern "C" ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} trying to writev({}, 0x{:x}, {})", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), fd, (void*)iov, iovcnt);
#endif

    IO_WAIT(fd, WRITE);
    return sys_writev(fd, iov, iovcnt);
}

#define TIMER_IO_WAIT(fd, event) \
            auto err = FiberPool::GetEventManager()->AddIOEvent(fd, EventManager::event); \
            if(!err){ \
                if(ts > 0) { \
                    auto timer_fd = FdRAIIWrapper(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)); \
                     \
                    struct itimerspec its; \
                    memset(&its, 0, sizeof(its)); \
                    its.it_value.tv_sec = ts / 1000000; \
                    its.it_value.tv_nsec = (ts % 1000000) * 1000; \
                    MYRPC_SYS_ASSERT(timerfd_settime(timer_fd.Getfd(), 0, &its, NULL) == 0); \
                     \
                    FiberPool::GetEventManager()->AddIOEvent(timer_fd.Getfd(), EventManager::READ); \
                     \
                    Fiber::Block(); \
                     \
                    FiberPool::GetEventManager()->RemoveIOEvent(timer_fd.Getfd(), EventManager::READ); \
                    enable_hook = false; \
                    timer_fd.Closefd(); \
                    enable_hook = true; \
                    if (FiberPool::GetEventManager()->IsExistIOEvent(fd, EventManager::event)) { \
                        MYRPC_SYS_ASSERT(FiberPool::GetEventManager()->RemoveIOEvent(fd, EventManager::event) == 0); \
                        return MYRPC_ERR_TIMEOUT_FLAG; \
                    } \
                } \
                else { \
                    Fiber::Block(); \
                } \
            } \
            else { \
                switch (errno) { \
                case EPERM: \
                enable_hook = true; \
                break; \
                default: \
                MYRPC_SYS_ASSERT(false); \
            } \
        } \
    } \


namespace MyRPC{
    ssize_t read_timeout(int fd, void *buf, size_t count, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to read({}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), fd, buf, count, ts);
#endif
        TIMER_IO_WAIT(fd, READ);
        return sys_read(fd, buf, count);
    }

    int accept_timeout(int sockfd, sockaddr *addr, socklen_t *addrlen, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to accept({}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, (void*)addrlen, ts);
#endif
        TIMER_IO_WAIT(sockfd, READ);
        return sys_accept(sockfd, addr, addrlen);
    }

    int connect_timeout(int sockfd, const sockaddr *addr, socklen_t addrlen, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to connect({}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, addrlen, ts);
#endif
        TIMER_IO_WAIT(sockfd, WRITE);
        return sys_connect(sockfd, addr, addrlen);
    }

    ssize_t recv_timeout(int sockfd, void *buf, size_t len, int flags, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to recv({}, {}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags, ts);
#endif
        TIMER_IO_WAIT(sockfd, READ);
        return sys_recv(sockfd, buf, len, flags);
    }

    ssize_t write_timeout(int fd, const void *buf, size_t count, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to write({}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), fd, buf, count, ts);
#endif
        TIMER_IO_WAIT(fd, WRITE);
        return sys_write(fd, buf, count);
    }

    ssize_t send_timeout(int sockfd, const void *buf, size_t len, int flags, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to send({}, {}, {}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags, ts);
#endif
        TIMER_IO_WAIT(sockfd, WRITE);
        return sys_send(sockfd, buf, len, flags);
    }

    ssize_t readv_timeout(int fd, const struct iovec *iov, int iovcnt, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to readv({}, 0x{:x}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), fd, (uint64_t)iov, iovcnt, ts);
#endif

        TIMER_IO_WAIT(fd, READ);
        return sys_readv(fd, iov, iovcnt);
    }

    ssize_t writev_timeout(int fd, const struct iovec *iov, int iovcnt, useconds_t ts) {
        IO_BEGIN;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} trying to writev({}, 0x{:x}, {}) with timeout {}us", FiberPool::GetCurrentThreadId(),
                          MyRPC::Fiber::GetCurrentId(), fd, (uint64_t)iov, iovcnt, ts);
#endif

        TIMER_IO_WAIT(fd, WRITE);
        return sys_writev(fd, iov, iovcnt);
    }
}// namespace MyRPC
