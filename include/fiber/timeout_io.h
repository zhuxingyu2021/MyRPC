#ifndef MYRPC_TIMEOUT_IO_H
#define MYRPC_TIMEOUT_IO_H

#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/socket.h>

#define MYRPC_ERR_TIMEOUT_FLAG -2

/**
 * @brief 带timeout的read, accept, connect, recv函数
 * @param ts 超时时间， 单位毫秒
 * @note 若超时，则返回MYRPC_ERR_TIMEOUT_FLAG。其他情况下的返回值与原系统调用相同
 */
namespace MyRPC{
    using ms_t = unsigned int;

    extern ssize_t read_timeout(int fd, void *buf, size_t count, ms_t ts);
    extern int accept_timeout(int sockfd, sockaddr *addr, socklen_t *addrlen, ms_t ts);
    extern int connect_timeout(int sockfd, const sockaddr *addr, socklen_t addrlen, ms_t ts);
    extern ssize_t recv_timeout(int sockfd, void *buf, size_t len, int flags, ms_t ts);
    ssize_t write_timeout(int fd, const void *buf, size_t count, ms_t ts);
    ssize_t send_timeout(int fd, const void *buf, size_t len, int flags, ms_t ts);
    ssize_t readv_timeout(int fd, const struct iovec *iov, int iovcnt, ms_t ts);
    ssize_t writev_timeout(int fd, const struct iovec *iov, int iovcnt, ms_t ts);
}

#endif //MYRPC_TIMEOUT_IO_H
