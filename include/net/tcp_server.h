#ifndef MYRPC_TCP_SERVER_H
#define MYRPC_TCP_SERVER_H

#include "fiber/fiber_pool.h"
#include "net/inetaddr.h"
#include "net/socket.h"
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
         * @param fiberPool 协程池
         * @param accept_timeout accept调用的超时时间，单位微秒，0表示不设置超时时间
         * @param ipv6 bind调用的地址是否是ipv6地址
         */
        TCPServer(FiberPool::ptr fiberPool, useconds_t accept_timeout=0, bool ipv6=false);

        /**
         * @brief TCPServer类的构造函数，该方法会调用socket系统函数，若失败，会抛出SocketException异常
         * @param thread_num 协程池允许的最大线程数量
         * @param accept_timeout accept调用的超时时间，单位微秒，0表示不设置超时时间
         * @param ipv6 bind调用的地址是否是ipv6地址
         */
        TCPServer(int thread_num=std::thread::hardware_concurrency(), useconds_t accept_timeout=0, bool ipv6=false);

        ~TCPServer();

        bool Bind(InetAddr::ptr addr) noexcept;

        /**
         * @brief 开启协程池，开启Acceptor协程，启动TCP服务器
         * @note 该方法会调用listen系统函数，若失败，会抛出SocketException异常
         */
        void Start();

        /**
         * @brief 停止Acceptor协程并等待所有连接关闭，停止协程池
         * @note 必须在协程池的析构函数或Stop()方法之前调用
         */
        void Stop();

        /**
         * @brief 停止Acceptor协程，不停止协程池
         * @note 若要再次启动Acceptor协程，必须先调用Stop()方法停止协程池，再调用Start()方法重启
         */
        void StopAccept();

        void Loop(){m_fiberPool->Wait();}

        void SetAcceptTimeout(useconds_t accept_timeout){
            m_acceptor_con_timeout = accept_timeout;
        }

    protected:
        virtual void handleConnection(const Socket::ptr& sock) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
            auto clientAddr = sock->GetPeerAddr();
            Logger::info("Thread: {}, Fiber: {}: A new connection form IP:{}, port:{}, connection fd:{}", FiberPool::GetCurrentThreadId(),
                         Fiber::GetCurrentId(), clientAddr->GetIP(), clientAddr->GetPort(), sock->GetSocketfd());
#endif
        }

        FiberPool::ptr m_fiberPool;

    private:
        bool m_ipv6;

        InetAddr::ptr m_addr;

        int m_listen_sock_fd = -1; // 用于监听端口的socket
        FiberPool::FiberController::ptr m_acceptor;
        useconds_t m_acceptor_con_timeout; // Accept超时时间

        bool m_running;
        std::atomic<bool> m_stopping = {false};

        void doAccept();
    };
}

#endif //MYRPC_TCP_SERVER_H
