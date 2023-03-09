#ifndef MYRPC_SPINLOCK_H
#define MYRPC_SPINLOCK_H

#include <atomic>

#include "noncopyable.h"

namespace MyRPC {
/**
 * @brief 自旋锁
 */
    class SpinLock : public NonCopyable {
    public:
        void lock() { while (m_lock.test_and_set(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(_M_X64)
            asm volatile("pause;");
#elif defined(__aarch64__)
            asm volatile("yield;");
#endif
        } }

        void unlock() { m_lock.clear(std::memory_order_release); }

        bool tryLock() { return !m_lock.test_and_set(std::memory_order_acquire); }

    private:
        std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
    };
}

#endif //MYRPC_SPINLOCK_H
