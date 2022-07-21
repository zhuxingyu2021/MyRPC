#include "eventmanager.h"
#include "macro.h"
#include <cstring>
#include "fiber.h"

using namespace myrpc;

EventManager::EventManager() {
    // 初始化epoll
    epoll_fd = epoll_create(5000);
    MYRPC_SYS_ASSERT(epoll_fd != -1);
}

void EventManager::AddIOEvent(int fd, EventType event) {
    epoll_event event_epoll; // epoll_ctl 的4个参数
    memset(&event_epoll, 0, sizeof(epoll_event));

    int op; // epoll_ctl 的第二个参数

    // 查找fd对应的event是否已存在
    auto iter = _fd_event_map.find(fd);
    if(iter != _fd_event_map.end())
    {
        // event已存在，则修改event
        EventType new_event = (EventType)(iter->second.second | event);
        _fd_event_map[fd] = std::make_pair(Fiber::GetThis(),new_event);

        event_epoll.events = EPOLLET | new_event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // event不存在，创建event
        _fd_event_map[fd] = std::make_pair(Fiber::GetThis(),event);
        event_epoll.events = EPOLLET | event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
        ++event_count;
    }

    // 调用epoll_ctl
    MYRPC_SYS_ASSERT(epoll_ctl(epoll_fd, op, fd, &event_epoll)==0);
}

void EventManager::AddIOFunc(int fd, EventType event, std::function<void()> func) {
    epoll_event event_epoll; // epoll_ctl 的4个参数
    memset(&event_epoll, 0, sizeof(epoll_event));

    int op; // epoll_ctl 的第二个参数

    // 查找fd对应的event是否已存在
    auto iter = _fd_event_map.find(fd);
    if(iter != _fd_event_map.end())
    {
        // event已存在，则修改event
        EventType new_event = (EventType)(iter->second.second | event);
        _fd_event_map[fd] = std::make_pair(nullptr,new_event);

        event_epoll.events = EPOLLET | new_event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // event不存在，创建event
        _fd_event_map[fd] = std::make_pair(nullptr,event);
        event_epoll.events = EPOLLET | event;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
        ++event_count;
    }
    // 添加事件处理函数
    _fd_func_map[fd] = func;

    // 调用epoll_ctl
    MYRPC_SYS_ASSERT(epoll_ctl(epoll_fd, op, fd, &event_epoll)==0);
}

void EventManager::WaitEvent() {
    auto n = epoll_wait(epoll_fd, _events, MAX_EVENTS, TIMEOUT);
    MYRPC_SYS_ASSERT(n>=0);
    for(int i=0;i<n;i++){
        auto now_event = _events[i].events;
        auto fd = _events[i].data.fd;

        auto fiber_event = _fd_event_map[fd];
        auto fiber = fiber_event.first;
        auto set_event = fiber_event.second;

        if(fiber == nullptr){ // 函数事件
            (_fd_func_map[fd])();
            continue;
        }

        if (now_event & (EPOLLERR | EPOLLHUP)){
            now_event |= ((EPOLLIN | EPOLLOUT) & set_event);
        }
        int now_rw_event = now_event & (EPOLLIN | EPOLLOUT);
        if(now_rw_event & set_event == 0) continue;

        // 获取还未触发的event，并重新注册。若event全部被触发，则删除。
        int left_event = set_event & (~now_rw_event);
        int op = left_event?EPOLL_CTL_MOD: EPOLL_CTL_DEL;
        if(!left_event) { // 若event全部被触发，则删除相应的event
            --event_count;
            _fd_event_map.erase(fd);
        }
        _events[i].events = left_event | EPOLLET;

        MYRPC_SYS_ASSERT(epoll_ctl(epoll_fd, op, fd, &_events[i])==0);

        if((now_rw_event & READ) | (now_rw_event & WRITE)){
            // 恢复协程执行
            fiber->Resume();
        }
    }
}
