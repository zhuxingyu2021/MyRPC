#ifndef MYRPC_FIBER_H
#define MYRPC_FIBER_H

#include <memory>
#include <functional>
#include <boost/coroutine2/coroutine.hpp>

namespace myrpc {
    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        using ptr = std::shared_ptr<Fiber>;
        using pull_type = boost::coroutines2::coroutine<int>::pull_type;
        using push_type = boost::coroutines2::coroutine<int>::push_type;

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
        Fiber(std::function<void()> func);
        ~Fiber();

        /**
         * @brief 对于当前协程，让出CPU控制权，必须由协程调用
         */
        static void Suspend();

        /**
         *@brief 阻塞当前协程，直至被事件唤醒，必须由协程调用
         *@note 该方法必须在协程池中调用，否则会永远阻塞
         */
         static void Block();

        /**
         * @brief 对于当前协程，停止执行，必须由协程调用
         */
         static void Exit();

        /**
         * @brief 恢复协程的CPU控制权
         */
        void Resume();

        /**
         * @brief 获得当前协程类的this指针，必须由协程调用
         */
         static Fiber* GetThis();

        /**
         * @brief 获得协程的状态
         */
        status GetStatus(){return _status;}

        /**
         * @brief 获得协程的ID
         */
        uint64_t GetId(){return fiber_id;}

        /**
         * @brief 获得当前协程状态，必须由协程调用
         */
         static status GetCurrentStatus();

        /**
         * @breif 获得当前协程ID，必须由协程调用
         */
         static uint64_t GetCurrentId();

    private:
        // 协程id
        uint64_t fiber_id = 0;
        // 当前执行状态
        status _status;
        // 需要执行的函数
        std::function<void()> _func;
        // 协程，使用boost coroutine2实现
        pull_type* func_pull_type = nullptr;
        push_type* func_push_type = nullptr;

        // 协程的主函数
        static void Main(push_type&);

    };

}

#endif //MYRPC_FIBER_H
