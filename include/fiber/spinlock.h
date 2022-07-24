#ifndef MYRPC_SPINLOCK_H
#define MYRPC_SPINLOCK_H

#include <atomic>

/**
 * @brief 线程级自旋锁
 * @note 可以用于线程间的同步，但是不能用于协程间的同步。因为如果获得锁的协程和请求锁的协程在同一个线程中，会造成死锁。
 * @note 所有线程级的锁，包括标准库、boost库等线程库提供的锁，都不能用于协程间的同步，因为很可能出现死锁。
 */
class ThreadLevelSpinLock{
public:
    void lock() { while(m_lock.test_and_set(std::memory_order_acquire)); }
    void unlock(){m_lock.clear(std::memory_order_release);}

    ThreadLevelSpinLock() = default;
    ~ThreadLevelSpinLock() = default;
    ThreadLevelSpinLock(const ThreadLevelSpinLock& rhs) = delete;
    ThreadLevelSpinLock(ThreadLevelSpinLock&& rhs) = delete;
    ThreadLevelSpinLock& operator=(const ThreadLevelSpinLock& rhs) = delete;
    ThreadLevelSpinLock& operator=(ThreadLevelSpinLock&& rhs) = delete;

private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
};

#endif //MYRPC_SPINLOCK_H
