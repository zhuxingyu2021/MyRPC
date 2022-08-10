#include "net/tcpsocket.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "macro.h"

using namespace MyRPC;

TCPSocket::TCPSocket(bool ipv6) {
    m_sockfd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    MYRPC_SYS_ASSERT(m_sockfd >= 0);
}

TCPSocket::~TCPSocket() {
    close(m_sockfd);
}

bool TCPSocket::Bind(InetAddr::ptr addr) {
    if(::bind(m_sockfd, addr->GetAddr(), addr->GetAddrLen())==0) {
        return true;
    }
    return false;
}


