#ifndef MYRPC_TCPSERVER_H
#define MYRPC_TCPSERVER_H

#include "fiber/fiberpool.h"
#include "net/inetaddr.h"
#include "net/tcpsocket.h"
#include <memory>

namespace MyRPC{
    class TCPServer: public std::enable_shared_from_this<TCPServer>{
    public:
        using ptr = std::shared_ptr<TCPServer>;

        TCPServer(FiberPool::ptr fiberPool,bool ipv6=false): m_listen_sock(ipv6), m_fiberPool(fiberPool), m_stopped(true){}
        ~TCPServer();

        virtual bool Bind(InetAddr::ptr addr);

        virtual bool Start();
        virtual void Stop();

    private:
        InetAddr::ptr m_addr;
        FiberPool::ptr m_fiberPool;

        TCPSocket m_listen_sock;
        bool m_stopped;
    };
}

#endif //MYRPC_TCPSERVER_H
