#ifndef MYRPC_EVENT_MANAGER_H
#define MYRPC_EVENT_MANAGER_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <array>
#include "fiber.h"

#include "noncopyable.h"
#include <boost/lockfree/spsc_queue.hpp>

#define MYRPC_MAXTASK_PER_THREAD 512

namespace MyRPC{

    class EventManager: public NonCopyable{
    public:
        static const int MAX_EVENTS = 300;
        static const int TIME_OUT = 5000;

        EventManager();
        ~EventManager();

        enum EventType{
            READ = 0,
            WRITE = 1
        };

        /**
         * @brief 添加IO事件，该方法必须由协程调用
         * @param fd[in] 文件描述符
         * @param event[in] IO事件类型
         * @return 0表示成功, -1表示失败
         */
        int AddIOEvent(int fd, EventType event);

        int AddWakeupEventfd(int fd);

        /**
         * @brief 删除IO事件，该方法必须由协程调用
         * @param fd[in] 文件描述符
         * @param event[in] IO事件类型
         * @return 0表示成功, -1表示失败。如果fd不存在，也会返回0
         */
        int RemoveIOEvent(int fd, EventType event);

        /**
         * @brief 检查是否在等待IO事件的发生
         * @param fd[in] 文件描述符
         * @param event[in] IO事件类型
         */
        bool IsExistIOEvent(int fd, EventType event) const;

        void Notify();

        /**
         * @brief 处理Epoll事件
         */
        void WaitEvent(int thread_id);

        // 线程的任务队列
        boost::lockfree::spsc_queue<Fiber::ptr> m_task_queue;

    private:
        int m_epoll_fd;

        int m_notify_event_fd; // 用于从epoll_wait中唤醒

        // m_adder_map: fd -> (READ Fiber, WRITE Fiber)
        // m_adder_map: 可以根据文件描述符查到谁添加了读/写IO事件
        std::unordered_map<int, std::array<Fiber::ptr,2>> m_adder_map;
        std::unordered_set<int> m_wake_up_set;
    };

}


#endif //MYRPC_EVENT_MANAGER_H
