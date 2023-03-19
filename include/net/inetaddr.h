#ifndef MYRPC_INETADDR_H
#define MYRPC_INETADDR_H

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <netinet/in.h>

#include "serialization/json_serializer.h"
#include "serialization/json_deserializer.h"

#include "noncopyable.h"

namespace MyRPC{
    class InetAddr{
    public:
        using ptr = std::shared_ptr<InetAddr>;
        InetAddr() = default;

        InetAddr(const std::string& ip, uint16_t port, bool ipv6=false);
        InetAddr(const InetAddr&) = default;

        bool operator==(const InetAddr& operand) const{
            if(m_ipv6 == operand.m_ipv6){
                if(!m_ipv6){
                    return (memcmp(&addr_, &operand.addr_, sizeof(addr_))==0);
                }else{
                    return (memcmp(&addr6_, &operand.addr6_, sizeof(addr6_))==0);
                }
            }
            return false;
        }

        bool operator<(const InetAddr& operand) const{
            if(m_ipv6 == operand.m_ipv6){
                if(!m_ipv6){
                    return (memcmp(&addr_, &operand.addr_, sizeof(addr_))<0);
                }else{
                    return (memcmp(&addr6_, &operand.addr6_, sizeof(addr6_))<0);
                }
            }
            return operand.m_ipv6;
        }

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

        static std::string BuildString(const std::string& ip_str, int port){
            return ip_str + ":" + std::to_string(port);
        }
        std::string ToString() const{return BuildString(m_ip_str, m_port);}

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
