#include "fiber/fiber_pool.h"
#include "macro.h"
#include <fcntl.h>
#include <chrono>
#include <mutex>

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
}

FiberPool::~FiberPool() {
    if (m_running) Stop();
    MYRPC_SYS_ASSERT(close(m_global_event_fd) == 0);

    for(auto threads_context: m_threads_context_ptr) delete threads_context;
}

void FiberPool::Start() {
    if(!m_running) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_ON_LEVEL
        Logger::info("FiberPool::Start() - start");
#endif
        for (int i = 0; i < m_threads_num; i++) {
            m_threads_context_ptr.push_back(new ThreadContext);
            // 添加读eventfd事件，用以唤醒线程
            m_threads_context_ptr[i]->AddWakeupEventfd(m_global_event_fd);
            m_threads_future.push_back(std::async(std::launch::async, &FiberPool::MainLoop, this, i));
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

void FiberPool::FiberController::Join() {
    while(!m_task_ptr->stopped){
        sleep(1);
    }
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

int FiberPool::MainLoop(int thread_id) {
    now_thread_id = thread_id;
    p_fiber_pool = this;

    auto context_ptr = m_threads_context_ptr[thread_id];

    while (true) {
        if (m_stopping) return 0;

        // 1. 读取消息队列中的任务
        if(!m_tasks.empty())
        {
            std::lock_guard<SpinLock> lock(m_tasks_lock);
            for (auto iter = m_tasks.begin(); iter != m_tasks.end();) {
                const auto& tsk_ptr = *iter;
                if (tsk_ptr->thread_id == thread_id || tsk_ptr->thread_id == -1) {
                    context_ptr->my_tasks[tsk_ptr->fiber->GetId()] = tsk_ptr;
                    iter = m_tasks.erase(iter);
                } else {
                    //NotifyAll(); // 告诉其他线程有新的任务要处理
                    iter++;
                }
            }
        }

        // 2. 调度线程的所有协程
        for (auto iter = context_ptr->my_tasks.begin(); iter != context_ptr->my_tasks.end();) {
            auto fiber_id = iter->first;
            const auto& tsk_ptr = iter->second;

            MYRPC_ASSERT(tsk_ptr->fiber->GetStatus() != Fiber::ERROR);
            tsk_ptr->thread_id = thread_id;
            if (tsk_ptr->fiber->GetStatus() == Fiber::READY) { // 如果任务已就绪，那么执行任务
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                Logger::debug("Thread: {}, Fiber: {} is ready to run #1", thread_id, tsk_ptr->fiber->GetId());
#endif
                // 任务已就绪，恢复任务执行
                auto ret_val = tsk_ptr->fiber->Resume();
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
                Logger::debug("Thread: {}, Fiber: {} is swapped out #1, return value:{}, status:{}", thread_id,
                              tsk_ptr->fiber->GetId(), ret_val, tsk_ptr->fiber->GetStatus());
#endif
                // 任务让出CPU，等待下次被调度
            } else if (tsk_ptr->fiber->GetStatus() == Fiber::TERMINAL) {
                    // 协程执行完成，从任务队列中删除
                    tsk_ptr->stopped = true;
                    context_ptr->my_tasks.erase(iter++);
                    --m_tasks_cnt;
                    continue;
            }
            ++iter;
        }

        // 3. 处理epoll事件
        context_ptr->WaitEvent(now_thread_id);
    }
}

void FiberPool::Wait() {
    while (m_tasks_cnt > 0 && m_running) {
        sleep(1);
    }
}

void FiberPool::ThreadContext::resume(int64_t fiber_id) {
    auto iter = my_tasks.find(fiber_id);
    if (iter != my_tasks.end()) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
        Logger::debug("Thread: {}, Fiber: {} is ready to run from the blocked status #2",
                      FiberPool::GetCurrentThreadId(), iter->second->fiber->GetId());
#endif
        auto ret_val = (*iter).second->fiber->Resume();
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_POOL_LEVEL
        Logger::debug("Thread: {}, Fiber: {} is swapped out #2, return value:{}, status:{}", FiberPool::GetCurrentThreadId(),
                      iter->second->fiber->GetId(), ret_val, iter->second->fiber->GetStatus());
#endif
    } else {
        Logger::warn("Attempt to resume a fiber which has been erased!");
    }
}

}