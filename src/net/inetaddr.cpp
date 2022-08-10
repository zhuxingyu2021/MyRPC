#include "net/inetaddr.h"
#include <cstring>
#include <arpa/inet.h>

#include "logger.h"

using namespace MyRPC;

InetAddr::InetAddr(const std::string& ip, uint16_t port, bool ipv6): m_ipv6(ipv6) {
    if(ipv6){
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        if(inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) == 1){
            addr6_.sin6_port = htons(port);
        }else{
            throw InetInvalidAddrException("invalid ipv6 address " + ip + "!");
        }
    }else{ // IPV4
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        if(inet_aton(ip.c_str(), &addr_.sin_addr) == 1){
            addr_.sin_port = htons(port);
        }else{
            throw InetInvalidAddrException("invalid ipv4 address " + ip + "!");
        }
    }
}

const sockaddr* InetAddr::GetAddr() const{
    return reinterpret_cast<const sockaddr *>(&addr6_);
}

const socklen_t InetAddr::GetAddrLen() const {
    return (m_ipv6) ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}
