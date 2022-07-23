#ifndef MYRPC_EVENTMANAGER_H
#define MYRPC_EVENTMANAGER_H

#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <functional>
#include "fiber.h"

namespace MyRPC{

    class EventManager : std::enable_shared_from_this<EventManager> {
    public:
        using ptr = std::shared_ptr<EventManager>;
        static const int MAX_EVENTS = 300;
        static const int TIMEOUT = 1000;

        EventManager();
        ~EventManager();

        enum EventType{
            READ = EPOLLIN,
            WRITE = EPOLLOUT,
            READ_WRITE = EPOLLIN|EPOLLOUT
        };

        /**
         * @brief 添加IO事件，该方法必须由协程调用
         * @param fd[in] 文件描述符
         * @param event[in] IO事件类型
         * @return 0表示成功, -1表示失败
         */
        int AddIOEvent(int fd, EventType event);

        /**
         * @brief 将IO事件与函数绑定，当IO事件到来，执行相应函数
         * @param fd[in] 文件描述符
         * @param event[in] IO事件类型
         * @param func[in] IO事件完成之后，执行的函数
         * @note 函数function内部不能有阻塞系统调用
         * @return 0表示成功, -1表示失败
         */
        int AddIOFunc(int fd, EventType event, std::function<void()> func);

        /**
         * @brief 处理Epoll事件
         */
        void WaitEvent();

        /**
         * @brief 获得事件数量
         */
         int GetNumEvents(){return event_count;}

    protected:
        // 恢复执行协程ID为fiber_id的协程
        // Note: 该方法没有实现，必须被子类重写
        virtual void resume(int64_t fiber_id);

    private:
        int epoll_fd;
        int event_count = 0;
        epoll_event _events[MAX_EVENTS];

        // fd -> (Fiber Id, Event)
        // 可以根据文件描述符查到谁添加了这个IO事件，以及IO事件的事件类型
        std::unordered_map<int, std::pair<int64_t ,EventType>> _fd_event_map;

        // fd -> function
        // 可以根据文件描述符查到AddIOFunc添加的函数
        std::unordered_map<int, std::function<void()>> _fd_func_map;
    };

}


#endif //MYRPC_EVENTMANAGER_H
