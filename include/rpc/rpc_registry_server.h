#ifndef MYRPC_RPCREGISTRY_H
#define MYRPC_RPCREGISTRY_H

#include "net/tcp_server.h"
#include "net/inetaddr.h"
#include "rpc/config.h"
#include "rpc/rpc_session.h"

#include "fiber/synchronization.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

namespace MyRPC{
    class RpcRegistryServer:public TCPServer{
    public:
        using ptr = std::shared_ptr<RpcRegistryServer>;
        explicit RpcRegistryServer(const Config::ptr& config);
    protected:
        void handleConnection(const Socket::ptr& sock) override;

    private:
        int m_keepalive;
        useconds_t m_timeout;

        // 服务提供者表
        // Service Name -> Service Provider 1 (IP)
        //              -> Service Provider 2 (IP)
        //              -> ...
        std::unordered_multimap<std::string, InetAddr::ptr> m_service_provider_map;

        FiberSync::RWMutex m_service_provider_map_mutex; // 用于保护m_service_provider_map变量的读写锁

        // 服务订阅者表
        // Service Name -> Subscriber 1 (IP)
        //              -> Subscriber 2 (IP)
        //              -> ...
        std::unordered_multimap<std::string, InetAddr::ptr> m_service_subscriber_map;

        FiberSync::RWMutex m_service_subscriber_map_mutex; // 用于保护m_service_subscriber_map变量的读写锁

        // 会话表（在会话开始时创建表项，关闭前删除相应表项，表项生命周期与会话生命周期相同）
        // IP(String) -> RPCSession
        std::unordered_map<std::string, RPCSession*> m_peer_ip_session_map;
        FiberSync::RWMutex m_peer_ip_session_map_mutex;

        void handleMessageRequestSubscribe(RPCSession &proto, std::vector<decltype(m_service_subscriber_map)::iterator>&);
        void handleMessageRequestRegistration(RPCSession &proto, std::vector<decltype(m_service_provider_map)::iterator>&);

        void PushRegistryInfo(std::unordered_set<std::string>& service_name_set);
    };
}

#endif //MYRPC_RPCREGISTRY_H
