#ifndef MYRPC_TCPSERVER_H
#define MYRPC_TCPSERVER_H

#include "fiber/fiberpool.h"
#include "net/inetaddr.h"
#include <memory>
#include <atomic>
#include <set>

#include <unistd.h>

namespace MyRPC{
    class TCPServer: public std::enable_shared_from_this<TCPServer>{
    public:
        using ptr = std::shared_ptr<TCPServer>;

        TCPServer(FiberPool::ptr fiberPool,  __useconds_t accept_timeout=0, bool ipv6=false);

        /**
         * @note 在析构函数之前，必须先调用Stop()，停止Acceptor协程
         */
        virtual ~TCPServer();

        virtual bool Bind(InetAddr::ptr addr);

        /**
         * @brief 开启Acceptor协程，启动TCP服务器
         * @return true表示成功，false表示失败
         */
        virtual bool Start();

        /**
         * @brief 停止服务器
         * @note 必须在协程池的析构函数或Stop()方法之前调用
         */
        virtual void Stop();
    protected:
        virtual void handleConnection(int sockfd);

    private:
        bool m_ipv6;

        InetAddr::ptr m_addr;
        FiberPool::ptr m_fiberPool;

        int m_listen_sock_fd = -1; // 用于监听端口的socket
        FiberPool::FiberController::ptr m_acceptor_fiber_con; // Acceptor协程
        useconds_t m_acceptor_con_timeout; // Accept超时时间

        std::set<int> m_conn_sock_fds; // 用于发送/接收数据的socket

        bool m_running;
        std::atomic<bool> m_stopping = {false};

        void doAccept();
    };
}

#endif //MYRPC_TCPSERVER_H
