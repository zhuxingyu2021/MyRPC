#ifndef MYRPC_TCP_CLIENT_H
#define MYRPC_TCP_CLIENT_H

#include "net/inetaddr.h"
#include "net/socket.h"
#include "net/tcp_client.h"
#include "fiber/timeout_io.h"

#include "net/exception.h"
#include "macro.h"

#include <unistd.h>
#include <memory>

namespace MyRPC{
template<class T>
class TCPClient: public std::enable_shared_from_this<TCPClient<T>>{
public:
    using ptr = std::shared_ptr<TCPClient<T>>;
    TCPClient() = delete;
    TCPClient(const TCPClient&) = delete;

    /**
     * @brief 与IP地址是addr的服务器建立连接
     * @param addr 服务器IP地址
     * @param conn_timeout 超时时间，单位微秒，0表示不设置超时时间
     * @return 如果与服务器之间的连接建立成功，返回TCPClient对象指针；如果连接超时，返回空指针。如果因为超时以外的原因导致连接失败，抛出SocketException异常。
     */
    static TCPClient<T>::ptr connect(InetAddr::ptr addr, __useconds_t conn_timeout=0){
        auto sock_fd = socket(addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
        MYRPC_ASSERT_EXCEPTION(sock_fd >= 0, throw SocketException("TCPClient socket creation"));

        auto err = connect_timeout(sock_fd, addr->GetAddr(), addr->GetAddrLen(), conn_timeout);
        if(err >= 0){
            return TCPClient<T>::ptr(new T(sock_fd));
        }else if(err == MYRPC_ERR_TIMEOUT_FLAG){ // 超时
            return nullptr;
        }else{
            throw SocketException("socket connect");
        }
    }

    template<class... Args>
    void doConnect(Args... args){
#if MYRPC_DEBUG_LEVEL >= MMYRPC_DEBUG_NET_LEVEL
        auto serverAddr = InetAddr::GetPeerAddr(sock->GetSocketfd());
        Logger::debug("A new connection to IP:{}, port:{}, connection fd:{}", serverAddr->GetIP(), serverAddr->GetPort(), sock->GetSocketfd());
#endif
        static_cast<T*>(this)->doConnect(std::forward<Args>(args)...);
    }
protected:
    Socket::unique_ptr sock;

    TCPClient(int sockfd):sock(std::make_unique<Socket>(sockfd)){}
};
}

#endif //MYRPC_TCP_CLIENT_H
