#ifndef MYRPC_FIBER_POOL_H
#define MYRPC_FIBER_POOL_H

#include "fiber/fiber.h"
#include <future>
#include <list>
#include <unordered_map>
#include <atomic>

#include "debug.h"
#include "spinlock.h"
#include "fiber/event_manager.h"

namespace MyRPC{
    namespace FiberSync{
        class Mutex;
        class ConditionVariable;
    }

    class FiberPool: public NonCopyable {
    public:
        using ptr = std::shared_ptr<FiberPool>;

        /**
         * @param thread_num[in] 线程数量
         */
        FiberPool(int thread_num);
        virtual ~FiberPool();

        /**
         * @brief 启动协程池
         */
        void Start();

        /**
         * @brief 停止协程池
         * @note 协程池中的协程会被强制关闭，因此可能出现部分协程未执行完成的情况
         */
        void Stop();

        /**
         * @brief 唤醒正在等待事件的线程（指定线程）
         * @param thread_id 需要唤醒的线程id
         */
        void Notify(int thread_id){
            m_threads_context_ptr[thread_id]->Notify();
        }

        /**
         * @brief 唤醒正在等待事件的线程（所有线程）
         * @note 该方法一次唤醒线程池中的所有线程
         */
        void NotifyAll();

        /**
         * @brief 主线程等待协程池中的所有协程执行完毕
         */
        void Wait();

        /**
         * 运行任务func
         * @param func 任务对应的函数
         * @param thread_id 将任务指定给线程thread_id执行。若thread_id被设置为-1，表示将任务分配给任意线程执行
         * @return
         */
        template<class Func>
        std::pair<Fiber::ptr, int> Run(Func&& func, int thread_id = -1){
            if(thread_id == -1)
                thread_id = rand() % m_threads_num;

            Fiber::ptr* ptr = new Fiber::ptr(new Fiber(std::forward<Func>(func)));
            _run_internal(ptr, thread_id);
            ++m_tasks_cnt;
            Notify(thread_id);
            return std::make_pair(*ptr, thread_id);
        }

        /**
         * 获得当前线程Id，该方法只能由协程池中的线程调用
         * @return 线程Id
         */
        static int GetCurrentThreadId();

        /**
         * 获得当前的协程池指针，该方法只能由协程池中的线程调用
         * @return 当前的协程池指针
         */
        static FiberPool* GetThis();

        /**
         * 获得当前线程的事件管理器，该方法只能由协程池中的协程调用
         * @return 当前协程的事件管理器
         */
        static EventManager* GetEventManager();
    private:
        int m_threads_num; // 线程数量

        std::vector<EventManager*> m_threads_context_ptr;

        std::vector<std::shared_future<int>> m_threads_future;

        // 协程池主循环
        int _main_loop(int thread_id);

        // 当有新的任务到来时，可以通过event_fd来唤醒协程池中的所有主协程
        int m_global_event_fd;

        // 判断协程池是否有线程在运行
        std::atomic<bool> m_running {false};

        // 用以强制关闭协程池
        std::atomic<bool> m_stopping{false};

        std::atomic<int> m_tasks_cnt {0}; // 当前任务数量

        friend FiberSync::Mutex;
        friend FiberSync::ConditionVariable;
        void _run_internal(Fiber::ptr* ptr, int thread_id){
            if(!m_threads_context_ptr[thread_id]->m_task_queue.TryPush(ptr)){
                MYRPC_CRITIAL_ERROR("Task queue is full!");
            }
        }
    };

}
#endif //MYRPC_FIBER_POOL_H
