#include "fiber/lockfree_queue.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "spinlock.h"
#include <mutex>

using namespace std;
using namespace MyRPC;

const int NUM_COUNT = 100000;
const int NUM_PROD_THREAD = 5;
const int NUM_CONSUM_THREAD = 5;


class Data
{
public:
    Data(long i) : i_(i) {}
    void dataPrint() {cout << "Hello";}

    long i_;
    double j;
};

Data* dataArray[NUM_COUNT];

MPMCLockFreeQueue<Data*, 1024> q;

SpinLock print_lock;

struct Producer
{
    void operator()()
    {
        for(long i=0; i<NUM_COUNT; i++) {
            while(!q.TryPush(dataArray[i]));
        }
    }
};


struct Consumer
{
    Data *pData;
    long result_;
    void operator()()
    {
        for(long i=0; i<NUM_COUNT; i++) {
            while (!q.TryPop(pData));
            {
                std::unique_lock<SpinLock> lock(print_lock);
                std::cout << pData->i_ << '\n';
            }
        }
    }
};

int main(){
    std::thread thrd [NUM_PROD_THREAD + NUM_CONSUM_THREAD];

    std::chrono::duration<double> elapsed_seconds;

    for (int i = 0; i < NUM_COUNT; ++i) dataArray[i] = new Data(i);

    auto start = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < NUM_PROD_THREAD;  i++ )
        thrd[i] = std::thread{ Producer() };

    for ( int i = 0; i < NUM_CONSUM_THREAD; i++ )
        thrd[NUM_PROD_THREAD+i] = std::thread{Consumer()};

    for ( int i = 0; i < NUM_CONSUM_THREAD; i++ )
        thrd[NUM_PROD_THREAD+i].join();

    auto end = std::chrono::high_resolution_clock::now();
    elapsed_seconds = end - start;
    std::cout << "Enqueue and Dequeue 1 million item in:"
              << elapsed_seconds.count() << std::endl;

    for ( int i = 0; i < NUM_PROD_THREAD; i++ )
        thrd[i].join();
    for (int i = 0; i < NUM_COUNT; ++i)
        delete dataArray[i];
}
