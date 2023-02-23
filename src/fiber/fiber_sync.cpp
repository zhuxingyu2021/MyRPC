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

    m_mutex_id = ++mutex_count;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("New mutex id: {}", m_mutex_id);
#endif
}

Mutex::~Mutex() {

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("Mutex id: {} closed", m_mutex_id);
#endif
}

void Mutex::lock() {
    // 以trylock方式尝试获得锁，如果获得锁失败，则阻塞等待
    while(!tryLock()){ // trylock失败
        // 在成功获得锁之前阻塞自己
        m_wait_queue_lock.lock();
        auto lock_id = m_lock_id.fetch_add(1);
        m_wait_queue.push(std::make_pair(lock_id, FiberPool::GetCurrentThreadId()));

        do{
            m_wait_queue_lock.unlock();
            Fiber::Suspend(); // 切换到其他协程
            m_wait_queue_lock.lock();
        }while(m_wait_queue.front().first != lock_id);

        m_wait_queue.pop();

        m_wait_queue_lock.unlock();
    }
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
    m_wait_queue_lock.lock();

    if(!m_wait_queue.empty()) {
        FiberPool::GetThis()->Notify(m_wait_queue.front().second);
    }

    m_wait_queue_lock.unlock();
}
