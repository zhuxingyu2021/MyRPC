#include <sys/types.h>
#include <sys/socket.h>

#include <utility>
#include <functional>

#include "fiber/fiber.h"
#include "fiber/fiberpool.h"
#include "net/tcpserver.h"
#include "macro.h"
#include "fiber/timeoutio.h"

using namespace MyRPC;

TCPServer::TCPServer(FiberPool::ptr fiberPool,useconds_t accept_timeout, bool ipv6) : m_fiberPool(fiberPool), m_running(false), m_ipv6(ipv6),
    m_acceptor_con_timeout(accept_timeout){
    m_listen_sock_fd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_SYS_ASSERT(m_listen_sock_fd >= 0);
}

TCPServer::~TCPServer() {
    if(m_running) {
        Logger::error("TCPServer::~TCPServer() - server is still running");
        Stop();
    }
    close(m_listen_sock_fd);
}

bool TCPServer::Start() {
    if(m_running) return false;

    MYRPC_SYS_ASSERT(listen(m_listen_sock_fd, SOMAXCONN) == 0);
    m_acceptor_fiber_con = m_fiberPool->Run(std::bind(&TCPServer::doAccept, this));

    m_running = true;

    return true;
}

void TCPServer::Stop() {
    if(m_running){
        m_stopping = true;
        m_acceptor_fiber_con->Join();
        m_running = false;
    }
}

bool TCPServer::Bind(InetAddr::ptr addr) {
    if(bind(m_listen_sock_fd, addr->GetAddr(), addr->GetAddrLen())==0) {
        return true;
    }
    return false;
}

void TCPServer::doAccept() {
    while(!m_stopping) {
        int sockfd = accept_timeout(m_listen_sock_fd, nullptr, nullptr, m_acceptor_con_timeout);
        if(sockfd < 0) {
            if(sockfd == -2){ // 超时
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("Thread: {}, Fiber: {}: TCPServer::doAccept() accept timeout", FiberPool::GetCurrentThreadId(), Fiber::GetCurrentId());
#endif
                continue;
            }
            if(errno == EINTR) continue;
            else break;
        }
        m_conn_sock_fds.insert(sockfd);
        m_fiberPool->Run([this, sockfd](){
            handleConnection(sockfd);
            close(sockfd);m_conn_sock_fds.erase(sockfd);
        });
    }
}

void TCPServer::handleConnection(int sockfd) {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
    auto clientAddr = InetAddr::GetPeerAddr(sockfd);
    Logger::info("Thread: {}, Fiber: {}: A new connection form IP:{}, port:{}", FiberPool::GetCurrentThreadId(), Fiber::GetCurrentId(),
                 clientAddr->GetIP(), clientAddr->GetPort());
#endif
}

