#ifndef MYRPC_TIMEOUTIO_H
#define MYRPC_TIMEOUTIO_H

#include <unistd.h>

/**
 * @brief 带timeout的read, accept, connect, recv函数
 * @param __useconds 超时时间，单位为微秒
 * @note 若超时，则返回-2。其他情况下的返回值与原系统调用相同
 */
namespace MyRPC{
    ssize_t read_timeout(int fd, void *buf, size_t count, __useconds_t __useconds);
    int accept_timeout(int sockfd, struct sockaddr *addr, socklen_t *addrlen, __useconds_t __useconds);
    int connect_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, __useconds_t __useconds);
    ssize_t recv_timeout(int sockfd, void *buf, size_t len, int flags, __useconds_t __useconds);
}

#endif //MYRPC_TIMEOUTIO_H
