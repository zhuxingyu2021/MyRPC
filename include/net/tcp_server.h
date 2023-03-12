#ifndef MYRPC_TCP_SERVER_H
#define MYRPC_TCP_SERVER_H

#include "fiber/fiber_pool.h"
#include "net/inetaddr.h"
#include "net/socket.h"
#include "net/tcp_server_conn.h"
#include <memory>
#include <atomic>
#include <set>
#include <thread>

#include <unistd.h>

#include "noncopyable.h"

namespace MyRPC{
    class TCPServer: public NonCopyable{
    public:
        using ptr = std::shared_ptr<TCPServer>;

        /**
         * @brief TCPServer类的构造函数，该方法会调用socket系统函数，若失败，会抛出SocketException异常
         * @param bind_addr bind调用的地址
         * @param fiberPool 协程池
         * @param timeout TCP系统调用调用的超时时间，单位微秒，0表示不设置超时时间
         */
        TCPServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, useconds_t timeout=0);

        virtual ~TCPServer();

        virtual bool bind() noexcept;

        /**
         * @brief 开启Acceptor协程，启动TCP服务器
         * @note 该方法会调用listen系统函数，若失败，会抛出SocketException异常
         */
        void Start();

        /**
         * @brief 停止Acceptor协程
         */
        void Stop();

        void Loop(){m_fiber_pool->Wait();}

    protected:
        template <class Func>
        void AddConnectionHandler(Func&& func){
            m_conn_handler.push_back(std::forward<Func>(func));
        }

        template <class T>
        void SetConnectionClass(){
            m_conn_establish_handler = [](TCPServerConn** ptr){
                *ptr = new T;
            };
            m_conn_close_handler = [](TCPServerConn* ptr){
                delete ptr;
            };
        }

        void DisableConnectionHandler(){
            m_conn_handler.clear();
        }

        void UnsetConnectionClass(){
            m_conn_establish_handler = nullptr;
            m_conn_close_handler = nullptr;
        }

        FiberPool::ptr m_fiber_pool;
        useconds_t m_timeout; // TCP超时时间

        InetAddr::ptr m_bind_addr;

    private:
        bool m_ipv6;

        int m_listen_sock_fd = -1; // 用于监听端口的socket
        Fiber::ptr m_acceptor = nullptr;
        int m_acceptor_thread_id = -1;

        void _do_accept();

        std::vector<std::function<void(Socket::ptr, TCPServerConn*)>> m_conn_handler;
        std::function<void(TCPServerConn*)> m_conn_close_handler;
        std::function<void(TCPServerConn**)> m_conn_establish_handler;
    };
}

#endif //MYRPC_TCP_SERVER_H
