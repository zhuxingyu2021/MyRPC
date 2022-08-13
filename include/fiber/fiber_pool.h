#ifndef MYRPC_FIBER_POOL_H
#define MYRPC_FIBER_POOL_H

#include "fiber/fiber.h"
#include <future>
#include <list>
#include <unordered_map>
#include <atomic>

#include "macro.h"
#include "spinlock.h"
#include "fiber/event_manager.h"

namespace MyRPC{
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

    private:
        struct Task{
            using ptr = std::shared_ptr<Task>;

            template<class Func>
            Task(Func&& func, int tid):fiber(new Fiber(std::forward<Func>(func))), thread_id(tid){}
            Task() = delete;
            ~Task() = default;

            Fiber::unique_ptr fiber; // 协程指针
            int thread_id; // 线程ID

            std::atomic<bool> stopped {false}; // 任务是否已停止

            int event_fd;
        };
        std::list<Task::ptr> m_tasks; // 任务队列
        SpinLock m_tasks_lock; // 任务队列锁
    public:

        class FiberController{
        public:
            using ptr = std::shared_ptr<FiberController>;

            friend FiberPool;
            FiberController(Task::ptr _ptr): m_task_ptr(_ptr){}

            /**
             * @brief 判断协程是否已停止
             * @return 若协程已停止，返回true，否则返回false
             */
            bool IsStopped() {
                return m_task_ptr->stopped;
            }

            /**
             * @brief Join协程和主线程
             * @note 若协程不可被Join，那么等待至协程能被Join为止
             */
            void Join();

            /**
             * @brief 获取协程ID
             * @return 协程ID
             */
            int64_t GetId(){return m_task_ptr->fiber->GetId();}
        private:
            Task::ptr m_task_ptr;
        };

        /**
         * 运行任务func
         * @param func 任务对应的函数
         * @param thread_id 将任务指定给线程thread_id执行。若thread_id被设置为-1，表示将任务分配给任意线程执行
         * @return
         */
        template<class Func>
        FiberController::ptr Run(Func&& func, int thread_id = -1){
            Task::ptr ptr(new Task(std::forward<Func>(func), thread_id));
            {
                std::lock_guard<SpinLock> lock(m_tasks_lock);
                m_tasks.push_back(ptr);
                ++m_tasks_cnt;
            }
            NotifyAll();
            return std::make_shared<FiberPool::FiberController>(ptr);
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

        class ThreadContext:public EventManager{
        public:
            ThreadContext():EventManager(){}

            // Fiber Id -> Task
            // 线程的任务队列
            std::unordered_map<int64_t, Task::ptr> my_tasks;
        protected:
            void resume(int64_t fiber_id) override;
        };

        std::vector<ThreadContext*> m_threads_context_ptr;

        std::vector<std::shared_future<int>> m_threads_future;

        // 协程池主循环
        int MainLoop(int thread_id);

        // 当有新的任务到来时，可以通过event_fd来唤醒协程池中的所有主协程
        int m_global_event_fd;

        // 判断协程池是否有线程在运行
        std::atomic<bool> m_running {false};

        // 用以强制关闭协程池
        std::atomic<bool> m_stopping{false};

        std::atomic<int> m_tasks_cnt {0}; // 当前任务数量
    };

}
#endif //MYRPC_FIBER_POOL_H
