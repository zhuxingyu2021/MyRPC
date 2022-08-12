#include "rpc/rpc_registry_server.h"

using namespace MyRPC;

RpcRegistryServer::RpcRegistryServer(Config::ptr config) :TCPServer(config->GetThreadsNum(), config->GetTimeout(),
                                                                    config->IsIPv6()){
    //TODO
}

void RpcRegistryServer::handleConnection(Socket::ptr sock) {
    // TODO
}
