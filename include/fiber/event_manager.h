#ifndef MYRPC_EVENT_MANAGER_H
#define MYRPC_EVENT_MANAGER_H

#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <array>
#include "fiber.h"

#include "noncopyable.h"

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

    protected:
        // 恢复执行协程ID为fiber_id的协程
        // Note: 该方法没有实现，必须被子类重写
        virtual void resume(int64_t fiber_id);

    private:
        int m_epoll_fd;
        epoll_event m_events[MAX_EVENTS];

        int m_notify_event_fd; // 用于从epoll_wait中唤醒

        // m_adder_map: fd -> (READ Fiber ID, WRITE Fiber ID)
        // m_adder_map: 可以根据文件描述符查到谁添加了读/写IO事件
        // 如果没有IO事件，那么对应的Fiber ID = 0
        // 如果是唤醒事件，READ Fiber ID < 0
        std::unordered_map<int, std::array<int64_t,2>> m_adder_map;
    };

}


#endif //MYRPC_EVENT_MANAGER_H
