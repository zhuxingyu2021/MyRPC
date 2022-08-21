#include "fiber/synchronization.h"
#include "fiber/fiber_pool.h"
#include "fiber/fiber.h"

#include <mutex>
#include <shared_mutex>
#include <cstdlib>
#include <iostream>

using namespace MyRPC;

#define THREADS_NUM 8
#define WRITER_COUNT 128
#define READER_COUNT 256

int main(){
    FiberPool fp(THREADS_NUM);

    FiberSync::RWMutex rwlock;
    volatile int var = 10;

    for(int i=0; i<WRITER_COUNT; i++){
        fp.Run([&rwlock, &var](){
            std::unique_lock<FiberSync::RWMutex> lock(rwlock);
            var += 10;
            Logger::info("write var: {}", var);
        }, rand()%THREADS_NUM);
    }

    for(int i=0; i<READER_COUNT; i++){
        fp.Run([&rwlock, &var](){
            std::shared_lock<FiberSync::RWMutex> lock_shared(rwlock);
            Logger::info("read var: {}", var);
        }, rand()%THREADS_NUM);
    }

    fp.Start();
    fp.Wait();
}
