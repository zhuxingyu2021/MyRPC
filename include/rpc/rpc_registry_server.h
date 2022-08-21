#ifndef MYRPC_RPCREGISTRY_H
#define MYRPC_RPCREGISTRY_H

#include "net/tcp_server.h"
#include "net/inetaddr.h"
#include "rpc/config.h"
#include "rpc/protocol.h"

#include "fiber/synchronization.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MyRPC{
    class RpcRegistryServer:public TCPServer{
    public:
        using ptr = std::shared_ptr<RpcRegistryServer>;
        explicit RpcRegistryServer(const Config::ptr& config);
    protected:
        void handleConnection(const Socket::ptr& sock) override;

    private:
        int m_keepalive;

        // Service Name -> Service Provider 1
        //              -> Service Provider 2
        //              -> ...
        std::unordered_multimap<std::string, InetAddr::ptr> m_service_provider_map;

        FiberSync::Mutex m_service_provider_map_mutex; // 用于保护m_service_provider_map变量的mutex

        void handleMessageRequestSubscribe(Protocol &proto);
        void handleMessageRequestRegistration(Protocol &proto);
    };
}

#endif //MYRPC_RPCREGISTRY_H
