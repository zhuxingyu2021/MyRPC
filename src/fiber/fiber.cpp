#include "fiber/fiber.h"
#include "logger.h"
#include "macro.h"
#include "fiber/hook_sleep.h"
#include "fiber/hook_io.h"

extern "C"{
    extern void myrpc_ctx_switch(void* switch_out_ctx, void* switch_in_ctx);
}

namespace MyRPC {

class StackUnwindException{
    // 用于栈回溯
};

static void  unwind(){
    throw StackUnwindException();
}

std::atomic<int64_t> fiber_count = 0;

thread_local bool enable_hook = false;

// 当前线程中的活动协程
static thread_local Fiber *p_fiber = nullptr;

#define SWAP_IN() {p_fiber=this; \
enable_hook = true;}

#define SWAP_OUT() {p_fiber=nullptr; \
enable_hook = false;}

#define GET_THIS() p_fiber

#if defined(__x86_64__) || defined(_M_X64)
#define STACK_ALIGN_BYTES (16) // x86_64的栈帧必须是16字节对齐的
#define STACK_BOTTOM_ADDR ((void**)(m_stack+m_stack_size)-STACK_ALIGN_BYTES/8)
#define STACK_BOTTOM (*STACK_BOTTOM_ADDR)

#define MAINCO_CTX_OFS 28
#define SUBCO_CTX_OFS 0
#define SP_CTX_OFS 0
#define FP_CTX_OFS 1

#elif defined(__aarch64__)

#define STACK_ALIGN_BYTES (16) // arm64的栈帧必须是16字节对齐的
#define STACK_BOTTOM_ADDR ((void**)(m_stack+m_stack_size))
#define STACK_BOTTOM (*STACK_BOTTOM_ADDR)

#define MAINCO_CTX_OFS 22
#define SUBCO_CTX_OFS 0
#define SP_CTX_OFS 12
#define FP_CTX_OFS 10
#define RET_CTX_OFS 13

#endif

#define RESUME(ctx_addr) myrpc_ctx_switch((ctx_addr) + MAINCO_CTX_OFS, (ctx_addr) + SUBCO_CTX_OFS);
#define YIELD(ctx_addr) myrpc_ctx_switch((ctx_addr) + SUBCO_CTX_OFS, (ctx_addr) + MAINCO_CTX_OFS);

void Fiber::init_stack_and_ctx() {
    memset(m_ctx, 0, 26 * sizeof(void*));
    m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS] = STACK_BOTTOM_ADDR; // 子协程的rsp指向协程栈的栈底
    m_ctx[SUBCO_CTX_OFS + FP_CTX_OFS] = m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS]; // 设置子协程的rbp = rsp

    // 第一次切换到协程时，需要跳转到Fiber::Main位置
#if defined(__x86_64__) || defined(_M_X64)
    STACK_BOTTOM = (void*)&Fiber::Main;
#elif defined(__aarch64__)
    m_ctx[SUBCO_CTX_OFS + RET_CTX_OFS] = (void*)&Fiber::Main;
#endif
}

Fiber::Fiber(const std::function<void()>& func) : m_fiber_id(++fiber_count), m_func(func), m_status(READY) {
    m_stack_size = init_stack_size;
    m_stack = (char*) aligned_alloc(64, sizeof(char) * init_stack_size);
    if(m_stack)
        init_stack_and_ctx();
}

Fiber::Fiber(std::function<void()>&& func) : m_fiber_id(++fiber_count), m_func(std::move(func)), m_status(READY) {
    m_stack_size = init_stack_size;
    m_stack = (char*) aligned_alloc(64, sizeof(char) * init_stack_size);;
    if(m_stack)
        init_stack_and_ctx();
}

Fiber::~Fiber() {
    if (m_status == EXEC) {
        MYRPC_CRITIAL_ERROR("Try to close a running fiber, id: " + std::to_string(m_fiber_id));
    }else if(m_status != TERMINAL && m_status != ERROR){
        // stack unwinding
#if defined(__x86_64__) || defined(_M_X64)
        void** rsp = (void**)m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS];

        // push (void**)&unwind
        --rsp;
        *rsp = (void**)&unwind;
        m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS] = (char*)rsp;
#elif defined(__aarch64__)
        m_ctx[SUBCO_CTX_OFS + RET_CTX_OFS] = (void**)&unwind;
#endif
        // 切换上下文
        RESUME(m_ctx);
    }
    if(m_stack != nullptr)
        free(m_stack);
}

