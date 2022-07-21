#include "fiberpool.h"
#include "macro.h"
#include <fcntl.h>

using namespace myrpc;

// 当前线程的协程池
static thread_local FiberPool* p_fiber_pool = nullptr;

// 当前线程id
thread_local int now_thread_id = -2;

FiberPool::FiberPool(int thread_num) :n_threads(thread_num), stopping(false){
    _threads_context.resize(thread_num);
    _threads_ptr.reserve(thread_num);

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
        // 添加读pipe事件，用以唤醒线程
        _threads_context[i]._manager.AddIOFunc(pipe_fd[0], EventManager::READ, [this](){
            uint8_t dummy[256];
            while(read(this->pipe_fd[0], dummy, sizeof(dummy)) > 0);
        });
        _threads_ptr.push_back(new std::thread(&FiberPool::MainLoop, this, i));
    }
}

void FiberPool::Stop() {
    stopping = true;
    NotifyAll();
    for(auto t:_threads_ptr){
       t->join();
       delete t;
    }
}

void FiberPool::NotifyAll() {
    MYRPC_SYS_ASSERT(write(pipe_fd[1], "N", 1) == 1);
}

FiberPool::FiberController FiberPool::Run(std::function<void()> func, int thread_id, bool circular) {
    Task::ptr ptr(new Task({std::make_shared<Fiber>(func), thread_id, circular, false}));
    {
        std::unique_lock<std::shared_mutex> lock(_tasks_mutex);
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

EventManager *FiberPool::GetEventManager() {
    if(now_thread_id>=0) {
        MYRPC_ASSERT(p_fiber_pool);
        return &(p_fiber_pool->_threads_context[now_thread_id]._manager);
    }
    return nullptr;
}

void FiberPool::MainLoop(int thread_id) {
    now_thread_id = thread_id;
    p_fiber_pool = this;


    auto& context = _threads_context[thread_id];
    auto& eve_manager = context._manager;

    while(true){
        if(stopping) return;

        // 1. 读取消息队列中的任务
        {
            std::unique_lock<std::shared_mutex> lock(_tasks_mutex);
            for(auto iter = _tasks.begin(); iter != _tasks.end();){
                auto tsk_ptr = *iter;
                if(tsk_ptr->thread_id == thread_id || tsk_ptr->thread_id == -1){
                    context.my_tasks.push_back(tsk_ptr);
                    iter = _tasks.erase(iter);
                }else{
                    iter++;
                }
            }
        }

        // 2. 调度线程的所有协程
        for(auto iter = context.my_tasks.begin(); iter != context.my_tasks.end();){
            auto tsk_ptr = *iter;
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
                    // 等待下次被调度
                }else{
                    // 协程执行完成，从任务队列中删除
                    tsk_ptr->joinable = true; // 执行完成，已经能够被Join
                    iter = context.my_tasks.erase(iter); continue;
                }
            }
            ++iter;
        }

        // 3. 处理epoll事件
        eve_manager.WaitEvent();
    }
}
