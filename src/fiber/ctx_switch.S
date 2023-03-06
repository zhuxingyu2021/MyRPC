.text
.globl ctx_resume
.globl ctx_yield

ctx_resume:
#if defined(__x86_64__)

    xchgq %rsp, %rdi
    // 现在%edi寄存器中是原先的%esp值

    // 保存上下文
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %rdi

    // 恢复上下文
    subq $104, %rsp

    popq %rdi
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r9
    popq %r8
    popq %rbp
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    xchgq %rsp, %rdi
    ret

#endif

ctx_yield:
#if defined(__x86_64__)

    xchgq %rsp, %rdi
    // 现在%edi寄存器中是原先的%esp值

    // 保存上下文
    subq $104, %rsp
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %rdi

    // 恢复上下文
    addq $104, %rsp

    popq %rdi
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r9
    popq %r8
    popq %rbp
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    xchgq %rsp, %rdi
    ret

#endif

// TODO i386 aarch64 ...