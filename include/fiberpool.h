#ifndef MYRPC_FIBERPOOL_H
#define MYRPC_FIBERPOOL_H

#include "fiber.h"
#include <future>
#include <list>
#include <unordered_map>
#include <atomic>

#include "macro.h"
#include "spinlock.h"
#include "eventmanager.h"

namespace MyRPC{
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

            Task(std::function<void()> func, int tid, bool _circular):fiber(new Fiber(func)), thread_id(tid),
                circular(_circular){}
            Task() = delete;

            Fiber::ptr fiber; // 协程指针
            int thread_id; // 线程ID
            std::atomic<bool> circular ; // 任务是否循环执行

            std::atomic<bool> stopped {false}; // 任务是否已停止
            std::atomic<uint32_t> circular_count {0}; //循环执行计数
        };
        std::list<Task::ptr> _tasks; // 任务队列
        ThreadLevelSpinLock _tasks_lock; // 任务队列锁
    public:

        class FiberController{
        public:
            friend FiberPool;
            FiberController(Task::ptr _ptr): _task_ptr(_ptr){}

            /**
             * @brief Join协程和主线程
             * @note 若协程不可被Join，那么等待至协程能被Join为止
             */
            void Join() {
                while(!_task_ptr->stopped){
#ifndef MYRPC_DEBUG_SYS_CALL
                    MYRPC_SYS_ASSERT(sched_yield()==0);
#endif
                }
            }

            /**
             * @brief 获取协程ID
             * @return 协程ID
             */
            int64_t GetId(){return _task_ptr->fiber->GetId();}

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
        static EventManager::ptr GetEventManager();

    private:
        int n_threads; // 线程数量

        class ThreadContext:public EventManager{
        public:
            using ptr = std::shared_ptr<ThreadContext>;
            ThreadContext():EventManager(){}

            // Fiber Id -> Task
            // 线程的任务队列
            std::unordered_map<int64_t, Task::ptr> my_tasks;
        protected:
            void resume(int64_t fiber_id) override;
        };

        std::vector<ThreadContext::ptr> _threads_context_ptr;

        std::vector<std::shared_future<int>> _threads_future;

        // 协程池主循环
        int MainLoop(int thread_id);

        // 当有新的任务到来时，可以通过event_fd来唤醒协程池中的主协程
        int event_fd;

        // 判断协程池是否有线程在运行
        std::atomic<bool> running {false};

        // 用以强制关闭协程池
        std::atomic<bool> stopping{false};
    };

}
#endif //MYRPC_FIBERPOOL_H
