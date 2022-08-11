#include "net/inetaddr.h"
#include <cstring>
#include <arpa/inet.h>

#include "logger.h"
#include "macro.h"

#include "net/exception.h"

using namespace MyRPC;

InetAddr::InetAddr(const std::string& ip, uint16_t port, bool ipv6): m_ipv6(ipv6) {
    if(ipv6){
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        MYRPC_ASSERT_EXCEPTION(inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) == 1, throw InetInvalidAddrException("invalid ipv6 address " + ip + "!"));
    }else{ // IPV4
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        MYRPC_ASSERT_EXCEPTION(inet_aton(ip.c_str(), &addr_.sin_addr) == 1, throw InetInvalidAddrException("invalid ipv4 address " + ip + "!"));
    }
}

const sockaddr* InetAddr::GetAddr() const{
    return reinterpret_cast<const sockaddr *>(&addr6_);
}

const socklen_t InetAddr::GetAddrLen() const {
    return (m_ipv6) ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

InetAddr::ptr InetAddr::GetPeerAddr(int sockfd) {
    InetAddr::ptr return_val(new InetAddr);

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if(getpeername(sockfd, (struct sockaddr*)&addr, &len) == 0){
        if(addr.ss_family == AF_INET){
            return_val->m_ipv6 = false;
            return_val->addr_.sin_family = AF_INET;
            return_val->addr_.sin_addr = ((struct sockaddr_in*)&addr)->sin_addr;
            return_val->addr_.sin_port = ((struct sockaddr_in*)&addr)->sin_port;
        }else if(addr.ss_family == AF_INET6){
            return_val->m_ipv6 = true;
            return_val->addr6_.sin6_family = AF_INET6;
            return_val->addr6_.sin6_addr = ((struct sockaddr_in6*)&addr)->sin6_addr;
            return_val->addr6_.sin6_port = ((struct sockaddr_in6*)&addr)->sin6_port;
        }
        return return_val;
    }else{
        return nullptr;
    }
}

const uint16_t InetAddr::GetPort() const {
    return (m_ipv6) ? ntohs(addr6_.sin6_port) : ntohs(addr_.sin_port);
}

const std::string InetAddr::GetIP() const {
    char buf[INET6_ADDRSTRLEN];
    if(m_ipv6){
        if(inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf)) == nullptr){
            return "";
        }
    }else{
        if(inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)) == nullptr){
            return "";
        }
    }
    return buf;
}

