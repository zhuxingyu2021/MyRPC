#ifndef MYRPC_FIBERPOOL_H
#define MYRPC_FIBERPOOL_H

#include "fiber.h"
#include <thread>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "eventmanager.h"

namespace myrpc{
    class FiberPool: public std::enable_shared_from_this<FiberPool> {
    public:
        using ptr = std::shared_ptr<FiberPool>;

        FiberPool(int thread_num);
        virtual ~FiberPool();

        void Start();
        void Stop();

        void NotifyAll();

    private:
        struct Task{
            using ptr = std::shared_ptr<Task>;
            Fiber::ptr fiber = nullptr; // 协程指针
            int thread_id = -1; // 线程ID
            volatile bool circular = false; // 任务是否循环执行
            volatile bool joinable = false; // 任务是否可以被join
        };
        std::list<Task::ptr> _tasks; // 任务队列
        mutable std::shared_mutex _tasks_mutex; // 任务队列的互斥锁
    public:

        class FiberController{
        public:
            friend FiberPool;
            FiberController(Task::ptr _ptr): _task_ptr(_ptr){}
            bool Joinable() { return _task_ptr->joinable; }
            void Join() {
                while(_task_ptr->joinable){
                    usleep(1000);
                }
            }
            void UnsetCircular(){_task_ptr->circular=false;}
        private:
            Task::ptr _task_ptr;
        };

        FiberController Run(std::function<void()> func, int thread_id = -1, bool circular = false);

        static int GetCurrentThreadId();
        static FiberPool* GetThis();
        static EventManager* GetEventManager();

        enum event{
            READ = EPOLLIN,
            WRITE = EPOLLOUT
        };


    private:
        int n_threads; // 线程数量

        struct ThreadContext{
        public:
            ThreadContext():_manager(){}

            EventManager _manager;

            std::list<Task::ptr> my_tasks;
        };

        std::vector<ThreadContext> _threads_context;

        std::vector<std::thread*> _threads_ptr;

        void MainLoop(int thread_id);

        // 当有新的任务到来时，可以往pipe中写数据来唤醒线程池中的线程
        int pipe_fd[2];

        volatile bool stopping;
    };

}
#endif //MYRPC_FIBERPOOL_H
