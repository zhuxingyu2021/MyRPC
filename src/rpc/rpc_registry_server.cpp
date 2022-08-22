#include "fiber/synchronization.h"
#include "rpc/rpc_registry_server.h"
#include "rpc/rpc_session.h"

#include <unistd.h>
#include <string>

#include <mutex>
#include <shared_mutex>

using namespace MyRPC;

RpcRegistryServer::RpcRegistryServer(const Config::ptr& config) :TCPServer(config->GetThreadsNum(), 1000*config->GetTimeout(),
                                                                    config->IsIPv6()), m_keepalive(config->GetKeepalive()),
                                                                    m_timeout(1000*config->GetTimeout()){
}

void RpcRegistryServer::handleConnection(const Socket::ptr& sock) {
    TCPServer::handleConnection(sock);

    RPCSession proto(*sock, m_timeout);

    std::vector<decltype(m_service_provider_map)::iterator> local_provide_service; // 记录当前客户端提供的服务
    std::vector<decltype(m_service_subscriber_map)::iterator> local_subscribe_service; // 记录当前客户端订阅的服务

    // 将当前会话加入会话表
    std::unordered_map<std::string, RPCSession*>::iterator iter_session; // 当前会话在会话表中的迭代器
    {
        std::unique_lock<FiberSync::RWMutex> lock(m_peer_ip_session_map_mutex);
        auto res = m_peer_ip_session_map.emplace(proto.GetPeerIP()->ToString(), &proto);

        MYRPC_ASSERT(res.second);
        iter_session = res.first;
    }

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

        if(message_type >= RPCSession::ERROR){
            if (heartbeat_stopped_flag) {
                // 心跳停止
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::debug("Connection socket fd {}: heartbeat package timeout!", sock->GetSocketfd());
#endif
                break;
            }
        }

        switch(message_type){
            case RPCSession::MESSAGE_HEARTBEAT:
                heartbeat_flag = true;
                break;
            case RPCSession::MESSAGE_REQUEST_SUBSCRIBE:
                heartbeat_flag = true;
                handleMessageRequestSubscribe(proto, local_subscribe_service);
                break;
            case RPCSession::MESSAGE_REQUEST_REGISTRATION:
                heartbeat_flag = true;
                handleMessageRequestRegistration(proto, local_provide_service);
                break;
            default:
                // TODO
                ;
        }
    }

    // 在会话表中删除当前会话的表项
    {
        std::unique_lock<FiberSync::RWMutex> lock(m_peer_ip_session_map_mutex);
        m_peer_ip_session_map.erase(iter_session);
    }

    // 删除订阅的服务
    if(!local_subscribe_service.empty()){
        std::unique_lock<FiberSync::RWMutex> lock(m_service_subscriber_map_mutex);
        for(auto iter: local_subscribe_service) m_service_subscriber_map.erase(iter);
    }

    // 下线提供的服务
    if(!local_provide_service.empty()){
        std::unordered_set<std::string> local_service;
        {
            std::unique_lock<FiberSync::RWMutex> lock(m_service_provider_map_mutex);
            for (auto iter: local_provide_service) {
                local_service.insert(iter->first);
                m_service_provider_map.erase(iter);
            }
        }
        // Push更新
        PushRegistryInfo(local_service);
    }

    // 等待子线程退出
    heartbeat_flag = false;
    wait_subtask_exit_mutex.lock();
}

