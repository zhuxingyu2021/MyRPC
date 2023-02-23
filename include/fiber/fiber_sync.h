#ifndef MYRPC_FIBER_SYNC_H
#define MYRPC_FIBER_SYNC_H

#include <atomic>
#include <unistd.h>
#include <set>
#include <queue>

#include "noncopyable.h"
#include "macro.h"
#include "spinlock.h"

#include "macro.h"

#include "fiber/fiber.h"

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
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

            void lock();

            void unlock();

            bool tryLock() {
                return !m_lock.test_and_set(std::memory_order_acquire);
            }

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_LOCK_LEVEL
            int64_t _debug_lock_owner = -10; // 锁持有者的Fiber id
#endif
        private:
            std::atomic_flag m_lock = ATOMIC_FLAG_INIT;

            std::atomic<int64_t> m_lock_id = {0};

            int m_mutex_id;
            std::queue<std::pair<int64_t, int>> m_wait_queue; // FiberID ThreadID
            SpinLock m_wait_queue_lock;
        };

        /*
         * @brief 协程级读写锁
         */
        class RWMutex{
        public:
            void lock() {
                write_lock.lock();
            }
            void unlock() {
                write_lock.unlock();
            }

            void lock_shared(){
                //read_lock.lock();
                while(!read_lock.tryLock()){
                    if(reader_blocked){ // 如果存在写操作，则阻塞直到获得读锁
                        read_lock.lock();
                        break;
                    } // 如果不存在写操作，则以自旋锁的方式获得读锁
                }
                if(++m_reader == 1){
                    // 存在线程读操作时，写加锁
                    if(!write_lock.tryLock()) {
                        reader_blocked = true;
                        write_lock.lock();
                        reader_blocked = false;
                    }
                }
                read_lock.unlock();
            }

            void unlock_shared(){
                //read_lock.lock();
                while(!read_lock.tryLock()); // 以自旋锁的方式获得读锁
                if(--m_reader == 0){
                    write_lock.unlock(); // 没有线程读操作时，释放写锁
                }
                read_lock.unlock();
            }
        private:
            Mutex write_lock;
            Mutex read_lock;

            int m_reader = 0;

            std::atomic<bool> reader_blocked = {false};
        };

        // TODO: Semaphore ConditionVariable

        template <class MutexType>
        class ConditionVariable : public NonCopyable{
        public:
            void wait(MutexType& mutex) {
                m_wait_queue_lock.lock();
                m_wait_queue.push(std::make_pair(Fiber::GetCurrentId(), FiberPool::GetCurrentThreadId()));

                do{
                    m_wait_queue_lock.unlock();
                    mutex.unlock();
                    Fiber::Suspend();
                    mutex.lock();
                    m_wait_queue_lock.lock();
                }while(m_wait_queue.front().first != Fiber::GetCurrentId());

                m_wait_queue.pop();
                m_wait_queue_lock.unlock();
            }
            void notify_one(){
                m_wait_queue_lock.lock();
                if(!m_wait_queue.empty()) {
                    FiberPool::GetThis()->Notify(m_wait_queue.front().second);
                }
                m_wait_queue_lock.unlock();
            }
            void notify_all(){
                //TODO Implementation
                MYRPC_NO_IMPLEMENTATION_ERROR();
            }

        private:
            std::queue<std::pair<int64_t, int>> m_wait_queue; // FiberID ThreadID
            SpinLock m_wait_queue_lock;
        };
    }
}

#endif //MYRPC_FIBER_SYNC_H
