#include "fiber/fiber_sync.h"

#include <sys/eventfd.h>
#include <fcntl.h>
#include <unordered_set>

using namespace MyRPC;
using namespace MyRPC::FiberSync;

static std::atomic<int> mutex_count = 0;

// Mutex id  同一个线程中是否有协程在等待获得锁
static thread_local std::unordered_set<int> wait_for_acquire_mutex_set;


Mutex::Mutex(){
    // 初始化event_fd
    m_event_fd = eventfd(0, O_NONBLOCK);
    MYRPC_SYS_ASSERT(m_event_fd > 0);

    m_mutex_id = ++mutex_count;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("New mutex id: {}, event_fd: {}", m_mutex_id, m_event_fd);
#endif
}

Mutex::~Mutex() {
    MYRPC_SYS_ASSERT(close(m_event_fd) == 0);

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("Mutex id: {} closed", m_mutex_id);
#endif
}

void Mutex::lock() {
    ++m_waiter;
    // 以trylock方式尝试获得锁，如果获得锁失败，则阻塞等待
    if(!tryLock()){ // trylock失败
        // 在成功获得锁之前阻塞自己
        do{
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
            Logger::debug("Thread: {}, Fiber: {} is failed to acquire a lock, block mutex id: {}, thread has waiter: {}", FiberPool::GetCurrentThreadId(),
                          Fiber::GetCurrentId(), m_mutex_id, wait_for_acquire_mutex_set.count(m_mutex_id) );
#endif

            if(wait_for_acquire_mutex_set.count(m_mutex_id) == 0) {
                // 在同一线程中，没有协程在等待获得锁
                wait_for_acquire_mutex_set.insert(m_mutex_id);

                uint64_t val;
                read(m_event_fd, &val, sizeof(val)); // 阻塞等待

                wait_for_acquire_mutex_set.erase(m_mutex_id);
            }else{
                Fiber::Suspend();
            }
        }while(m_lock.test_and_set(std::memory_order_acquire));
    }
    --m_waiter;
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("Thread: {}, Fiber: {} has acquired a lock, mutex id: {}", FiberPool::GetCurrentThreadId(),
                  Fiber::GetCurrentId(), m_mutex_id);
    _debug_lock_owner = Fiber::GetCurrentId();
#endif
}

void Mutex::unlock() {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    _debug_lock_owner = -10;
#endif

    m_lock.clear(std::memory_order_release); // 清除锁标志
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("Thread: {}, Fiber: {} has released a lock, mutex id: {}", FiberPool::GetCurrentThreadId(),
                  Fiber::GetCurrentId(), m_mutex_id);
#endif
    if(m_waiter > 0){ // 如果有协程阻塞在锁上，那么唤醒该协程
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
        Logger::debug("Thread: {}, Fiber: {} is trying to wake up blocked fiber. Waiter count:{}, wakeup event_fd: {}, mutex id:{}", FiberPool::GetCurrentThreadId(),
                      Fiber::GetCurrentId(), m_waiter.load(), m_event_fd, m_mutex_id);
#endif
        auto tmp = enable_hook;
        enable_hook = false;

        uint64_t val = 1;
        MYRPC_SYS_ASSERT(write(m_event_fd, &val, sizeof(uint64_t)) == sizeof(uint64_t));

        enable_hook = tmp;
    }
}


