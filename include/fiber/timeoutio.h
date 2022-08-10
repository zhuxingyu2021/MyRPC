#ifndef MYRPC_TIMEOUTIO_H
#define MYRPC_TIMEOUTIO_H

#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/socket.h>

/**
 * @brief 带timeout的read, accept, connect, recv函数
 * @param ts 超时时间， 单位微妙
 * @note 若超时，则返回-2。其他情况下的返回值与原系统调用相同
 */
namespace MyRPC{
    extern ssize_t read_timeout(int fd, void *buf, size_t count, useconds_t ts);
    extern int accept_timeout(int sockfd, sockaddr *addr, socklen_t *addrlen, useconds_t ts);
    extern int connect_timeout(int sockfd, const sockaddr *addr, socklen_t addrlen, useconds_t ts);
    extern ssize_t recv_timeout(int sockfd, void *buf, size_t len, int flags, useconds_t ts);
}

#endif //MYRPC_TIMEOUTIO_H
