#include "fiberpool.h"
#include "fiber.h"
#include <iostream>
#include <vector>
#include "logger.h"

using namespace MyRPC;

#define NUM_THREADS 8

int main(){
    FiberPool fp(NUM_THREADS);
    fp.Start();

    std::vector<FiberPool::FiberController> v;

    for(int i = 1; i <= 1000; i++){
        v.push_back(fp.Run([](){
            Logger::info("Hello World! From thread {}, fiber {}", FiberPool::GetCurrentThreadId(),Fiber::GetCurrentId());
        }, i%NUM_THREADS));
    }

    for(auto &f : v){
        f.Join();
    }

    fp.Stop();
    return 0;
}

