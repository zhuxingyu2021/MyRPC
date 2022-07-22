# include "fiberpool.h"
# include "logger.h"
# include "eventmanager.h"
# include <sys/timerfd.h>
# include <cstring>
# include <iostream>

using namespace MyRPC;

#define NUM_THREADS 1

int main(){

    FiberPool fp(NUM_THREADS);
    fp.Start();

    auto f = fp.Run([](){
        auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

        struct itimerspec its;
        memset(&its, 0, sizeof(its));
        its.it_value.tv_sec = 1;
        MYRPC_SYS_ASSERT(timerfd_settime(timer_fd, 0, &its, NULL) == 0);

        FiberPool::GetEventManager()->AddIOEvent(timer_fd, EventManager::READ);

        Fiber::Block();

        close(timer_fd);

        Logger::info("Timer expired!");
    }, 0, true);

    f.Join();
    fp.Stop();

    return 0;
}
