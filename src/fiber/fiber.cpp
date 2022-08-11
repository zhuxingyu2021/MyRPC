#include "fiber/fiber.h"
#include "logger.h"
#include "macro.h"
#include "fiber/hook_sleep.h"
#include "fiber/hook_io.h"

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

Fiber::Fiber(std::function<void()> func) : m_fiber_id(++fiber_count) {
    m_func = func;
    m_status = READY;
}

Fiber::~Fiber() {
    if (m_status != TERMINAL) {
        Logger::warn("Fiber{} exited abnormally!", m_fiber_id);
    }
    delete m_func_pull_type;
}

void Fiber::Suspend() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->m_status = READY;

        MYRPC_ASSERT(ptr->m_func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->m_func_push_type))(0);
    }
}

void Fiber::Block() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->m_status = BLOCKED;

        MYRPC_ASSERT(ptr->m_func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->m_func_push_type))(0);
    }
}

void Fiber::Exit() {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->m_status = TERMINAL;

        MYRPC_ASSERT(ptr->m_func_push_type != nullptr);
        SWAP_OUT();
        // 切换上下文
        (*(ptr->m_func_push_type))(0);
    }
}

void Fiber::Resume() {
    if (m_status == READY || m_status == BLOCKED) {
        SWAP_IN();
        m_status = EXEC;
        if (!m_func_pull_type) {
            m_func_pull_type = new pull_type(Main);
            MYRPC_ASSERT(m_func_pull_type != nullptr);
        } else {
            (*m_func_pull_type)();
        }
    } else if (m_status == EXEC) {
        Logger::warn("Trying to resume fiber{} which is in execution!", m_fiber_id);
    } else {
        Logger::warn("Trying to resume fiber{} which is not in ready!", m_fiber_id);
    }
}

void Fiber::Reset() {
    if (m_status == TERMINAL || m_status == BLOCKED) {
        m_status = READY;
        delete m_func_pull_type;
        m_func_pull_type = nullptr;
        m_func_push_type = nullptr;
    } else {
        Logger::warn("Trying to reset fiber{} which is not terminated or blocked!", m_fiber_id);
    }
}

Fiber::status Fiber::GetCurrentStatus() {
    auto ptr = GET_THIS();
    if (ptr) {
        return ptr->m_status;
    }
    return ERROR;
}

int64_t Fiber::GetCurrentId() {
    auto ptr = GET_THIS();
    if (ptr) {
        return ptr->m_fiber_id;
    }
    return 0;
}

void Fiber::Main(push_type &p) {
    GET_THIS()->m_func_push_type = &p;
    try {
        GET_THIS()->m_func();
    }
    catch (std::exception &e) {
        Logger::warn("Fiber id:{} throws an exception {}.", GET_THIS()->m_fiber_id, e.what());
    }
    catch (...) {
        Logger::warn("Fiber id:{} throws an exception.", GET_THIS()->m_fiber_id);
    }
    GET_THIS()->m_status = TERMINAL;
    SWAP_OUT();
}

}