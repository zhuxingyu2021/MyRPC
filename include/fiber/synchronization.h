#ifndef MYRPC_SYNCHRONIZATION_H
#define MYRPC_SYNCHRONIZATION_H

#include <atomic>
#include <unistd.h>

#include "noncopyable.h"
#include "macro.h"
#include "spinlock.h"

#include "macro.h"

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
#include "fiber/fiber.h"
#include "fiber/fiber_pool.h"
#endif

namespace MyRPC{
    namespace FiberSync {
        /**
         * @brief 协程级互斥锁
         */
        class Mutex : public NonCopyable {
        public:
            Mutex();
            ~Mutex();

            void lock() {
                ++m_waiter;
                if(!m_lock.test_and_set(std::memory_order_acquire)){ // trylock失败
                    // 在成功获得锁之前阻塞自己
                    do{
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
                        Logger::debug("Thread: {}, Fiber: {} is failed to acquire a lock, block event_fd: {}", FiberPool::GetCurrentThreadId(),
                                      Fiber::GetCurrentId(), m_event_fd);
#endif
                        uint64_t val;
                        read(m_event_fd, &val, sizeof(val));
                    }while(m_lock.test_and_set(std::memory_order_acquire));
                }
                --m_waiter;
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
                Logger::debug("Thread: {}, Fiber: {} has acquired a lock", FiberPool::GetCurrentThreadId(),
                              Fiber::GetCurrentId());
#endif
            }
            void unlock() {
                m_lock.clear(std::memory_order_release);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
                Logger::debug("Thread: {}, Fiber: {} has released a lock", FiberPool::GetCurrentThreadId(),
                              Fiber::GetCurrentId());
#endif
                if(m_waiter > 0){ // 如果有协程阻塞在锁上，那么唤醒该协程
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
                    Logger::debug("Thread: {}, Fiber: {} is trying to wake up blocked fiber. Waiter count:{}, wakeup event_fd: {}", FiberPool::GetCurrentThreadId(),
                                  Fiber::GetCurrentId(), m_waiter.load(), m_event_fd);
#endif
                    auto tmp = enable_hook;
                    enable_hook = false;

                    uint64_t val = 1;
                    MYRPC_SYS_ASSERT(write(m_event_fd, &val, sizeof(uint64_t)) == sizeof(uint64_t));

                    enable_hook = tmp;
                }
            }
            bool try_lock() { return m_lock.test_and_set(std::memory_order_acquire); }

        friend class RWMutex;
        private:
            std::atomic_flag m_lock = ATOMIC_FLAG_INIT;

            std::atomic<int> m_waiter = 0;

            int m_event_fd;

            inline void _block(){ // 阻塞自己
                uint64_t val;
                read(m_event_fd, &val, sizeof(val));
            }
        };

        /*
         * @brief 协程级读写锁
         */
        class RWMutex{
        public:
            void lock() {
                m_lock.lock();
                ++m_lock_owner_writer;
            }
            void unlock() {
                --m_lock_owner_writer;
                m_lock.unlock();
            }

            void lock_shared(){
                while(m_shared_lock.test_and_set(std::memory_order_acquire)){
                    // 获得读锁失败
                    while(m_lock_owner_writer > 0){ // 如果有写者持有锁
                        // 有一个写者在持有锁，同时还有个读者在请求锁
                        ++m_lock.m_waiter;
                        if(m_lock_owner_writer > 0) m_lock._block();
                        --m_lock.m_waiter;
                    }
                }
                if(++m_reader == 1){
                    m_lock.lock();
                }
                m_shared_lock.clear(std::memory_order_release);
            }
            void unlock_shared(){
                while(m_shared_lock.test_and_set(std::memory_order_acquire));
                if(--m_reader == 0){
                    m_lock.unlock();
                }
                m_shared_lock.clear(std::memory_order_release);
            }
        private:
            Mutex m_lock;

            std::atomic_flag m_shared_lock = ATOMIC_FLAG_INIT;

            int m_reader = 0;
            volatile uint8_t m_lock_owner_writer = 0; // 持有锁的writer数量
        };

        // TODO: Semaphore, ConditionVariable
    }
}

#endif //MYRPC_SYNCHRONIZATION_H
