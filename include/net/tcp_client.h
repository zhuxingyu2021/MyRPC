#ifndef MYRPC_TCP_CLIENT_H
#define MYRPC_TCP_CLIENT_H

#include "fiber/fiber_pool.h"
#include "net/inetaddr.h"
#include "net/socket.h"

#include "noncopyable.h"
#include <memory>
#include <vector>

namespace MyRPC{
    class TCPClient: public NonCopyable{
    public:
        using ptr = std::shared_ptr<TCPClient>;

        TCPClient(InetAddr::ptr& server_addr, FiberPool::ptr& fiberPool, ms_t timeout=0):
                m_server_addr(server_addr), m_fiber_pool(fiberPool), m_timeout(timeout){}

        virtual ~TCPClient(){DisConnect();}

        bool Connect();

        void DisConnect();

        const InetAddr::ptr& GetServerAddr() const{return m_server_addr;}

        bool IsClosed() const{return m_conn_handler_array.empty();}

    protected:
        template <class Func>
        void AddConnectionHandler(Func&& func){
            m_conn_handler.push_back(std::forward<Func>(func));
        }

        template <class Func>
        void SetCloseHandler(Func&& func){
            m_close_handler(std::forward<Func>(func));
        }

        void DisableConnectionHandler(){
            m_conn_handler.clear();
        }

        void UnsetCloseHandler(){
            m_close_handler = nullptr;
        }

        InetAddr::ptr m_server_addr;

        ms_t m_timeout;
        FiberPool::ptr m_fiber_pool;

    private:
        std::vector<std::function<void(Socket::ptr)>> m_conn_handler;
        std::vector<Fiber::ptr> m_conn_handler_array;
        std::function<void()> m_close_handler;

        int m_conn_thread_id = -1;

        bool m_ipv6;
    };
}

#endif //MYRPC_TCP_CLIENT_H
