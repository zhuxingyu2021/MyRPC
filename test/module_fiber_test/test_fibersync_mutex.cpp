#include "fiber/synchronization.h"
#include "fiber/fiber_pool.h"

#include <mutex>
#include <cstdlib>
#include <iostream>

using namespace MyRPC;

#define THREADS_NUM 6
#define FIBER_COUNT 16

int main(){
    FiberPool fp(THREADS_NUM);
    fp.Start();

    FiberSync::Mutex mutex;
    int count = 0;

    for(int i=0;i<FIBER_COUNT;i++){
        fp.Run([&mutex, &count](){
            mutex.lock();
            ++count;
            mutex.unlock();
        }, rand()%THREADS_NUM);
    }
    fp.Wait();

    std::cout << "Fiber count: " << count << std::endl;
}