void RpcRegistryServer::handleMessageRequestSubscribe(RPCSession& proto, std::vector<decltype(m_service_subscriber_map)::iterator>& local_subscribe_service) {
    // 从客户端接收订阅的服务名称 格式：std::unordered_set<std::string>，服务名称的数组
    std::unordered_set<std::string> service_name_set;
    proto.ParseContent(service_name_set);
    // 返回提供相应服务的服务器IP地址 格式：std::multimap<std::string, InetAddr>，服务名称 -> 服务器IP地址

    // 查询相应服务器的IP地址
    std::multimap<std::string, InetAddr::ptr> map_service_ip;
    for (const auto& service_name: service_name_set) {
        std::shared_lock<FiberSync::RWMutex> lock_shared(m_service_provider_map_mutex);
        auto ret = m_service_provider_map.equal_range(service_name);
        for(auto iter = ret.first; iter != ret.second; ++iter)
            map_service_ip.emplace(iter->first, iter->second);
    }

    // 将查询结果发送给客户端
    proto.Send(RPCSession::MESSAGE_RESPOND_OK, map_service_ip);

    // 订阅机制：把客户端IP加入服务订阅者表
    auto client_ip = proto.GetPeerIP();
    {
        std::unique_lock<FiberSync::RWMutex> lock(m_service_subscriber_map_mutex);
        for(const auto& service_name: service_name_set){
            local_subscribe_service.push_back(m_service_subscriber_map.emplace(service_name, client_ip));
        }
    }
}

void RpcRegistryServer::handleMessageRequestRegistration(RPCSession &proto, std::vector<decltype(m_service_provider_map)::iterator>& local_provide_service) {
    // 从客户端接收注册的服务名称 格式：std::unordered_set<std::string>，服务名称的数组
    std::unordered_set<std::string> service_name_set;
    proto.ParseContent(service_name_set);

    // 把客户端IP加入服务提供者表
    auto client_ip = proto.GetPeerIP();
    {
        std::unique_lock<FiberSync::RWMutex> lock(m_service_provider_map_mutex);
        for(const auto& service_name: service_name_set){
            local_provide_service.push_back(m_service_provider_map.emplace(service_name, client_ip));
        }
    }

    // 返回OK应答
    proto.Send(RPCSession::MESSAGE_RESPOND_OK);

    // 向订阅了相应服务的客户端返回新注册的服务
    PushRegistryInfo(service_name_set);

}

void RpcRegistryServer::PushRegistryInfo(std::unordered_set<std::string> &service_name_set) {
    // 查询服务名对应的服务订阅者
    std::multimap<std::string, std::string> subscriber_service_map; // subscriber IP -> service name
    std::vector<std::string> subscribers; // 服务名对应的订阅者
    for (const auto &service_name: service_name_set) {
        std::shared_lock<FiberSync::RWMutex> shared_lock(m_service_subscriber_map_mutex);

        // 对于更新的服务，查询服务订阅者
        auto ret = m_service_subscriber_map.equal_range(service_name);
        for(auto iter=ret.first; iter!=ret.second; ++iter){
            auto it = subscriber_service_map.emplace(iter->second->ToString(), iter->first);
            subscribers.push_back(it->first);
        }
    }

    // 向每个服务订阅者发送更新服务的服务提供者表
    for(const auto& subscriber_ip: subscribers){
        // 对于每个服务订阅者，获得对应的RPCSession
        std::shared_lock<FiberSync::RWMutex> shared_lock(m_peer_ip_session_map_mutex);
        auto iter_session = m_peer_ip_session_map.find(subscriber_ip);
        if(iter_session != m_peer_ip_session_map.end()){
            auto proto = iter_session->second;
            // 查询更新的服务，并返回更新后的服务器IP地址 格式：std::multimap<std::string, InetAddr>，服务名称 -> 服务器IP地址
            std::multimap<std::string, InetAddr::ptr> map_service_ip;

            auto ret_service = subscriber_service_map.equal_range(subscriber_ip);

            for(auto iter_service = ret_service.first; iter_service!=ret_service.second; ++iter_service){
                std::shared_lock<FiberSync::RWMutex> lock_shared(m_service_provider_map_mutex);

                // 根据服务名查询IP地址
                auto ret_server_ip = m_service_provider_map.equal_range(iter_service->second);
                for(auto iter_server_ip = ret_server_ip.first; iter_server_ip != ret_server_ip.second; ++iter_server_ip)
                    map_service_ip.emplace(iter_server_ip->first, iter_server_ip->second);
            }

            // 发送更新内容给客户端
            proto->Send(RPCSession::MESSAGE_PUSH, map_service_ip);
        }
    }
}
