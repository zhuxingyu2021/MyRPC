#include "fiber/fiber_sync.h"
#include "fiber/fiber_pool.h"

#include <sys/eventfd.h>
#include <fcntl.h>
#include <unordered_set>

using namespace MyRPC;
using namespace MyRPC::FiberSync;

static std::atomic<int> mutex_count = 0;


Mutex::Mutex(){

    m_mutex_id = ++mutex_count;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("New mutex id: {}", m_mutex_id);
#endif
}

Mutex::~Mutex() {
    // 删除所有阻塞的协程
    auto fp_this = FiberPool::GetThis();
    m_wait_queue_lock.lock();
    while(!m_wait_queue.empty()){
        auto& val = m_wait_queue.front();
        Fiber::ptr p_fib = val.first;

        fp_this->Run([p_fib]{
            p_fib->Term();
        }, val.second);

        m_wait_queue.pop();
    }
    m_wait_queue_lock.unlock();

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
    Logger::debug("Mutex id: {} closed", m_mutex_id);
#endif
}

void Mutex::lock() {
    // 以trylock方式尝试获得锁，如果获得锁失败，则阻塞等待
    while(!tryLock()){ // trylock失败
        // 在成功获得锁之前阻塞自己
        auto current_id = Fiber::GetCurrentId();
        m_wait_queue_lock.lock();
        m_wait_queue.emplace(Fiber::GetSharedFromThis(), FiberPool::GetCurrentThreadId());

        m_wait_queue_lock.unlock();
        Fiber::Block();
        m_wait_queue_lock.lock();

        MYRPC_ASSERT(m_wait_queue.front().first->GetCurrentId() == current_id);

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
        auto fp_this = FiberPool::GetThis();
        auto current_thread_id = FiberPool::GetCurrentThreadId();
        auto& [fiber, wake_thread_id] = m_wait_queue.front();
        fiber->SetStatus(Fiber::READY);

        fp_this->_run_internal(new Fiber::ptr(fiber), wake_thread_id);
        if(current_thread_id != wake_thread_id){
            fp_this->Notify(wake_thread_id);
        }
    }

    m_wait_queue_lock.unlock();
}

ConditionVariable::~ConditionVariable() {
    // 删除所有阻塞的协程
    auto fp_this = FiberPool::GetThis();
    m_wait_queue_lock.lock();
    while(!m_wait_queue.empty()){
        auto& val = m_wait_queue.front();
        Fiber::ptr p_fib = val.first;

        fp_this->Run([p_fib]{
            p_fib->Term();
        }, val.second);

        m_wait_queue.pop();
    }
    m_wait_queue_lock.unlock();
}

void ConditionVariable::wait(Mutex &mutex) {
    m_wait_queue_lock.lock();
    m_wait_queue.emplace(Fiber::GetSharedFromThis(), FiberPool::GetCurrentThreadId());

    m_wait_queue_lock.unlock();
    mutex.unlock();
    Fiber::Block();
    m_wait_queue_lock.lock();

    if(!m_notify_all){
        MYRPC_ASSERT(m_wait_queue.front().first->GetCurrentId() == Fiber::GetCurrentId());
    }

    m_wait_queue.pop();
    if(m_notify_all && m_wait_queue.empty()){
        m_notify_all = false;
    }
    m_wait_queue_lock.unlock();

    // 从等待队列出来后再加锁
    mutex.lock();
}

void ConditionVariable::notify_one() {
    m_wait_queue_lock.lock();
    if(!m_wait_queue.empty()) {
        auto fp_this = FiberPool::GetThis();
        auto current_thread_id = FiberPool::GetCurrentThreadId();
        auto& [fiber, wake_thread_id] = m_wait_queue.front();
        fiber->SetStatus(Fiber::READY);

        fp_this->_run_internal(new Fiber::ptr(fiber), wake_thread_id);
        if(current_thread_id != wake_thread_id){
            fp_this->Notify(wake_thread_id);
        }
    }
    m_wait_queue_lock.unlock();
}

void ConditionVariable::notify_all() {
    m_wait_queue_lock.lock();
    auto fp_this = FiberPool::GetThis();

    m_notify_all = true;
    while(!m_wait_queue.empty()){
        auto& [fiber, wake_thread_id] = m_wait_queue.front();
        fiber->SetStatus(Fiber::READY);

        fp_this->_run_internal(new Fiber::ptr(fiber), wake_thread_id);
    }
    m_wait_queue_lock.unlock();
    FiberPool::GetThis()->NotifyAll();
}
