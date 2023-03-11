#include "fiber/fiber_pool.h"
#include "macro.h"
#include <fcntl.h>
#include <chrono>
#include <mutex>
#include <unistd.h>

#include <sys/eventfd.h>

namespace MyRPC {

// 当前线程的协程池
static thread_local FiberPool *p_fiber_pool = nullptr;

// 当前线程id
static thread_local int now_thread_id = -2;

FiberPool::FiberPool(int thread_num) : m_threads_num(thread_num) {
    m_threads_context_ptr.reserve(thread_num);
    m_threads_future.reserve(thread_num);

    // 创建event_fd
    m_global_event_fd = eventfd(0, O_NONBLOCK);
    MYRPC_ASSERT(m_global_event_fd > 0);

    // 初始化Eventmanager
    for(int i = 0; i < thread_num; ++i){
        m_threads_context_ptr.push_back(new EventManager);
    }
}

FiberPool::~FiberPool() {
    if (m_running) Stop();
    close(m_global_event_fd);

    for(auto threads_context: m_threads_context_ptr) delete threads_context;
}

void FiberPool::Start() {
    if(!m_running) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL
        Logger::info("FiberPool::Start() - start");
#endif
        for (int i = 0; i < m_threads_num; i++) {
            // 添加读eventfd事件，用以唤醒线程
            m_threads_context_ptr[i]->AddWakeupEventfd(m_global_event_fd);
            m_threads_future.push_back(std::async(std::launch::async, &FiberPool::_main_loop, this, i));
        }
        m_running = true;
    }
}

void FiberPool::Stop() {
    if(m_running) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL
        Logger::info("FiberPool::Stop() - stop");
#endif
        m_stopping = true;
        for (int i = 0; i < m_threads_num; i++) {
            NotifyAll();
            auto status = m_threads_future[i].wait_for(std::chrono::nanoseconds(1));
            while (status != std::future_status::ready) {
                NotifyAll();
                status = m_threads_future[i].wait_for(std::chrono::nanoseconds(1));
            }
            MYRPC_ASSERT(m_threads_future[i].get() == 0);
        }
        for(auto threads_context: m_threads_context_ptr) delete threads_context;
        m_threads_context_ptr.clear();
        m_threads_future.clear();
        m_running = false;
    }
}

void FiberPool::NotifyAll() {
    auto tmp = enable_hook;
    enable_hook = false;
    uint64_t val = 1;
    MYRPC_SYS_ASSERT(write(m_global_event_fd, &val, sizeof(uint64_t)) == sizeof(uint64_t));
    enable_hook = tmp;
}

int FiberPool::GetCurrentThreadId() {
    return now_thread_id;
}

FiberPool *FiberPool::GetThis() {
    return p_fiber_pool;
}

EventManager* FiberPool::GetEventManager() {
    if (now_thread_id >= 0) {
        MYRPC_ASSERT(p_fiber_pool);
        return static_cast<EventManager*>(p_fiber_pool->m_threads_context_ptr[now_thread_id]);
    }
    return nullptr;
}

int FiberPool::_main_loop(int thread_id) {
    now_thread_id = thread_id;
    p_fiber_pool = this;

    auto context_ptr = m_threads_context_ptr[thread_id];

    while(!m_running); // 等待所有线程启动完成

    while (true) {
        if (m_stopping) return 0;

        Fiber::ptr* tsk_ptr_from_q = nullptr;
        // 1. 调度线程的所有协程
        while(context_ptr->m_task_queue.TryPop(tsk_ptr_from_q)){
            auto tsk_ptr = *tsk_ptr_from_q;

            MYRPC_ASSERT(tsk_ptr->GetStatus() != Fiber::ERROR);
            if (tsk_ptr->GetStatus() == Fiber::READY) { // 如果任务已就绪，那么执行任务
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                Logger::debug("Thread: {}, Fiber: {} is ready to run #1", thread_id, tsk_ptr->GetId());
#endif
                // 任务已就绪，恢复任务执行
                auto ret_val = tsk_ptr->Resume();
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                Logger::debug("Thread: {}, Fiber: {} is swapped out #1, return value:{}, status:{}", thread_id,
                              tsk_ptr->GetId(), ret_val, tsk_ptr->GetStatus());
#endif
                // 任务让出CPU，等待下次被调度
                auto status = tsk_ptr->GetStatus();
                if(status == Fiber::READY) {
                    if(!context_ptr->m_task_queue.TryPush(tsk_ptr_from_q)){
                        MYRPC_CRITIAL_ERROR("Task queue is full!");
                    }
                }
                else if(status == Fiber::BLOCKED){
                    delete tsk_ptr_from_q;
                }
                else{
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                    Logger::debug("Thread: {}, Fiber: {} is going to delete", thread_id, tsk_ptr->GetId());
#endif
                    delete tsk_ptr_from_q;
                    --m_tasks_cnt;
                }
                tsk_ptr_from_q = nullptr;
            }else if (tsk_ptr->GetStatus() == Fiber::TERMINAL) {
                    // 协程执行完成，从任务队列中删除
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                Logger::debug("Thread: {}, Fiber: {} is going to delete", thread_id, tsk_ptr->GetId());
#endif
                delete tsk_ptr_from_q;
                tsk_ptr_from_q = nullptr;
                --m_tasks_cnt;
            }else{
                // 该分支不应该被执行
                MYRPC_CRITIAL_ERROR("Internal Error!");
            }
        }

        // 2. 处理epoll事件
        context_ptr->WaitEvent(now_thread_id);
    }
}

void FiberPool::Wait() {
    while (m_tasks_cnt > 0 && m_running) {
        sleep(1);
    }
}

}