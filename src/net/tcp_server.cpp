#include <sys/types.h>
#include <sys/socket.h>

#include <utility>
#include <functional>

#include "fiber/fiber.h"
#include "fiber/fiber_pool.h"
#include "net/tcp_server.h"
#include "debug.h"
#include "fiber/timeout_io.h"

#include "net/exception.h"
#include <csignal>

using namespace MyRPC;

TCPServer::TCPServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, ms_t timeout) : m_fiber_pool(fiberPool),
                                                                                                     m_timeout(timeout),
                                                                                                     m_acceptor(nullptr),
                                                                                                     m_bind_addr(bind_addr){
    m_listen_sock_fd = socket(bind_addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_ASSERT_EXCEPTION(m_listen_sock_fd >= 0, throw NetException("TCPServer socket creation", NetException::SYS));
}

TCPServer::~TCPServer() {
    Stop();
    MYRPC_SYS_ASSERT(close(m_listen_sock_fd) == 0);

}

void TCPServer::Start() {
    if(!m_acceptor) {
        MYRPC_ASSERT_EXCEPTION(listen(m_listen_sock_fd, SOMAXCONN) == 0, throw NetException("socket listen", NetException::SYS));
        m_fiber_pool->Start();
        auto [fiber, tid] = m_fiber_pool->Run(std::bind(&TCPServer::_do_accept, this));
        m_acceptor = fiber;
        m_acceptor_thread_id = tid;
    }
}

void TCPServer::Stop() {
    if(m_acceptor){
        Fiber::ptr tmp = nullptr;
        std::swap(m_acceptor, tmp);
        m_fiber_pool->Run([tmp]{tmp->Term();},m_acceptor_thread_id);
    }
}

bool TCPServer::bind() noexcept {
    if(::bind(m_listen_sock_fd, m_bind_addr->GetAddr(), m_bind_addr->GetAddrLen())==0) {
        return true;
    }
    return false;
}

void TCPServer::_do_accept() {
    while(true) {
        int sockfd = accept_timeout(m_listen_sock_fd, nullptr, nullptr, m_timeout);
        if(sockfd < 0) {
            if(sockfd == MYRPC_ERR_TIMEOUT_FLAG){ // 超时
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("Thread: {}, Fiber: {}: TCPServer::_do_accept() accept timeout", FiberPool::GetCurrentThreadId(), Fiber::GetCurrentId());
#endif
                continue;
            }
            if(errno == EINTR) continue;
            else break;
        }

        int tid = -1;
        Socket::ptr sock = std::make_shared<Socket>(sockfd);
        TCPServerConn* conn = nullptr;
        if(m_conn_establish_handler) {
            m_conn_establish_handler(&conn);
            sock->m_destructor = [this, conn](){
                m_conn_close_handler(conn);
            };
        }
        for(auto& func:m_conn_handler){
            if(tid == -1) {
                auto [fiber, _tid] = m_fiber_pool->Run([func, sock, conn] { return func(sock, conn); });
                tid = _tid;
                if(conn)
                    conn->m_active_handler.emplace_back(fiber);
            }else{
                auto [fiber, _tid] = m_fiber_pool->Run([func, sock, conn] { return func(sock, conn); }, tid);
                if(conn)
                    conn->m_active_handler.emplace_back(fiber);
            }
        }
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
        auto clientAddr = sock->GetPeerAddr();
        Logger::info("Thread: A new connection form IP:{}, port:{}, connection fd:{}", tid,
                     clientAddr->GetIP(), clientAddr->GetPort(), sock->GetSocketfd());
#endif
    }
}

