#include "fiber/event_manager.h"
#include "macro.h"
#include <cstring>
#include <sys/fcntl.h>
#include "fiber/fiber.h"
#include "fiber/hook_io.h"

#include <sys/eventfd.h>

using namespace MyRPC;

EventManager::EventManager() {
    // 初始化epoll
    m_epoll_fd = epoll_create(1);
    MYRPC_SYS_ASSERT(m_epoll_fd != -1);

    // 初始化唤醒event_fd
    m_notify_event_fd = eventfd(0, O_NONBLOCK);
    MYRPC_SYS_ASSERT(m_notify_event_fd > 0);
    MYRPC_SYS_ASSERT(AddWakeupEventfd(m_notify_event_fd) == 0);
}

EventManager::~EventManager() {
    MYRPC_SYS_ASSERT(close(m_notify_event_fd) == 0);
    MYRPC_SYS_ASSERT(close(m_epoll_fd) == 0);
}

int EventManager::AddIOEvent(int fd, EventType event) {
    epoll_event event_epoll; // epoll_ctl 的第4个参数
    memset(&event_epoll, 0, sizeof(epoll_event));

    int op; // epoll_ctl 的第二个参数

    // 查找fd对应的event是否已存在
    auto iter = m_fd_event_map.find(fd);
    if(iter != m_fd_event_map.end())
    {
        assert(iter->second.first == Fiber::GetCurrentId());

        // event已存在，则修改event
        EventType new_event = (EventType)(iter->second.second | event);
        //m_fd_event_map[fd] = std::make_pair(Fiber::GetCurrentId(), new_event);
        m_fd_event_map.emplace(std::piecewise_construct,
                               std::forward_as_tuple(fd), std::forward_as_tuple(Fiber::GetCurrentId(), new_event));

        event_epoll.events = EPOLLET | new_event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // event不存在，创建event
        //m_fd_event_map[fd] = std::make_pair(Fiber::GetCurrentId(), event);
        m_fd_event_map.emplace(std::piecewise_construct,
                               std::forward_as_tuple(fd), std::forward_as_tuple(Fiber::GetCurrentId(), event));
        event_epoll.events = EPOLLET | event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
        ++m_event_count;
    }

    // 调用epoll_ctl
    return epoll_ctl(m_epoll_fd, op, fd, &event_epoll);
}

void EventManager::WaitEvent(int thread_id) {
    auto n = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS, TIME_OUT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_EPOLL_LEVEL
    Logger::debug("Thread: {}, epoll_wait() returned {}", thread_id ,n);
#endif
    for(int i=0;i<n;i++){
        auto happened_event = m_events[i].events;
        auto fd = m_events[i].data.fd;

        auto[fiber_id, fiber_event] = m_fd_event_map[fd];

        if(fiber_id == -1){ // eventfd唤醒
            auto tmp = enable_hook;
            enable_hook = false;

            uint64_t val;
            read(fd, &val, sizeof(val));

            enable_hook = tmp;
            continue;
        }

        if (happened_event & (EPOLLERR | EPOLLHUP)){
            happened_event |= ((EPOLLIN | EPOLLOUT) & fiber_event);
        }
        int now_rw_event = happened_event & (EPOLLIN | EPOLLOUT);
        if(now_rw_event & fiber_event == 0) continue;

        // 获取还未触发的event，并重新注册。若event全部被触发，则删除。
        int left_event = fiber_event & (~now_rw_event);
        int op = left_event?EPOLL_CTL_MOD: EPOLL_CTL_DEL;
        if(!left_event) { // 若event全部被触发，则删除相应的event
            --m_event_count;
            m_fd_event_map.erase(fd);
        }else{
            m_fd_event_map[fd].second = static_cast<EventType>(left_event);
        }
        m_events[i].events = left_event | EPOLLET;

        MYRPC_SYS_ASSERT(epoll_ctl(m_epoll_fd, op, fd, &m_events[i]) == 0);

        if((now_rw_event & READ) | (now_rw_event & WRITE)){
            // 恢复协程执行
            resume(fiber_id);
        }
    }
}

void EventManager::resume(int64_t fiber_id) {
    MYRPC_NO_IMPLEMENTATION_ERROR();
}

int EventManager::RemoveIOEvent(int fd, EventManager::EventType event) {
    // 查找fd对应的event是否已存在
    auto iter = m_fd_event_map.find(fd);
    if(iter == m_fd_event_map.end())
    {
        // event不存在，则返回
        return 0;
    }
    else{
        // event已存在
        EventType new_event = (EventType)(iter->second.second & ~event);
        struct epoll_event event_epoll; // epoll_ctl 的第4个参数
        memset(&event_epoll, 0, sizeof(epoll_event));
        event_epoll.data.fd = fd;
        event_epoll.events = EPOLLET | new_event;

        if(new_event == 0) {
            --m_event_count;
            m_fd_event_map.erase(fd);
            return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event_epoll);
        }else{
            //m_fd_event_map[fd] = std::make_pair(iter->second.first, new_event);
            (*iter).second.second = new_event;
            return epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event_epoll);
        }
    }
}

bool EventManager::IsExistIOEvent(int fd, EventManager::EventType event) const {
    // 查找fd对应的event是否已存在
    auto iter = m_fd_event_map.find(fd);
    if(iter == m_fd_event_map.end())
    {
        // event不存在，则返回
        return false;
    }else{
        return (iter->second.second & event) != 0;
    }
}

int EventManager::AddWakeupEventfd(int fd) {
    if(m_fd_event_map.find(fd) == m_fd_event_map.end()) {

        // 将Eventfd设置为非阻塞模式
        int flags;
        MYRPC_SYS_ASSERT((flags = fcntl(fd, F_GETFL, 0)) != -1);
        if (!(flags & O_NONBLOCK)) {
            flags |= O_NONBLOCK;
            MYRPC_SYS_ASSERT(fcntl(fd, F_SETFL, flags) == 0)
        }

        EventType eventfd_event = EventManager::READ; // 需要从eventfd读数据
        //m_fd_event_map[fd] = std::make_pair(-1, eventfd_event);
        m_fd_event_map.emplace(std::piecewise_construct,
                               std::forward_as_tuple(fd), std::forward_as_tuple(-1, eventfd_event));

        epoll_event event_epoll; // epoll_ctl 的4个参数
        memset(&event_epoll, 0, sizeof(epoll_event));
        event_epoll.data.fd = fd;
        event_epoll.events = EPOLLET | eventfd_event;

        // 调用epoll_ctl
        return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event_epoll);
    }
    return 0;
}

void EventManager::Notify() {
    auto tmp = enable_hook;
    enable_hook = false;
    uint64_t val = 1;
    MYRPC_SYS_ASSERT(write(m_notify_event_fd, &val, sizeof(uint64_t)) == sizeof(uint64_t));
    enable_hook = tmp;
}
