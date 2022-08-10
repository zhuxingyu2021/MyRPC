#ifndef MYRPC_TCPSOCKET_H
#define MYRPC_TCPSOCKET_H

#include "net/inetaddr.h"
#include <memory>
#include <exception>

namespace MyRPC{

    // TCP Socket的封装
    class TCPSocket: public std::enable_shared_from_this<TCPSocket>{
    public:
        using ptr = std::shared_ptr<TCPSocket>;

        explicit TCPSocket(bool ipv6 = false);
        TCPSocket(const TCPSocket&) = delete;
        ~TCPSocket();

        virtual bool Bind(InetAddr::ptr addr);
    private:
        int m_sockfd;
    };

}

#endif //MYRPC_TCPSOCKET_H
