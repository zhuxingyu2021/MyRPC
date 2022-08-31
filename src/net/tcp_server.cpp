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
#include <csignal>

using namespace MyRPC;

int MyRPC::_sigint_handler_initializer = [](){
    // 捕捉SIGINT事件，使得程序在Ctrl-C事件发生之后能正常关闭TCPServer
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = TCPServer::handleSIGINT;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    return 0;
}();


TCPServer::TCPServer(const InetAddr::ptr& bind_addr, int thread_num, useconds_t timeout) : m_fiberPool(std::make_shared<FiberPool>(thread_num)),
                                                                                           m_running(false), m_timeout(timeout), m_acceptor(nullptr), m_bind_addr(bind_addr){
    m_listen_sock_fd = socket(bind_addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_ASSERT_EXCEPTION(m_listen_sock_fd >= 0, throw SocketException("TCPServer socket creation"));

    m_avail_server.push_back(this);
}

TCPServer::TCPServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, useconds_t timeout) : m_fiberPool(fiberPool), m_running(false),
                                                                                                     m_timeout(timeout), m_acceptor(nullptr), m_bind_addr(bind_addr){
    m_listen_sock_fd = socket(bind_addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_ASSERT_EXCEPTION(m_listen_sock_fd >= 0, throw SocketException("TCPServer socket creation"));

    m_avail_server.push_back(this);
}

TCPServer::~TCPServer() {
    Stop();
    MYRPC_SYS_ASSERT(close(m_listen_sock_fd) == 0);

    auto iter = std::find(m_avail_server.begin(), m_avail_server.end(),this);
    if(iter != m_avail_server.end()){
        m_avail_server.erase(iter);
    }else{
        Logger::critical("Closing a tcp server which haven't been registered!");
    }
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

bool TCPServer::bind() noexcept {
    if(::bind(m_listen_sock_fd, m_bind_addr->GetAddr(), m_bind_addr->GetAddrLen())==0) {
        return true;
    }
    return false;
}

void TCPServer::doAccept() {
    while(!IsStopping()) {
        int sockfd = accept_timeout(m_listen_sock_fd, nullptr, nullptr, m_timeout);
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

void TCPServer::handleSIGINT(int sig) {
    Logger::info("Caught SIGINT: {}", sig);

    for(auto server_ptr: m_avail_server){
        server_ptr->~TCPServer();
    }

    exit(-1);
}
