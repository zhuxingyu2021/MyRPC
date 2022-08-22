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

#include "noncopyable.h"

namespace MyRPC{
template<class Derived>
class TCPClient: public NonCopyable{
public:
    using ptr = std::shared_ptr<TCPClient<Derived>>;
    TCPClient() = delete;

    /**
     * @brief 与IP地址是addr的服务器建立连接
     * @param addr 服务器IP地址
     * @param conn_timeout 超时时间，单位微秒，0表示不设置超时时间
     * @tparam Args 用户可以扩展子类的构造函数，以支持更多的参数
     * @return 如果与服务器之间的连接建立成功，返回TCPClient对象指针；如果连接超时，返回空指针。如果因为超时以外的原因导致连接失败，抛出SocketException异常。
     */
     template<class... Args>
    static TCPClient<Derived>::ptr connect(InetAddr::ptr addr, useconds_t conn_timeout=0, Args... args){
        auto sock_fd = socket(addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
        MYRPC_ASSERT_EXCEPTION(sock_fd >= 0, throw SocketException("TCPClient socket creation"));

        auto err = connect_timeout(sock_fd, addr->GetAddr(), addr->GetAddrLen(), conn_timeout);
        if(err >= 0){
            return TCPClient<Derived>::ptr(new Derived(sock_fd, std::forward<Args>(args)...));
        }else if(err == MYRPC_ERR_TIMEOUT_FLAG){ // 超时
            return nullptr;
        }else{
            throw SocketException("socket connect");
        }
    }

    template<class... Args>
    void doConnect(Args... args){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
        auto serverAddr = sock->GetPeerAddr();
        Logger::debug("A new connection to IP:{}, port:{}, connection fd:{}", serverAddr->GetIP(), serverAddr->GetPort(), sock->GetSocketfd());
#endif
        static_cast<Derived*>(this)->doConnect(std::forward<Args>(args)...);
    }
protected:
    Socket::unique_ptr sock;

    TCPClient(int sockfd):sock(std::make_unique<Socket>(sockfd)){}
};
}

#endif //MYRPC_TCP_CLIENT_H
