#include "hook.h"
#include "logger.h"
#include "fiberpool.h"
#include "macro.h"

#include <dlfcn.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace MyRPC{
    thread_local bool enable_hook = false;

    unsigned int (*sys_sleep)(unsigned int seconds) = nullptr;
    int (*sys_usleep)(__useconds_t __useconds) = nullptr;
    int (*sys_nanosleep)(const struct timespec *__req, struct timespec *__rem) = nullptr;

    namespace Initializer {
        int _hook_initializer = []() {
            sys_sleep = (unsigned int (*)(unsigned int))dlsym(RTLD_NEXT, "sleep");
            sys_usleep = (int (*)(__useconds_t))dlsym(RTLD_NEXT, "usleep");
            sys_nanosleep = (int (*)(const struct timespec *, struct timespec *))dlsym(RTLD_NEXT, "nanosleep");
            return 0;
        }();
    }
}

using namespace MyRPC;


extern "C"{
// 覆盖glibc的sleep函数
unsigned int sleep (unsigned int __seconds) {
    if (enable_hook) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to sleep({})", FiberPool::GetCurrentThreadId(),
                            MyRPC::Fiber::GetCurrentId(), __seconds);
#endif
        if(__seconds > 0) {
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            memset(&its, 0, sizeof(its));
            its.it_value.tv_sec = __seconds;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            close(timer_fd);
        }
        return 0;
    }
    return MyRPC::sys_sleep(__seconds);
}

//覆盖glibc的usleep函数
int usleep (__useconds_t __useconds) {
    if (enable_hook) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to usleep({})", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), __useconds);
#endif
        if(__useconds > 0) {
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            memset(&its, 0, sizeof(its));
            its.it_value.tv_nsec = __useconds * 1000;
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            close(timer_fd);
        }
        return 0;
    }
    return MyRPC::sys_usleep(__useconds);
}

//覆盖glibc的nanosleep函数
int nanosleep (const struct timespec *__req, struct timespec *__rem) {
    if (enable_hook) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_HOOK_LEVEL
        Logger::debug("Thread:{} Fiber:{} trying to nanosleep(sec:{}, nsec:{})", FiberPool::GetCurrentThreadId(),
                      MyRPC::Fiber::GetCurrentId(), __req->tv_sec, __req->tv_nsec);
#endif
        if(__req->tv_sec > 0 || __req->tv_nsec > 0) {
            auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

            struct itimerspec its;
            memset(&its, 0, sizeof(its));
            memcpy(&its.it_value, __req, sizeof(struct timespec));
            MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

            FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

            Fiber::Block();

            close(timer_fd);
        }
        if(__rem != NULL) {
            memset(__rem, 0, sizeof(struct timespec));
        }
        return 0;
    }
    return MyRPC::sys_nanosleep(__req, __rem);
}
}
