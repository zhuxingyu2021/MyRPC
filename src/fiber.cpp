#include "fiber.h"
#include "spdlog/spdlog.h"
#include "macro.h"

using namespace myrpc;

std::atomic<uint64_t> fiber_count = 0;

// 当前线程中的活动协程
static thread_local Fiber* p_fiber = nullptr;

#define SET_THIS() p_fiber=this
#define UNSET_THIS() p_fiber=nullptr
#define GET_THIS() p_fiber

Fiber::Fiber(std::function<void()> func):fiber_id(++fiber_count){
    _func = func;
    _status = READY;
}

Fiber::~Fiber() {
    if(_status != TERMINAL){
        spdlog::warn("Fiber{} exited abnormally!", fiber_id);
    }
    delete func_pull_type;
}

void Fiber::Suspend() {
    auto ptr = GET_THIS();
    if(ptr) {
        ptr->_status = READY;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        UNSET_THIS();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Block(){
    auto ptr = GET_THIS();
    if(ptr) {
        ptr->_status = BLOCKED;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        UNSET_THIS();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Exit() {
    auto ptr = GET_THIS();
    if(ptr) {
        ptr->_status = TERMINAL;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        UNSET_THIS();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Resume() {
    if(_status==READY) {
        SET_THIS();
        _status = EXEC;
        if(!func_pull_type) {
            func_pull_type = new pull_type(Main);
            MYRPC_ASSERT(func_pull_type != nullptr);
        }else {
            (*func_pull_type)();
        }
    }
    else if(_status==EXEC){
        spdlog::warn("Trying to resume fiber{} which is in execution!", fiber_id);
    }
    else{
        spdlog::warn("Trying to resume fiber{} which is not in ready!", fiber_id);
    }
}

Fiber *Fiber::GetThis() {
    return GET_THIS();
}

Fiber::status Fiber::GetCurrentStatus() {
    auto ptr = GET_THIS();
    if(ptr) {
        return ptr->_status;
    }
    return ERROR;
}

uint64_t Fiber::GetCurrentId() {
    auto ptr = GET_THIS();
    if(ptr) {
        return ptr->fiber_id;
    }
    return 0;
}

void Fiber::Main(push_type &p) {
    GET_THIS()->func_push_type = &p;
    try {
        GET_THIS()->_func();
    }
    catch(std::exception& e){
        spdlog::warn("Fiber id:{} throws an exception {}.", GET_THIS()->fiber_id, e.what());
    }
    catch(...){
        spdlog::warn("Fiber id:{} throws an exception.", GET_THIS()->fiber_id);
    }
    GET_THIS()->_status = TERMINAL;
}
