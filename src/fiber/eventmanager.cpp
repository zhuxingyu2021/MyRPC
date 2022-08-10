#include "fiber/eventmanager.h"
#include "macro.h"
#include <cstring>
#include "fiber/fiber.h"
#include "fiber/hookio.h"

using namespace MyRPC;

EventManager::EventManager() {
    // 初始化epoll
    m_epoll_fd = epoll_create(1);
    MYRPC_SYS_ASSERT(m_epoll_fd != -1);
}

EventManager::~EventManager() {
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
        // event已存在，则修改event
        EventType new_event = (EventType)(iter->second.second | event);
        m_fd_event_map[fd] = std::make_pair(Fiber::GetCurrentId(), new_event);

        event_epoll.events = EPOLLET | new_event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // event不存在，创建event
        m_fd_event_map[fd] = std::make_pair(Fiber::GetCurrentId(), event);
        event_epoll.events = EPOLLET | event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
        ++m_event_count;
    }

    // 调用epoll_ctl
    return epoll_ctl(m_epoll_fd, op, fd, &event_epoll);
}

int EventManager::AddIOFunc(int fd, EventType event, std::function<void()> func) {
    epoll_event event_epoll; // epoll_ctl 的4个参数
    memset(&event_epoll, 0, sizeof(epoll_event));

    int op; // epoll_ctl 的第二个参数

    // 查找fd对应的event是否已存在
    auto iter = m_fd_event_map.find(fd);
    if(iter != m_fd_event_map.end())
    {
        // event已存在，则修改event
        EventType new_event = (EventType)(iter->second.second | event);
        m_fd_event_map[fd] = std::make_pair(-1, new_event);

        event_epoll.events = EPOLLET | new_event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // event不存在，创建event
        m_fd_event_map[fd] = std::make_pair(-1, event);
        event_epoll.events = EPOLLET | event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
        ++m_event_count;
    }
    // 添加事件处理函数
    m_fd_func_map[fd] = func;

    // 调用epoll_ctl
    return epoll_ctl(m_epoll_fd, op, fd, &event_epoll);
}

void EventManager::WaitEvent() {
    auto n = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS, TIMEOUT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
    Logger::debug("epoll_wait() returned {}", n);
#endif
    for(int i=0;i<n;i++){
        auto happened_event = m_events[i].events;
        auto fd = m_events[i].data.fd;

        auto[fiber_id, fiber_event] = m_fd_event_map[fd];

        if(fiber_id == -1){ // 函数事件
            auto tmp = enable_hook;
            enable_hook = false;
            (m_fd_func_map[fd])();
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
    Logger::critical("No Implementation Error! In file {}, line: {}", __FILE__, __LINE__);
    exit(-1);
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
            m_fd_event_map[fd] = std::make_pair(iter->second.first, new_event);
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
