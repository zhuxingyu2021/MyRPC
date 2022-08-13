#ifndef MYRPC_NONCOPYABLE_H
#define MYRPC_NONCOPYABLE_H

namespace MyRPC{
    class NonCopyable{
    public:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(const NonCopyable& noncopyable) = delete;
        NonCopyable& operator=(const NonCopyable& noncopyable) = delete;
    };
}

#endif //MYRPC_NONCOPYABLE_H