void Fiber::Suspend(int64_t return_value) {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->m_status = READY;

        SWAP_OUT();
        // 切换上下文
        YIELD(ptr->m_ctx);
    }
}

void Fiber::Block(int64_t return_value) {
    auto ptr = GET_THIS();
    if (ptr) {
        ptr->m_status = BLOCKED;

        SWAP_OUT();
        // 切换上下文
        YIELD(ptr->m_ctx);
    }
}

void Fiber::Exit(int64_t return_value) {
    auto ptr = GET_THIS();
    if (ptr) {
        // 栈回溯
        StackUnwindException _uw;
        throw _uw;
    }
}

int64_t Fiber::Resume() {
    if (m_status == READY || m_status == BLOCKED) {
        SWAP_IN();
        m_status = EXEC;

        // 切换上下文
        RESUME(m_ctx);
    } else if (m_status == EXEC) {
        Logger::warn("Trying to resume fiber{} which is in execution!", m_fiber_id);
    } else {
        Logger::warn("Trying to resume fiber{} which is not in ready!", m_fiber_id);
    }
    return 0;
}

void Fiber::Reset() {
    if (m_status == TERMINAL || m_status == ERROR) {
        m_status = READY;
        init_stack_and_ctx();
    } else {
        // Stack Unwinding
#if defined(__x86_64__) || defined(_M_X64)
        void** rsp = (void**)m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS];

        // push (void**)&unwind
        --rsp;
        *rsp = (void**)&unwind;
        m_ctx[SUBCO_CTX_OFS + SP_CTX_OFS] = (char*)rsp;
#elif defined(__aarch64__)
        m_ctx[SUBCO_CTX_OFS + RET_CTX_OFS] = (void**)&unwind;
#endif

        // 切换上下文
        RESUME(m_ctx);
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

void Fiber::Main() {
    auto ptr = GET_THIS();
    {
        try {
            ptr->m_func();
        }
        catch (std::exception &e) {
            Logger::warn("Fiber id:{} throws an exception {}.", ptr->m_fiber_id, e.what());
        }
        catch(StackUnwindException &e){
            // Do nothing
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_FIBER_LEVEL
            Logger::debug("Fiber id: {} stack unwinding", ptr->m_fiber_id);
#endif
        }
        catch (...) {
            Logger::warn("Fiber id:{} throws an exception.", ptr->m_fiber_id);
        }
    }
    ptr->m_status = TERMINAL;
    SWAP_OUT();

    // 上下文切换
    YIELD(ptr->m_ctx);
}

Fiber::ptr Fiber::GetSharedFromThis() {
    return GET_THIS()->shared_from_this();
}

size_t Fiber::GetStacksize() {
    return GET_THIS()->m_stack_size;
}

size_t Fiber::GetStackFreeSize() {
    char* stack = GET_THIS()->m_stack;
    char* rsp;
    asm volatile("movq %%rsp, %0":"=g"(rsp));
    return (size_t)(rsp-stack);
}

bool Fiber::ExtendStackCapacity() {
    /*
    Fiber* volatile ptr = GET_THIS();
    size_t volatile new_stack_size = ptr->m_stack_size * 2;
    char* volatile new_stack = (char*) std::aligned_alloc(4096, sizeof(char) * new_stack_size);

    char volatile *rsp;
    char volatile *rbp;
    char *new_rsp, *new_rbp;
    asm volatile("movq %%rsp, %0":"=g"(rsp));
    asm volatile("movq %%rbp, %0":"=g"(rbp));

    size_t rsp_ofs, rbp_ofs;
    if(!new_stack)
        return false;

    memcpy(new_stack+ptr->m_stack_size, ptr->m_stack, ptr->m_stack_size);

    rsp_ofs = (ptr->m_stack + ptr->m_stack_size) - rsp;
    rbp_ofs = (ptr->m_stack + ptr->m_stack_size) - rbp;

    new_rsp = new_stack + new_stack_size - rsp_ofs;
    new_rbp = new_stack + new_stack_size - rbp_ofs;


    asm volatile("movq %0, %%rsp":"=r"(new_rsp));
    asm volatile("movq %0, %%rbp":"=r"(new_rbp));

    auto tmp = ptr->m_stack;
    ptr->m_stack = new_stack;
    ptr->m_stack_size = new_stack_size;
    free(tmp);*/

    MYRPC_NO_IMPLEMENTATION_ERROR();
}

}