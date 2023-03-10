#ifndef MYRPC_ARCH_H
#define MYRPC_ARCH_H


#if defined(__x86_64__) || defined(_M_X64)
    #define MYRPC_PAUSE asm volatile("pause;")
#elif defined(__aarch64__)
    #define MYRPC_PAUSE asm volatile("yield;");
#endif // defined(__x86_64__) || defined(_M_X64)

#endif //MYRPC_ARCH_H
