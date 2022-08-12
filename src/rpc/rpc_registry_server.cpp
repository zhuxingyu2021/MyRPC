#include "rpc/rpc_registry_server.h"
#include "rpc/protocol.h"

using namespace MyRPC;

RpcRegistryServer::RpcRegistryServer(const Config::ptr& config) :TCPServer(config->GetThreadsNum(), config->GetTimeout(),
                                                                    config->IsIPv6()){
    //TODO
}

void RpcRegistryServer::handleConnection(const Socket::ptr& sock) {
    // TODO
}
