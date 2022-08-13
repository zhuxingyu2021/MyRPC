#include <sys/types.h>
#include <sys/socket.h>

#include <utility>
#include <functional>

#include "fiber/fiber.h"
#include "fiber/fiber_pool.h"
#include "net/tcp_server.h"
#include "macro.h"
#include "fiber/timeout_io.h"

#include "net/exception.h"

using namespace MyRPC;

TCPServer::TCPServer(int thread_num, __useconds_t accept_timeout, bool ipv6) :m_fiberPool(std::make_shared<FiberPool>(thread_num)), m_running(false),
m_ipv6(ipv6), m_acceptor_con_timeout(accept_timeout), m_acceptor(nullptr){
    m_listen_sock_fd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_ASSERT_EXCEPTION(m_listen_sock_fd >= 0, throw SocketException("TCPServer socket creation"));
}

TCPServer::TCPServer(FiberPool::ptr fiberPool, useconds_t accept_timeout, bool ipv6) : m_fiberPool(fiberPool), m_running(false), m_ipv6(ipv6),
    m_acceptor_con_timeout(accept_timeout), m_acceptor(nullptr){
    m_listen_sock_fd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_ASSERT_EXCEPTION(m_listen_sock_fd >= 0, throw SocketException("TCPServer socket creation"));
}

TCPServer::~TCPServer() {
    Stop();
    close(m_listen_sock_fd);
}

void TCPServer::Start() {
    if(!m_running) {

        MYRPC_ASSERT_EXCEPTION(listen(m_listen_sock_fd, SOMAXCONN) == 0, throw SocketException("socket listen"));
        m_acceptor = m_fiberPool->Run(std::bind(&TCPServer::doAccept, this));
        m_fiberPool->Start();

        m_running = true;
    }
}

void TCPServer::Stop() {
    if(m_running){
        m_stopping = true;
        m_fiberPool->Wait();
        m_fiberPool->Stop();
        m_running = false;
    }
}

void TCPServer::StopAccept() {
    if(m_acceptor){
        m_acceptor->Join();
        m_acceptor = nullptr;
    }
}

bool TCPServer::Bind(InetAddr::ptr addr) noexcept {
    if(bind(m_listen_sock_fd, addr->GetAddr(), addr->GetAddrLen())==0) {
        return true;
    }
    return false;
}

void TCPServer::doAccept() {
    while(!m_stopping) {
        int sockfd = accept_timeout(m_listen_sock_fd, nullptr, nullptr, m_acceptor_con_timeout);
        if(sockfd < 0) {
            if(sockfd == MYRPC_ERR_TIMEOUT_FLAG){ // 超时
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("Thread: {}, Fiber: {}: TCPServer::doAccept() accept timeout", FiberPool::GetCurrentThreadId(), Fiber::GetCurrentId());
#endif
                continue;
            }
            if(errno == EINTR) continue;
            else break;
        }
        Socket::ptr sock = std::make_shared<Socket>(sockfd);
        m_fiberPool->Run(std::bind(&TCPServer::handleConnection, this, sock));
    }
}
