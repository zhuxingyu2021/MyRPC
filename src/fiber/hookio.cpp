#include "fiber/hookio.h"
#include "fiber/timeoutio.h"
#include "logger.h"
#include "fiber/fiberpool.h"
#include "macro.h"

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

ssize_t MyRPC::read_timeout(int fd, void *buf, size_t count, const timespec& ts) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to read({}, {}, {}) with timeout", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), fd, buf, count);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(fd, EventManager::READ);
        if(!err){

            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            its.it_interval.tv_nsec=0;
            its.it_interval.tv_sec=0;
            its.it_value = ts;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            FiberPool::GetEventManager()->RemoveIOEvent(timer_fd, EventManager::READ);
            sys_close(timer_fd);

            // 如果fd的读事件还没被触发，说明超时
            if(FiberPool::GetEventManager()->IsExistIOEvent(fd, EventManager::READ)) {
                MYRPC_SYS_ASSERT(FiberPool::GetEventManager()->RemoveIOEvent(fd, EventManager::READ)==0);
                return -2;
            }
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

int MyRPC::accept_timeout(int sockfd, struct sockaddr *addr, socklen_t *addrlen, const timespec& ts) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to accept({}, {}, {}) with timeout", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, *addrlen);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::READ);
        if(!err){
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            its.it_interval.tv_nsec=0;
            its.it_interval.tv_sec=0;
            its.it_value = ts;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            FiberPool::GetEventManager()->RemoveIOEvent(timer_fd, EventManager::READ);
            sys_close(timer_fd);

            // 如果sockfd的读事件还没被触发，说明超时
            if(FiberPool::GetEventManager()->IsExistIOEvent(sockfd, EventManager::READ)) {
                MYRPC_SYS_ASSERT(FiberPool::GetEventManager()->RemoveIOEvent(sockfd, EventManager::READ)==0);
                return -2;
            }
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

int MyRPC::connect_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const timespec& ts) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to connect({}, {}, {}) with timeout", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), sockfd, (void*)addr, addrlen);
#endif

        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::WRITE);
        if(!err){
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            its.it_interval.tv_nsec=0;
            its.it_interval.tv_sec=0;
            its.it_value = ts;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            FiberPool::GetEventManager()->RemoveIOEvent(timer_fd, EventManager::READ);
            sys_close(timer_fd);

            // 如果sockfd的读事件还没被触发，说明超时
            if(FiberPool::GetEventManager()->IsExistIOEvent(sockfd, EventManager::READ)) {
                MYRPC_SYS_ASSERT(FiberPool::GetEventManager()->RemoveIOEvent(sockfd, EventManager::READ)==0);
                return -2;
            }
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

ssize_t MyRPC::recv_timeout(int sockfd, void *buf, size_t len, int flags, const timespec& ts) {
    if (enable_hook) {
        enable_hook = false;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to recv({}, {}, {}, {}) with timeout", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), sockfd, buf, len, flags);
#endif
        auto err = FiberPool::GetEventManager()->AddIOEvent(sockfd, EventManager::READ);
        if(!err){
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            its.it_interval.tv_nsec=0;
            its.it_interval.tv_sec=0;
            its.it_value = ts;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            FiberPool::GetEventManager()->RemoveIOEvent(timer_fd, EventManager::READ);
            sys_close(timer_fd);

            // 如果sockfd的读事件还没被触发，说明超时
            if(FiberPool::GetEventManager()->IsExistIOEvent(sockfd, EventManager::READ)) {
                MYRPC_SYS_ASSERT(FiberPool::GetEventManager()->RemoveIOEvent(sockfd, EventManager::READ)==0);
                return -2;
            }
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
