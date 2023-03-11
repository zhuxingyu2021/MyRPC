#ifndef MYRPC_FIBER_H
#define MYRPC_FIBER_H

#include <memory>
#include <functional>

#include "noncopyable.h"

namespace MyRPC {
    class Fiber : public NonCopyable, public std::enable_shared_from_this<Fiber>{
    public:
        using ptr = std::shared_ptr<Fiber>;
        using unique_ptr = std::unique_ptr<Fiber>;

        const size_t init_stack_size = 8192;

        enum status{
            READY = 1,
            EXEC = 2,
            BLOCKED = 3,
            TERMINAL = 4,
            ERROR
        };

        /**
         * @param[in] func 协程中运行的函数
         */
        Fiber(const std::function<void()>& func);
        Fiber(std::function<void()>&& func);
        ~Fiber();

        /**
         * @brief 对于当前协程，让出CPU控制权，必须由协程调用
         * @param return_value 返回值
         */
        static void Suspend(int64_t return_value = 0);

        /**
         *@brief 阻塞当前协程，直至被IO事件唤醒，必须由协程调用
         *@param return_value 返回值
         *@note 该方法必须由协程池中的协程调用，否则会永远阻塞
         */
         static void Block(int64_t return_value = 0);

        /**
         * @brief 对于当前协程，停止执行，必须由协程调用
         * @param return_value 返回值
         */
         static void Exit(int64_t return_value = 0);

        /**
         * @brief 恢复协程的CPU控制权
         * @return 协程的返回值
         */
        int64_t Resume();

        /**
         * @brief 重置协程的状态
         * @note 该方法不会发生上下文切换
         */
        void Reset();

        /**
         * @brief 获得协程的状态
         */
        status GetStatus(){return m_status;}

        /**
         * @brief 获得协程的ID
         */
        int64_t GetId(){return m_fiber_id;}

        /**
         * @brief 获得当前协程状态，必须由协程调用
         */
         static status GetCurrentStatus();

        /**
         * @brief 获得当前协程ID，必须由协程调用
         */
         static int64_t GetCurrentId();

         static Fiber::ptr GetSharedFromThis();

         /**
          * @brief 获得栈大小，必须由协程调用
          */
         static size_t GetStacksize();

         /**
          * @brief 获得栈剩余空间的大小，必须由协程调用
          */
         static size_t GetStackFreeSize();

         /**
          * @brief 将栈空间扩充到两倍，必须由协程调用
          * @return 返回true表示成功，false表示失败
          */
         static bool ExtendStackCapacity();

    private:
        // 协程id
        int64_t m_fiber_id = 0;
        // 当前执行状态
        status m_status;
        // 需要执行的函数
        std::function<void()> m_func;
        // 协程栈
        char* m_stack = nullptr;
        size_t m_stack_size;

        // 上下文
        // 仅需要保存callee-saved寄存器

#if defined(__x86_64__) || defined(_M_X64)
        // x86_64: rsp, rbp, rbx, r12-r15, xmm6-xmm15
        // m_ctx内存布局：
        // 0-27: 子协程 rsp rbp r12 r13 r14 r15 rbx (preserved) xmm6 xmm7 xmm8 xmm9 xmm10 xmm11 xmm12 xmm13 xmm14 xmm15
        // 28-55: 主协程 rsp rbp r12 r13 r14 r15 rbx (preserved) xmm6 xmm7 xmm8 xmm9 xmm10 xmm11 xmm12 xmm13 xmm14 xmm15
        void* m_ctx[56] __attribute__((aligned (16)));
#elif defined(__aarch64__)
        // arm64: sp x19-x29 d8-d15 x30(return address)
        // m_ctx内存布局：
        // 0-21: 子协程 (x19 x20) (x21 x22) (x23 x24) (x25 x26) (x27 x28) (x29 x30) (sp x30(return_address)) (d8 d9) (d10 d11) (d12 d13) (d14 d15)
        // 22-43: 主协程 (x19 x20) (x21 x22) (x23 x24) (x25 x26) (x27 x28) (x29 x30) (sp x30(return_address)) (d8 d9) (d10 d11) (d12 d13) (d14 d15)
        void* m_ctx[44] __attribute__((aligned (16)));
#endif
        void _init_stack_and_ctx();

        // 协程的主函数
        static void _main_func();

    };

}

#endif //MYRPC_FIBER_H
