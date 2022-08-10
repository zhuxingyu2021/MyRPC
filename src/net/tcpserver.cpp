#include "net/tcpserver.h"

#include <utility>

bool MyRPC::TCPServer::Start() {
    if(!m_stopped) return false;
}

void MyRPC::TCPServer::Stop() {

}

bool MyRPC::TCPServer::Bind(MyRPC::InetAddr::ptr addr) {
    return m_listen_sock.Bind(addr);
}
