#include "fiber/synchronization.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_session.h"
#include "rpc/exception.h"

#include <unordered_set>

using namespace MyRPC;

RPCServer::RPCServer(Config::ptr config) :TCPServer(config->GetThreadsNum(), 1000*config->GetTimeout(),
                                                           config->IsIPv6()), m_keepalive(config->GetKeepalive()),
                                                 m_timeout(1000*config->GetTimeout()), m_registry_server_ip(config->GetRegistryServerIP()),
                                                m_registry(this){
}

void RPCServer::handleConnection(const Socket::ptr &sock) {
    TCPServer::handleConnection(sock);

    RPCSession proto(*sock, m_timeout);

    FiberSync::Mutex wait_subtask_exit_mutex; // 用于等待心跳检测子协程退出
    wait_subtask_exit_mutex.lock();

    std::atomic<bool> heartbeat_flag = {false}; // 若该变量为true，表示已接收到客户端的请求/心跳包
    std::atomic<bool> heartbeat_stopped_flag = false; // 若该变量为true，表示心跳包超时，主动关闭当前连接

    // 创建心跳检测子协程，用于检测心跳包是否超时，若超时则将heartbeat_stopped_flag设为true并退出
    m_fiberPool->Run([ this, &heartbeat_flag, &heartbeat_stopped_flag, &wait_subtask_exit_mutex](){
        do{
            sleep(m_keepalive);
        }while(heartbeat_flag.exchange(false)); // 如果在m_keepalive秒内接收到心跳包，就再等待m_keepalive秒
        heartbeat_stopped_flag = true;
        wait_subtask_exit_mutex.unlock();
    });

    while (!IsStopping()) {
        MessageType message_type;
        try {
            message_type = proto.RecvAndParseHeader();
        }catch(std::exception& e){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Internal Error: {}, connection form IP: {}, port: {}", e.what(), proto.GetPeerIP()->GetIP(),
                         proto.GetPeerIP()->GetPort());
            message_type = ERROR_OTHERS;
#endif
            break;
        }

        switch(message_type){
            default:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::info("A connection form IP: {}, port: {} is closed because of internal error", proto.GetPeerIP()->GetIP(),
                             proto.GetPeerIP()->GetPort());
#endif
                goto end_loop;
        }

    }

    end_loop:
    // 等待心跳检测子协程退出
    heartbeat_flag = false;
    wait_subtask_exit_mutex.lock();
}


void RPCServer::RegistryClientSession::doConnect() {
    m_connection_handler_thread_id = Fiber::GetCurrentId();

    RPCSession proto(*m_sock, m_server->m_timeout);

    FiberSync::Mutex wait_subtask_exit_mutex; // 用于等待子协程退出
    wait_subtask_exit_mutex.lock();
    bool kill_subtask = false;
    m_server->m_fiberPool->Run([this, &proto, &kill_subtask, &wait_subtask_exit_mutex](){
        usleep(m_server->m_keepalive * 800000);
        while(!kill_subtask){
            proto.PrepareAndSend(MESSAGE_HEARTBEAT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Registry server: IP: {}, port: {}, heartbeats package have already sent",
                         proto.GetPeerIP()->GetIP(),
                         proto.GetPeerIP()->GetPort());
#endif

            usleep(m_server->m_keepalive * 800000);
        }
        wait_subtask_exit_mutex.unlock();
    }, FiberPool::GetCurrentThreadId());

    while(!m_server->IsStopping()) {

        std::unordered_set<std::string> new_service;
        {
            m_service_queue_mutex.lock();
            while (m_service_queue.empty()) { // 如果没有新的服务需要被注册，就等待直到有新的服务到来
                m_service_queue_mutex.unlock();
                Fiber::Suspend();
                m_service_queue_mutex.lock();
            }
            for (const auto &service: m_service_queue) new_service.emplace(service);
            m_service_queue.clear();

            m_service_queue_mutex.unlock();
        }

        proto.PrepareAndSend(MESSAGE_REQUEST_REGISTRATION, new_service);

        auto message_type = proto.RecvAndParseHeader();
        switch(message_type){
            case MESSAGE_RESPOND_OK:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Connection to registry server: IP: {}, port: {}, register service: {} OK!",
                         proto.GetPeerIP()->GetIP(),
                         proto.GetPeerIP()->GetPort(), JsonSerializer::ToString(new_service));
#endif
                break;
            default:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::error("Connection to registry server: IP: {}, port: {}, error message:{}",
                              proto.GetPeerIP()->GetIP(),
                              proto.GetPeerIP()->GetPort(), ToString(message_type));
#endif
                goto do_connect_end_while;
        }
    }

    do_connect_end_while:
    kill_subtask = true;
    m_connection_handler_thread_id = -1;
    m_connection_closed.store(true, std::memory_order_release);
    wait_subtask_exit_mutex.lock();

    if(!m_server->IsStopping()) {
        do {
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::error("Connection to registry server: IP: {}, port: {} is closed. Retrying ...",
                          proto.GetPeerIP()->GetIP(),
                          proto.GetPeerIP()->GetPort());
#endif
        } while (!m_server->IsStopping() && !Connect());
    }
}

void RPCServer::RegistryClientSession::Update(std::string_view service_name) {
    std::unique_lock<SpinLock> lock(m_service_queue_mutex);

    m_service_queue.emplace_back(service_name);
    if (m_service_queue.empty() && m_connection_handler_thread_id >= 0) {
        m_server->m_fiberPool->Notify(m_connection_handler_thread_id);
    }
}
