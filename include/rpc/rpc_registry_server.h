#ifndef MYRPC_RPCREGISTRY_H
#define MYRPC_RPCREGISTRY_H

#include "net/tcp_server.h"
#include "rpc/config.h"
#include <memory>

namespace MyRPC{
    class RpcRegistryServer:public TCPServer{
    public:
        using ptr = std::shared_ptr<RpcRegistryServer>;
        explicit RpcRegistryServer(const Config::ptr& config);
    protected:
        void handleConnection(const Socket::ptr& sock) override;
    };
}

#endif //MYRPC_RPCREGISTRY_H
