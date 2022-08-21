#include "fiber/synchronization.h"
#include "rpc/rpc_registry_server.h"
#include "rpc/protocol.h"

#include <unistd.h>
#include <string>

using namespace MyRPC;

RpcRegistryServer::RpcRegistryServer(const Config::ptr& config) :TCPServer(config->GetThreadsNum(), config->GetTimeout(),
                                                                    config->IsIPv6()), m_keepalive(config->GetKeepalive()){
    //TODO
}

void RpcRegistryServer::handleConnection(const Socket::ptr& sock) {
    TCPServer::handleConnection(sock);

    Protocol proto(*sock);
    FiberSync::Mutex wait_subtask_exit_mutex;
    wait_subtask_exit_mutex.lock();

    std::atomic<bool> heartbeat_flag = {false};
    std::atomic<bool> heartbeat_stopped_flag = false;

    m_fiberPool->Run([ this, &heartbeat_flag, &heartbeat_stopped_flag, &wait_subtask_exit_mutex](){
        do{
            sleep(m_keepalive);
        }while(heartbeat_flag.exchange(false));
        heartbeat_stopped_flag = true;
        wait_subtask_exit_mutex.unlock();
    });

    while (true) {
        auto message_type = proto.ParseHeader();

        if(message_type >= Protocol::ERROR){
            if (heartbeat_stopped_flag) {
                // 心跳停止
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::debug("Connection socket fd {}: heartbeat package timeout!", sock->GetSocketfd());
#endif
                break;
            }
        }

        switch(message_type){
            case Protocol::MESSAGE_HEARTBEAT:
                heartbeat_flag = true;
                break;
            case Protocol::MESSAGE_REQUEST_SUBSCRIBE:
                heartbeat_flag = true;
                handleMessageRequestSubscribe(proto);
                break;
            case Protocol::MESSAGE_REQUEST_REGISTRATION:
                heartbeat_flag = true;
                handleMessageRequestRegistration(proto);
                break;
            default:
                // TODO
                ;
        }
    }

    // 等待子线程退出
    heartbeat_flag = false;
    wait_subtask_exit_mutex.lock();
}

void RpcRegistryServer::handleMessageRequestSubscribe(Protocol& proto) {
    // 从客户端接收订阅的服务名称 格式：std::vector<std::string>，服务名称的数组
    std::vector<std::string> service_name_array;
    proto.ParseContent(service_name_array);
    // TODO 返回提供相应服务的服务器IP地址 格式：std::map<std::string, std::vector<InetAddr>>，服务名称 -> 服务器IP地址的数组
    std::map<std::string, std::vector<InetAddr::ptr>> service_ip;
    for(auto service_name: service_name_array){

    }
}

void RpcRegistryServer::handleMessageRequestRegistration(Protocol &proto) {
    // TODO 从客户端接收注册的服务名称 格式：std::vector<std::string>，服务名称的数组

    // TODO 返回OK应答

    // TODO 向订阅了相应服务的客户端返回新注册的服务
}
