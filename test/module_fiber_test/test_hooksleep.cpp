# include "fiber/fiber_pool.h"
# include "logger.h"

using namespace MyRPC;

#define NUM_THREADS 1

int main(){
    FiberPool fp(NUM_THREADS);
    fp.Start();

    Logger::info("sleep Test Start!");
    Logger::info("Syscall sleep({}) started!", 1);
    sleep(1);
    Logger::info("Syscall sleep({}) finished!", 1);

    auto f = fp.Run([](){
        sleep(2);

        Logger::info("Timer expired!");
    }, 0);


    f->Join();

    Logger::info("usleep Test Start!");
    Logger::info("Syscall usleep({}) started!", 10000);
    usleep(10000);
    Logger::info("Syscall usleep({}) finished!", 10000);

    auto g = fp.Run([](){
        usleep(20000);

        Logger::info("Timer expired!");
    }, 0);

    g->Join();


    Logger::info("usleep Test Start!");
    Logger::info("Syscall nanosleep({}) started!", 100000000);
    struct timespec ts = {0, 100000000};
    nanosleep(&ts, NULL);
    Logger::info("Syscall nanosleep({}) finished!", 100000000);

    auto h = fp.Run([](){
        struct timespec ts = {0, 200000000};
        nanosleep(&ts, NULL);

        Logger::info("Timer expired!");
    }, 0);

    h->Join();

    fp.Stop();

    return 0;
}
