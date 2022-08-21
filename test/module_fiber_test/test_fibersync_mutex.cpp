#include "fiber/synchronization.h"
#include "fiber/fiber_pool.h"
#include "fiber/fiber.h"

#include <mutex>
#include <cstdlib>
#include <iostream>

using namespace MyRPC;

#define THREADS_NUM 8
#define FIBER_COUNT 4096

int main(){
    FiberPool fp(THREADS_NUM);
    fp.Start();

    FiberSync::Mutex mutex;
    int count = 0;

    for(int i=0;i<FIBER_COUNT;i++){
        fp.Run([&mutex, &count](){
            std::unique_lock<FiberSync::Mutex> lock(mutex);
            ++count;
            Logger::info("Fiber: {} has acquired the lock", Fiber::GetCurrentId());
        }, rand()%THREADS_NUM);
    }
    fp.Wait();

    Logger::info("Fiber count: {}", count);
}
