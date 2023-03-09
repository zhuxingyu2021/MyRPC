.text
.globl myrpc_ctx_switch

myrpc_ctx_switch:
#if defined(__x86_64__) || defined(_M_X64)

    xchgq %rsp, %rdi
    // 现在%rdi寄存器中是原先的%rsp值

    // 保存上下文
    movq %rdi, (%rsp)
    movq %rbp, 8(%rsp)
    movq %r12, 16(%rsp)
    movq %r13, 24(%rsp)
    movq %r14, 32(%rsp)
    movq %r15, 40(%rsp)
    movq %rbx, 48(%rsp)
    movaps %xmm6, 64(%rsp)
    movaps %xmm7, 80(%rsp)
    movaps %xmm8, 96(%rsp)
    movaps %xmm9, 112(%rsp)
    movaps %xmm10, 128(%rsp)
    movaps %xmm11, 144(%rsp)
    movaps %xmm12, 160(%rsp)
    movaps %xmm13, 176(%rsp)
    movaps %xmm14, 192(%rsp)
    movaps %xmm15, 208(%rsp)

    // 恢复上下文
    movq %rsi, %rsp
    movq (%rsp), %rdi
    movq 8(%rsp), %rbp
    movq 16(%rsp), %r12
    movq 24(%rsp), %r13
    movq 32(%rsp), %r14
    movq 40(%rsp), %r15
    movq 48(%rsp), %rbx
    movaps 64(%rsp), %xmm6
    movaps 80(%rsp), %xmm7
    movaps 96(%rsp), %xmm8
    movaps 112(%rsp), %xmm9
    movaps 128(%rsp), %xmm10
    movaps 144(%rsp), %xmm11
    movaps 160(%rsp), %xmm12
    movaps 176(%rsp), %xmm13
    movaps 192(%rsp), %xmm14
    movaps 208(%rsp), %xmm15

    movq %rdi, %rsp
    ret

#elif defined(__aarch64__)
    mov x10, sp
    mov x11, x30
    stp x19, x20, [x0, #(0*16)]
    stp x21, x22, [x0, #(1*16)]
    stp d8, d9, [x0, #(7*16)]
    stp x23, x24, [x0, #(2*16)]
    stp d10, d11, [x0, #(8*16)]
    stp x25, x26, [x0, #(3*16)]
    stp d12, d13, [x0, #(9*16)]
    stp x27, x28, [x0, #(4*16)]
    stp d14, d15, [x0, #(10*16)]
    stp x29, x30, [x0, #(5*16)]
    stp x10, x11, [x0, #(6*16)]
    ldp x19, x20, [x1, #(0*16)]
    ldp x21, x22, [x1, #(1*16)]
    ldp d8, d9, [x1, #(7*16)]"
    ldp x23, x24, [x1, #(2*16)]
    ldp d10, d11, [x1, #(8*16)]
    ldp x25, x26, [x1, #(3*16)]
    ldp d12, d13, [x1, #(9*16)]
    ldp x27, x28, [x1, #(4*16)]
    ldp d14, d15, [x1, #(10*16)]
    ldp x29, x30, [x1, #(5*16)]
    ldp x10, x11, [x1, #(6*16)]
    mov sp, x10
    br x11
#endif
