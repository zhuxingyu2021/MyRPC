#ifndef MYRPC_FIBER_H
#define MYRPC_FIBER_H

#include <memory>
#include <functional>
#include <boost/coroutine2/coroutine.hpp>

#include "noncopyable.h"

namespace MyRPC {
    class Fiber : public NonCopyable, public std::enable_shared_from_this<Fiber>{
    public:
        using ptr = std::shared_ptr<Fiber>;
        using unique_ptr = std::unique_ptr<Fiber>;
        using pull_type = boost::coroutines2::coroutine<int64_t>::pull_type;
        using push_type = boost::coroutines2::coroutine<int64_t>::push_type;

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
         * @breif 获得当前协程ID，必须由协程调用
         */
         static int64_t GetCurrentId();

         static Fiber::ptr GetSharedFromThis();

    private:
        // 协程id
        int64_t m_fiber_id = 0;
        // 当前执行状态
        status m_status;
        // 需要执行的函数
        std::function<void()> m_func;
        // 协程，使用boost coroutine2实现
        pull_type* m_func_pull_type = nullptr;
        push_type* m_func_push_type = nullptr;

        // 协程的主函数
        static void Main(push_type&);

    };

}

#endif //MYRPC_FIBER_H
