#ifndef MYRPC_INETADDR_H
#define MYRPC_INETADDR_H

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <netinet/in.h>

namespace MyRPC{
    class InetAddr{
    public:
        using ptr = std::shared_ptr<InetAddr>;
        InetAddr(const std::string& ip, uint16_t port, bool ipv6=false);

        /**
         * @brief 获得客户端的ip地址
         * @param sockfd socket描述符
         * @return 客户端的ip地址
         */
        static InetAddr::ptr GetPeerAddr(int sockfd);

        const std::string GetIP() const;
        const uint16_t GetPort() const;

        const sockaddr* GetAddr() const;
        const socklen_t GetAddrLen() const;

        const bool IsIPv6() const{return m_ipv6;}
    private:
        InetAddr(){}

        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool m_ipv6;
    };
}

#endif //MYRPC_INETADDR_H
