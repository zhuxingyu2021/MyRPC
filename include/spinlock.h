#ifndef MYRPC_SPINLOCK_H
#define MYRPC_SPINLOCK_H

#include <atomic>

class SpinLock{
public:
    void lock() { while(m_lock.test_and_set(std::memory_order_acquire)); }
    void unlock(){m_lock.clear(std::memory_order_release);}

    SpinLock() = default;
    ~SpinLock() = default;
    SpinLock(const SpinLock& rhs) = delete;
    SpinLock(SpinLock&& rhs) = delete;
    SpinLock& operator=(const SpinLock& rhs) = delete;
    SpinLock& operator=(SpinLock&& rhs) = delete;

private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
};

#endif //MYRPC_SPINLOCK_H
