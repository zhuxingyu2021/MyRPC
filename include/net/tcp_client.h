#ifndef MYRPC_TCP_CLIENT_H
#define MYRPC_TCP_CLIENT_H

#include "fiber/fiber_pool.h"
#include "net/inetaddr.h"
#include "net/socket.h"

#include "noncopyable.h"
#include <memory>

namespace MyRPC{
    class TCPClient: public NonCopyable{
    public:
        using ptr = std::shared_ptr<TCPClient>;

        TCPClient(InetAddr::ptr& server_addr, FiberPool::ptr& fiberPool, useconds_t timeout=0):
                m_server_addr(server_addr), m_fiberPool(fiberPool), m_timeout(timeout){}

        virtual ~TCPClient(){disConnect();}

        virtual bool Connect(){
            m_closing = false;
            if(m_connection_closed) {
                m_sock = Socket::Connect(m_server_addr, m_timeout);
                if (m_sock) {
                    m_fiberPool->Run(std::bind(&TCPClient::handleConnect, this));
                    m_connection_closed = false;
                    return true;
                }
                return false;
            }
            return true;
        };

        virtual void disConnect(){
            m_closing = true;
        }

        const InetAddr::ptr& GetServerAddr() const{return m_server_addr;}

        bool IsClosed() const{return m_connection_closed.load();}
        bool IsClosing() const{return m_closing.load();}

    protected:
        virtual void handleConnect(){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
            Logger::info("Thread: {}, Fiber: {}: A new connection to server IP:{}, port:{}, connection fd:{}", FiberPool::GetCurrentThreadId(),
                         Fiber::GetCurrentId(), m_server_addr->GetIP(), m_server_addr->GetPort(), m_sock->GetSocketfd());
#endif
        }

        InetAddr::ptr m_server_addr;
        Socket::ptr m_sock;

        useconds_t m_timeout;
        FiberPool::ptr m_fiberPool;

        std::atomic<bool> m_connection_closed = {true};
    private:
        bool m_ipv6;

        std::atomic<bool> m_closing = {false};
    };
}

#endif //MYRPC_TCP_CLIENT_H
