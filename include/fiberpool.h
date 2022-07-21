#ifndef MYRPC_FIBERPOOL_H
#define MYRPC_FIBERPOOL_H

#include "fiber.h"
#include <thread>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <atomic>

#include "eventmanager.h"

namespace myrpc{
    class FiberPool: public std::enable_shared_from_this<FiberPool> {
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
         * @brief 唤醒正在等待事件的线程
         */
        void NotifyAll();

    private:
        struct Task{
            using ptr = std::shared_ptr<Task>;
            Fiber::ptr fiber = nullptr; // 协程指针
            int thread_id = -1; // 线程ID
            volatile bool circular = false; // 任务是否循环执行
            volatile bool joinable = false; // 任务是否可以被join

            uint32_t circular_count = 0; //循环执行计数
        };
        std::list<Task::ptr> _tasks; // 任务队列
        mutable std::shared_mutex _tasks_mutex; // 任务队列的互斥锁
    public:

        class FiberController{
        public:
            friend FiberPool;
            FiberController(Task::ptr _ptr): _task_ptr(_ptr){}

            /**
             * @brief 判断协程是否可以被Join（已执行完成）
             * @return true表示协程可以被Join（已执行完成）， false表示协程不能被Join（未执行完成）
             */
            bool Joinable() { return _task_ptr->joinable; }

            /**
             * @brief Join协程和主线程
             * @note 若协程不可被Join，那么等待至协程能被Join为止
             */
            void Join() {
                while(_task_ptr->joinable){
                    usleep(1000);
                }
            }

            /**
             * @brief 获得当前任务已循环执行的次数
             * @return 返回循环执行计数，表示任务已被执行了多少次
             */
            int32_t GetCircularCount(){return _task_ptr->circular_count;}

            /**
             * @brief 将任务设置成不能循环执行
             */
            void UnsetCircular(){_task_ptr->circular=false;}
        private:
            Task::ptr _task_ptr;
        };

        /**
         * 运行任务func
         * @param func 任务对应的函数
         * @param thread_id 将任务指定给线程thread_id执行。若thread_id被设置为-1，表示将任务分配给任意线程执行
         * @param circular true表示任务会被循环执行，false表示任务只执行一次
         * @return
         */
        FiberController Run(std::function<void()> func, int thread_id = -1, bool circular = false);

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
        int n_threads; // 线程数量

        struct ThreadContext{
        public:
            ThreadContext():_manager(){}

            EventManager _manager; // 线程的事件管理器

            std::list<Task::ptr> my_tasks; // 线程的任务队列
        };

        std::vector<ThreadContext> _threads_context;

        std::vector<std::thread*> _threads_ptr;

        // 协程池主循环
        void MainLoop(int thread_id);

        // 当有新的任务到来时，可以往pipe中写数据来唤醒线程池中的线程
        int pipe_fd[2];

        // 用以强制关闭协程池
        volatile bool stopping;
    };

}
#endif //MYRPC_FIBERPOOL_H
