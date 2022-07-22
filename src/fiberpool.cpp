#include "fiberpool.h"
#include "macro.h"
#include <fcntl.h>
#include <chrono>
#include <mutex>

using namespace MyRPC;

// 当前线程的协程池
static thread_local FiberPool* p_fiber_pool = nullptr;

// 当前线程id
static thread_local int now_thread_id = -2;

FiberPool::FiberPool(int thread_num) :n_threads(thread_num){
    _threads_context_ptr.reserve(thread_num);
    _threads_future.reserve(thread_num);

    // 打开pipe
    MYRPC_SYS_ASSERT(pipe(pipe_fd) == 0);
    // 将读pipe设定为非阻塞IO
    MYRPC_SYS_ASSERT(fcntl(pipe_fd[0],F_SETFL,O_NONBLOCK)==0);
}

FiberPool::~FiberPool() {
    Stop();
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

void FiberPool::Start() {
    for(int i=0; i<n_threads; i++){
        _threads_context_ptr.emplace_back(new ThreadContext);
        // 添加读pipe事件，用以唤醒线程
        _threads_context_ptr[i]->AddIOFunc(pipe_fd[0], EventManager::READ, [this](){
            uint8_t dummy[256];
            while(read(this->pipe_fd[0], dummy, sizeof(dummy)) > 0);
        });
        _threads_future.emplace_back(std::async(std::launch::async, &FiberPool::MainLoop, this, i));
    }
}

void FiberPool::Stop() {
    stopping = true;
    for(int i=0; i<n_threads; i++){
        NotifyAll();
        auto status = _threads_future[i].wait_for(std::chrono::nanoseconds(1));
        while(status != std::future_status::ready){
            NotifyAll();
            status = _threads_future[i].wait_for(std::chrono::nanoseconds(1));
        }
        MYRPC_ASSERT(_threads_future[i].get() == 0);
    }
}

void FiberPool::NotifyAll() {
    MYRPC_SYS_ASSERT(write(pipe_fd[1], "N", 1) == 1);
}

FiberPool::FiberController FiberPool::Run(std::function<void()> func, int thread_id, bool circular) {
    Task::ptr ptr(new Task(func, thread_id, circular));
    {
        std::lock_guard<SpinLock> lock(_tasks_lock);
        _tasks.push_back(ptr);
    }
    NotifyAll();
    FiberController j(ptr);
    return j;
}

int FiberPool::GetCurrentThreadId() {
    return now_thread_id;
}

FiberPool *FiberPool::GetThis() {
    return p_fiber_pool;
}

EventManager::ptr FiberPool::GetEventManager() {
    if(now_thread_id>=0) {
        MYRPC_ASSERT(p_fiber_pool);
        return static_cast<EventManager::ptr>(p_fiber_pool->_threads_context_ptr[now_thread_id]);
    }
    return nullptr;
}

int FiberPool::MainLoop(int thread_id) {
    now_thread_id = thread_id;
    p_fiber_pool = this;

    auto context_ptr = _threads_context_ptr[thread_id];

    while(true){
        if(stopping) return 0;

        // 1. 读取消息队列中的任务
        {
            std::lock_guard<SpinLock> lock(_tasks_lock);
            for(auto iter = _tasks.begin(); iter != _tasks.end();){
                auto tsk_ptr = *iter;
                if(tsk_ptr->thread_id == thread_id || tsk_ptr->thread_id == -1){
                    context_ptr->my_tasks[tsk_ptr->fiber->GetId()] = tsk_ptr;
                    iter = _tasks.erase(iter);
                }else{
                    NotifyAll(); // 告诉其他线程有新的任务要处理
                    iter++;
                }
            }
        }

        // 2. 调度线程的所有协程
        for(auto iter = context_ptr->my_tasks.begin(); iter != context_ptr->my_tasks.end();){
            auto [fiber_id, tsk_ptr] = *iter;
            MYRPC_ASSERT(tsk_ptr->fiber->GetStatus() != Fiber::ERROR);
            tsk_ptr->thread_id = thread_id;
            if(tsk_ptr->fiber->GetStatus() == Fiber::READY){ // 如果任务已就绪，那么执行任务
                tsk_ptr->fiber->Resume();
                if((tsk_ptr->fiber->GetStatus() == Fiber::READY) || (tsk_ptr->fiber->GetStatus() == Fiber::BLOCKED)){
                    // 任务让出CPU，等待下次被调度
                }
            }
            if(tsk_ptr->fiber->GetStatus() == Fiber::TERMINAL){
                if(tsk_ptr->circular) {
                    ++tsk_ptr->circular_count;
                    // 等待下次被调度
                }else{
                    // 协程执行完成，从任务队列中删除
                    tsk_ptr->stopped = true; // 执行完成
                    context_ptr->my_tasks.erase(iter++); continue;
                }
            }
            ++iter;
        }

        // 3. 处理epoll事件
        context_ptr->WaitEvent();
    }
}

void FiberPool::ThreadContext::resume(int64_t fiber_id) {
    auto iter = my_tasks.find(fiber_id);
    if(iter!=my_tasks.end()) {
        (*iter).second->fiber->Resume();
    }
    else{
        Logger::warn("Attempt to resume a fiber which has been erased!");
    }
}
