#include "fiber/fiber_pool.h"
#include "fiber/fiber.h"
#include <thread>
#include <vector>
#include "logger.h"

#include <atomic>
#include <iostream>

using namespace MyRPC;

#define NUM_THREADS 8

int main(){
    FiberPool fp(NUM_THREADS);
    fp.Start();

    std::atomic<int> fiber_cnt=0;

    for(int i = 1; i <= 1000; i++){
        fp.Run([&fiber_cnt](){
            Logger::info("Hello World! From thread {}, fiber {}", FiberPool::GetCurrentThreadId(),Fiber::GetCurrentId());
            ++fiber_cnt;
        }, i%NUM_THREADS);
    }

    fp.Wait();

    std::cout << "Total fiber count: " << fiber_cnt << std::endl;

    fp.Stop();

    return 0;
}

