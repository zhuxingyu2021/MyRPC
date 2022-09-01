#include "rpc/rpc_client.h"
#include <queue>

using namespace MyRPC;

RPCClient::RPCClient(Config::ptr config) :m_fiber_pool(std::make_shared<FiberPool>(config->GetThreadsNum())),
m_timeout(1000*config->GetTimeout()), m_keepalive(config->GetKeepalive()), m_registry(config->GetRegistryServerAddr(), *this){
}


void RPCClient::RegistryClientSession::handleConnect() {
    TCPClient::handleConnect();
    std::multimap<std::string, ServiceQueueNode::ptr> wait_recv_queue;

    m_connection_handler_thread_id = FiberPool::GetCurrentThreadId();

    RPCSession proto(*m_sock, m_timeout);

    FiberSync::Mutex wait_subtask_exit_mutex; // 用于等待子协程退出
    wait_subtask_exit_mutex.lock();
    bool kill_subtask = false;
    // 开启子协程，定时发送Heartbeat包
    m_fiber_pool->Run([this, &proto, &kill_subtask, &wait_subtask_exit_mutex](){
        usleep(m_keepalive * 800000);
        while(!kill_subtask){
            proto.PrepareAndSend(MESSAGE_HEARTBEAT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Register server: IP: {}, port: {}, heartbeats package have already sent",
                         m_server_addr->GetIP(), m_server_addr->GetPort());
#endif

            usleep(m_keepalive * 800000);
        }
        wait_subtask_exit_mutex.unlock();
    }, m_connection_handler_thread_id);

    FiberSync::Mutex wait_subscribe_task_exit_mutex; // 用于等待订阅子协程退出
    wait_subscribe_task_exit_mutex.lock();
    // 开启子协程，不断向注册服务器发送订阅消息
    m_fiber_pool->Run([this, &kill_subtask, &wait_subscribe_task_exit_mutex, &proto, &wait_recv_queue](){
        while(!kill_subtask){
            std::unordered_set<std::string> new_service;
            {
                std::unique_lock<SpinLock> spin_lock(m_service_queue_mutex);
                while (!kill_subtask && m_service_queue.empty()) { // 如果没有新的服务需要被订阅，就等待直到有新的服务到来
                    spin_lock.unlock();
                    Fiber::Suspend();
                    spin_lock.lock();
                }
                if(kill_subtask) break;

                for(const auto & node_ptr : m_service_queue){
                    new_service.emplace(node_ptr->m_service_name);
                    wait_recv_queue.emplace(node_ptr->m_service_name, node_ptr);
                }
                m_service_queue.clear();
            }
            proto.PrepareAndSend(MESSAGE_REQUEST_SUBSCRIBE, new_service);

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Connection to registry server: IP: {}, port: {}, have sent message to register service: {}",
                         m_server_addr->GetIP(), m_server_addr->GetPort(), JsonSerializer::ToString(new_service));
#endif
        }
        wait_subscribe_task_exit_mutex.unlock();
    }, m_connection_handler_thread_id);

    while(!IsClosing()) {

        MessageType message_type = proto.RecvAndParseHeader();

        switch(message_type){
            case MESSAGE_RESPOND_OK:
            case MESSAGE_PUSH:
            {
                // 读取服务器发送过来的数据
                std::tuple<std::unordered_set<std::string>, std::multimap<std::string, InetAddr::ptr>> result;
                proto.ParseContent(result);

                auto& service_name_set = std::get<0>(result);
                auto& service_addr_map = std::get<1>(result);

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::info("Connection to registry server: IP: {}, port: {}, update service: {} with message type:{}",
                             m_server_addr->GetIP(), m_server_addr->GetPort(), JsonSerializer::ToString(service_name_set),
                             ToString(message_type));
#endif

                // 修改服务提供者表
                {
                    std::unique_lock<FiberSync::RWMutex> lock(m_client.m_service_table_mutex);
                    for(const auto& service_name: service_name_set){
                        // 先删除服务提供者表的相应服务
                        if(m_client.m_service_table.find(service_name) != m_client.m_service_table.end()){
                            m_client.m_service_table.erase(service_name);
                        }

                        auto service_range = service_addr_map.equal_range(service_name);
                        MYRPC_ASSERT(service_range.first != service_range.second);
                        if(service_range.first->second) {
                            // 服务service_name 存在

                            // 更新服务提供者表
                            for (auto service_iter = service_range.first; service_iter != service_range.second; ++service_iter) {
                                m_client.m_service_table.emplace(std::move(service_iter->first), std::move(service_iter->second));
                            }

                            // 唤醒等待协程
                            auto wait_range = wait_recv_queue.equal_range(service_name);
                            if(wait_range.first != wait_range.second) {
                                for (auto wait_iter = wait_range.first; wait_iter != wait_range.second; ++wait_iter) {
                                    wait_iter->second->is_exist = true;
                                    wait_iter->second->wait_mutex.unlock();
                                }

                                wait_recv_queue.erase(wait_range.first, wait_range.second);
                            }
                        }else{
                            // 服务service_name 不存在

                            // 唤醒等待协程
                            auto wait_range = wait_recv_queue.equal_range(service_name);
                            if(wait_range.first != wait_range.second) {
                                for (auto wait_iter = wait_range.first; wait_iter != wait_range.second; ++wait_iter) {
                                    wait_iter->second->wait_mutex.unlock();
                                }

                                wait_recv_queue.erase(wait_range.first, wait_range.second);
                            }
                        }
                    }
                }
            }
            break;

            default:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::error("Connection to registry server: IP: {}, port: {}, error message:{}",
                              m_server_addr->GetIP(), m_server_addr->GetPort(), ToString(message_type));
#endif
                goto do_connect_end_while;
        }
    }

    do_connect_end_while:
    kill_subtask = true;
    m_connection_handler_thread_id = -1;
    wait_subtask_exit_mutex.lock();
    m_connection_closed = true;

    if(!IsClosing()) {
        do {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::error("Connection to registry server: IP: {}, port: {} is closed. Retrying ...",
                          m_server_addr->GetIP(), m_server_addr->GetPort());
#endif
        } while (!IsClosing() && !Connect());
    }
}

bool RPCClient::RegistryClientSession::Query(std::string_view service_name) {
    if(!IsClosed()) {
        std::unique_lock<SpinLock> lock(m_service_queue_mutex);
        bool is_queue_empty = m_service_queue.empty();
        auto service = m_service_queue.emplace_back(std::make_shared<ServiceQueueNode>(service_name));
        lock.unlock();

        if (is_queue_empty && m_connection_handler_thread_id >= 0) {
            m_fiber_pool->Notify(m_connection_handler_thread_id);
        }

        service->wait_mutex.lock();
        service->wait_mutex.lock();

        return service->is_exist;
    }
    return false;
}
