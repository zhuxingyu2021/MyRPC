#ifndef MYRPC_SOCKET_H
#define MYRPC_SOCKET_H

#include <memory>
#include <unistd.h>

#include "macro.h"

namespace MyRPC{
    // RAII Socket对象
    class Socket: public std::enable_shared_from_this<Socket> {
    public:
        using ptr = std::shared_ptr<Socket>;
        using unique_ptr = std::unique_ptr<Socket>;

        Socket(int sockfd):m_socketfd(sockfd){}
        Socket(const Socket&) = delete;
        Socket(Socket&&) = delete;

        ~Socket(){
            if(m_socketfd != -1){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("socket fd:{} closed", m_socketfd);
#endif
                close(m_socketfd);
            }
        }

        int GetSocketfd() const{
            return m_socketfd;
        }
    private:
        int m_socketfd = -1;
    };
}

#endif //MYRPC_SOCKET_H
