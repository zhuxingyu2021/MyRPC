#ifndef MYRPC_INETADDR_H
#define MYRPC_INETADDR_H

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <netinet/in.h>

#include "net/serializer.h"
#include "net/deserializer.h"

#include "noncopyable.h"

namespace MyRPC{
    class InetAddr: public NonCopyable{
    public:
        using ptr = std::shared_ptr<InetAddr>;
        InetAddr() = default;

        InetAddr(const std::string& ip, uint16_t port, bool ipv6=false);

        /**
         * @brief 获得客户端的ip地址
         * @param sockfd socket描述符
         * @return 客户端的ip地址
         */
        static InetAddr::ptr GetPeerAddr(int sockfd);

        const std::string& GetIP() const { return m_ip_str; }
        const uint16_t GetPort() const { return m_port; }

        const sockaddr* GetAddr() const;
        const socklen_t GetAddrLen() const;

        const bool IsIPv6() const{return m_ipv6;}

        friend JsonSerializer;
        friend JsonDeserializer;
    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool m_ipv6;
        std:: string m_ip_str;
        int m_port;

        void _construct_from_string();

        LOAD_BEGIN
            LOAD_ALIAS_ITEM(IP, m_ip_str)
            LOAD_ALIAS_ITEM(Port, m_port)
            LOAD_ALIAS_ITEM(IsIPv6, m_ipv6)
            _construct_from_string();
        LOAD_END

        SAVE_BEGIN
            SAVE_ALIAS_ITEM(IP, m_ip_str);
            SAVE_ALIAS_ITEM(Port, m_port)
            SAVE_ALIAS_ITEM(IsIPv6, m_ipv6)
        SAVE_END
    };
}

#endif //MYRPC_INETADDR_H
