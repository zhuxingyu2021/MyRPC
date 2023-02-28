#include "fiber/event_manager.h"
#include "macro.h"
#include <cstring>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include "fiber/fiber.h"
#include "fiber/hook_io.h"

#include <sys/eventfd.h>

using namespace MyRPC;

EventManager::EventManager():m_task_queue(MYRPC_MAXTASK_PER_THREAD) {
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

    // 当前文件描述符对应的IO事件
    auto iter = m_adder_map.find(fd);
    if(iter != m_adder_map.end())
    {
        assert(iter->second[event] == 0);

        // event已存在，则修改event
        iter->second[event] = Fiber::GetSharedFromThis();

        event_epoll.events = EPOLLIN | EPOLLOUT;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_MOD;
    }
    else{
        // 目前当前文件描述符没有IO事件
        std::array<Fiber::ptr,2> tmp = {0};
        tmp[event] = Fiber::GetSharedFromThis();
        m_adder_map.emplace(fd, tmp);

        int event_in_epoll = (event == READ) ? EPOLLIN : EPOLLOUT;
        event_epoll.events = event_in_epoll;
        event_epoll.data.fd = fd;

        op = EPOLL_CTL_ADD;
    }

    // 调用epoll_ctl
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_EPOLL_LEVEL
    auto _e = event_epoll.events;
    Logger::debug("Fiber: {}, call epoll_ctl({}, 0x{:x}, {}, ...), epoll events:0x{:x}", Fiber::GetCurrentId(), m_epoll_fd, op, fd, _e);
#endif
    return epoll_ctl(m_epoll_fd, op, fd, &event_epoll);
}

void EventManager::WaitEvent(int thread_id) {
    epoll_event m_events[MAX_EVENTS];

    auto n = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS, TIME_OUT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_EPOLL_LEVEL
    Logger::debug("Thread: {}, epoll_wait() returned {}", thread_id ,n);
#endif
    for(int i=0;i<n;i++){
        auto happened_event = m_events[i].events;
        auto fd = m_events[i].data.fd;

        if(m_wake_up_set.count(fd) > 0){ // eventfd唤醒
            auto tmp = enable_hook;
            enable_hook = false;

            uint64_t val;
            read(fd, &val, sizeof(val));

            enable_hook = tmp;
            continue;
        }

        auto& event_fiber_map = m_adder_map[fd];
        auto [read_fiber, write_fiber] = event_fiber_map;

        // 在当前文件描述符上添加的IO事件
        int reg_event = ((event_fiber_map[READ] > 0) ? EPOLLIN: 0) | ((event_fiber_map[WRITE] > 0) ? EPOLLOUT: 0);

        if (happened_event & (EPOLLERR | EPOLLHUP)){
            happened_event |= ((EPOLLIN | EPOLLOUT) & reg_event);
        }
        int now_rw_event = happened_event & (EPOLLIN | EPOLLOUT);
        if((now_rw_event & reg_event) == 0) continue;

        // 获取还未触发的event，并重新注册。若event全部被触发，则删除。
        int left_event = reg_event & (~now_rw_event);
        int op = left_event?EPOLL_CTL_MOD: EPOLL_CTL_DEL;
        if(!left_event) { // 若event全部被触发，则删除相应的event
            m_adder_map.erase(fd);
        }else{
            event_fiber_map[(now_rw_event == EPOLLIN) ? READ: WRITE] = 0;
        }
        m_events[i].events = left_event;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_EPOLL_LEVEL
        auto _e = m_events[i].events;
        Logger::debug("Fiber: {}, call epoll_ctl({}, 0x{:x}, {}, ...), epoll events:0x{:x}", Fiber::GetCurrentId(), m_epoll_fd, op, fd, _e);
#endif
        MYRPC_SYS_ASSERT(epoll_ctl(m_epoll_fd, op, fd, &m_events[i]) == 0);

        if(now_rw_event & EPOLLIN){
            // 读事件发生，恢复相应协程执行
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
            Logger::debug("Thread: {}, Fiber: {} is ready to run #1", thread_id, read_fiber->GetId());
#endif
            auto ret_val = read_fiber->Resume();
            if(read_fiber->GetStatus() == Fiber::READY)
                m_task_queue.push(read_fiber);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
            Logger::debug("Thread: {}, Fiber: {} is swapped out #1, return value:{}, status:{}", thread_id,
                          read_fiber->GetId(), ret_val, read_fiber->GetStatus());
#endif
        }
        if(now_rw_event & EPOLLOUT){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
            Logger::debug("Thread: {}, Fiber: {} is ready to run #1", thread_id, write_fiber->GetId());
#endif
            // 写事件发生，恢复相应协程执行
            auto ret_val = write_fiber->Resume();
            if(write_fiber->GetStatus() == Fiber::READY)
                m_task_queue.push(write_fiber);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
            Logger::debug("Thread: {}, Fiber: {} is swapped out #1, return value:{}, status:{}", thread_id,
                          write_fiber->GetId(), ret_val, write_fiber->GetStatus());
#endif
        }
    }
}

int EventManager::RemoveIOEvent(int fd, EventManager::EventType event) {
    // 查找fd上是否有事件
    auto iter = m_adder_map.find(fd);
    if(iter == m_adder_map.end())
    {
        // fd上没有事件，返回
        return 0;
    }
    else{
        int new_event = 0; // 删除event后当前fd剩余的事件
        if(event == READ){
            new_event |= ((iter->second[WRITE] > 0) ? EPOLLOUT: 0);
        }else if(event == WRITE){
            new_event |= ((iter->second[READ] > 0) ? EPOLLIN: 0);
        }

        int op; // epoll_ctl 的第2个参数
        struct epoll_event event_epoll; // epoll_ctl 的第4个参数
        memset(&event_epoll, 0, sizeof(epoll_event));
        event_epoll.data.fd = fd;
        event_epoll.events = new_event;

        if(new_event == 0) {
            m_adder_map.erase(fd);
            op = EPOLL_CTL_DEL;
        }else{
            iter->second[event] = 0;
            op = EPOLL_CTL_MOD;
        }

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_EPOLL_LEVEL
        auto _e = event_epoll.events;
        Logger::debug("Fiber: {}, call epoll_ctl({}, 0x{:x}, {}, ...), epoll events:0x{:x}", Fiber::GetCurrentId(), m_epoll_fd, op, fd, _e);
#endif
        return epoll_ctl(m_epoll_fd, op, fd, &event_epoll);
    }
}

bool EventManager::IsExistIOEvent(int fd, EventManager::EventType event) const {
    // 查找fd上是否有事件
    auto iter = m_adder_map.find(fd);
    if(iter == m_adder_map.end())
    {
        // fd上没有事件，返回
        return false;
    }else{
        return iter->second[event] > 0;
    }
}

int EventManager::AddWakeupEventfd(int fd) {
    if(m_adder_map.find(fd) == m_adder_map.end()) {

        // 将Eventfd设置为非阻塞模式
        int flags;
        MYRPC_SYS_ASSERT((flags = fcntl(fd, F_GETFL, 0)) != -1);
        if (!(flags & O_NONBLOCK)) {
            flags |= O_NONBLOCK;
            MYRPC_SYS_ASSERT(fcntl(fd, F_SETFL, flags) == 0)
        }

        m_wake_up_set.emplace(fd);

        epoll_event event_epoll; // epoll_ctl 的4个参数
        memset(&event_epoll, 0, sizeof(epoll_event));
        event_epoll.data.fd = fd;
        event_epoll.events = EPOLLIN; // 等待读事件的发生

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
