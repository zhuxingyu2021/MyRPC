#ifndef MYRPC_TIMEOUTIO_H
#define MYRPC_TIMEOUTIO_H

#include <unistd.h>
#include <sys/timerfd.h>

/**
 * @brief 带timeout的read, accept, connect, recv函数
 * @param ts 超时时间
 * @note 若超时，则返回-2。其他情况下的返回值与原系统调用相同
 */
namespace MyRPC{
    ssize_t read_timeout(int fd, void *buf, size_t count, const timespec& ts);
    int accept_timeout(int sockfd, struct sockaddr *addr, socklen_t *addrlen, const timespec& ts);
    int connect_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const timespec& ts);
    ssize_t recv_timeout(int sockfd, void *buf, size_t len, int flags, const timespec& ts);
}

#endif //MYRPC_TIMEOUTIO_H
