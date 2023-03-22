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
         * @param bind_addr bind调用的地址
         * @param fiberPool 协程池
         * @param timeout TCP系统调用调用的超时时间，单位微秒，0表示不设置超时时间
         */
        explicit TCPServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, useconds_t timeout=0);

        /**
         * @brief TCPServer类的构造函数，该方法会调用socket系统函数，若失败，会抛出SocketException异常
         * @param bind_addr bind调用的地址
         * @param thread_num 协程池允许的最大线程数量
         * @param timeout TCP系统调用的超时时间，单位微秒，0表示不设置超时时间
         */
        explicit TCPServer(const InetAddr::ptr& bind_addr, int thread_num=std::thread::hardware_concurrency(), useconds_t timeout=0);

        virtual ~TCPServer();

        static void handleSIGINT(int);

        virtual bool bind() noexcept;

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

        void Loop(){m_fiber_pool->Wait();}

    protected:
        virtual void handleConnection(const Socket::ptr& sock) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
            auto clientAddr = sock->GetPeerAddr();
            Logger::info("Thread: {}, Fiber: {}: A new connection form IP:{}, port:{}, connection fd:{}", FiberPool::GetCurrentThreadId(),
                         Fiber::GetCurrentId(), clientAddr->GetIP(), clientAddr->GetPort(), sock->GetSocketfd());
#endif
        }

        /**
         * @brief 判断当前服务器是否将要被关闭。（关闭原因可能是调用Stop函数/析构函数/SIGINT事件）
         */
        bool IsStopping() const{return m_stopping.load();}


        FiberPool::ptr m_fiber_pool;
        useconds_t m_timeout; // TCP超时时间

        InetAddr::ptr m_bind_addr;

    private:
        bool m_ipv6;

        int m_listen_sock_fd = -1; // 用于监听端口的socket
        Fiber::ptr m_acceptor;

        bool m_running;
        std::atomic<bool> m_stopping = {false};

        inline static std::vector<TCPServer*> m_avail_server;

        void doAccept();
    };

    extern int _sigint_handler_initializer;
    static int _my_sigint_handler_initializer = _sigint_handler_initializer;
}

#endif //MYRPC_TCP_SERVER_H
