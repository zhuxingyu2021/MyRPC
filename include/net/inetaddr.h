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

        const sockaddr* GetAddr() const;
        const socklen_t GetAddrLen() const;
    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool ipv6_;
    };
}

#endif //MYRPC_INETADDR_H
