#include "fiber.h"
#include "logger.h"
#include "macro.h"
#include "hooksleep.h"
#include "hookio.h"

namespace MyRPC {

std::atomic<int64_t> fiber_count = 0;

thread_local bool enable_hook = false;

// 当前线程中的活动协程
static thread_local Fiber *p_fiber = nullptr;

#define SWAP_IN() {p_fiber=this; \
enable_hook = true;}

#define SWAP_OUT() {p_fiber=nullptr; \
enable_hook = false;}

#define GET_THIS() p_fiber

Fiber::Fiber(std::function<void()> func) : fiber_id(++fiber_count) {
    _func = func;
    _status = READY;
}

Fiber::~Fiber() {
    if (_status != TERMINAL) {
        Logger::warn("Fiber{} exited abnormally!", fiber_id);
    }
    delete func_pull_type;
}

void Fiber::Suspend() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->_status = READY;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Block() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->_status = BLOCKED;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Exit() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->_status = TERMINAL;

        MYRPC_ASSERT(ptr->func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->func_push_type))(0);
    }
}

void Fiber::Resume() {
    if (_status == READY || _status == BLOCKED) {
        SWAP_IN();
        _status = EXEC;
        if (!func_pull_type) {
            func_pull_type = new pull_type(Main);
            MYRPC_ASSERT(func_pull_type != nullptr);
        } else {
            (*func_pull_type)();
        }
    } else if (_status == EXEC) {
        Logger::warn("Trying to resume fiber{} which is in execution!", fiber_id);
    } else {
        Logger::warn("Trying to resume fiber{} which is not in ready!", fiber_id);
    }
}

void Fiber::Reset() {
    if (_status == TERMINAL || _status == BLOCKED) {
        _status = READY;
        delete func_pull_type;
        func_pull_type = nullptr;
        func_push_type = nullptr;
    } else {
        Logger::warn("Trying to reset fiber{} which is not terminated or blocked!", fiber_id);
    }
}

Fiber::status Fiber::GetCurrentStatus() {
    auto ptr = GET_THIS();
    if (ptr) {
        return ptr->_status;
    }
    return ERROR;
}

int64_t Fiber::GetCurrentId() {
    auto ptr = GET_THIS();
    if (ptr) {
        return ptr->fiber_id;
    }
    return 0;
}

void Fiber::Main(push_type &p) {
    GET_THIS()->func_push_type = &p;
    try {
        GET_THIS()->_func();
    }
    catch (std::exception &e) {
        Logger::warn("Fiber id:{} throws an exception {}.", GET_THIS()->fiber_id, e.what());
    }
    catch (...) {
        Logger::warn("Fiber id:{} throws an exception.", GET_THIS()->fiber_id);
    }
    GET_THIS()->_status = TERMINAL;
    SWAP_OUT();
}

}