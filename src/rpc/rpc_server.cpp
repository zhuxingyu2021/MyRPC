#include "fiber/fiber_sync.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_session.h"
#include "rpc/exception.h"

#include <unordered_set>
#include <shared_mutex>

using namespace MyRPC;

RPCServer::RPCServer(InetAddr::ptr bind_addr, Config::ptr config) :TCPServer(bind_addr, config->GetThreadsNum(),
                                                                             1000*config->GetTimeout()), m_keepalive(config->GetKeepalive()),
                                                                             m_registry(config->GetRegistryServerAddr(),
                                                                             m_fiber_pool, 1000*config->GetTimeout(), m_keepalive){
}

void RPCServer::handleConnection(const Socket::ptr &sock) {
    TCPServer::handleConnection(sock);

    RPCSession proto(*sock, m_timeout);

    FiberSync::Mutex wait_subtask_exit_mutex; // 用于等待心跳检测子协程退出
    wait_subtask_exit_mutex.lock();

    std::atomic<bool> heartbeat_flag = {false}; // 若该变量为true，表示已接收到客户端的请求/心跳包
    std::atomic<bool> heartbeat_stopped_flag = false; // 若该变量为true，表示心跳包超时，主动关闭当前连接

    // 创建心跳检测子协程，用于检测心跳包是否超时，若超时则将heartbeat_stopped_flag设为true并退出
    m_fiber_pool->Run([ this, &heartbeat_flag, &heartbeat_stopped_flag, &wait_subtask_exit_mutex](){
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
            case MESSAGE_REQUEST_RPC:
                handleMessageRequestRPC(proto);
                break;
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


void RPCServer::RegistryClientSession::handleConnect() {
    TCPClient::handleConnect();

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
    }, FiberPool::GetCurrentThreadId());

    while(!IsClosing()) {

        std::unordered_map<std::string, uint16_t> new_service;
        {
            std::unique_lock<SpinLock> spin_lock(m_service_queue_mutex);
            while (!IsClosing() && m_service_queue.empty()) { // 如果没有新的服务需要被注册，就等待直到有新的服务到来
                spin_lock.unlock();
                Fiber::Suspend();
                spin_lock.lock();
            }
            if(IsClosing()) break;

            for (const auto &service: m_service_queue) new_service.emplace(service, m_port);
            m_service_queue.clear();
        }

        proto.PrepareAndSend(MESSAGE_REQUEST_REGISTRATION, new_service);

        auto message_type = proto.RecvAndParseHeader();
        switch(message_type){
            case MESSAGE_RESPOND_OK:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Connection to registry server: IP: {}, port: {}, register service: {} OK!",
                         m_server_addr->GetIP(), m_server_addr->GetPort(), JsonSerializer::ToString(new_service));
#endif
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

void RPCServer::RegistryClientSession::Update(std::string_view service_name) {
    std::unique_lock<SpinLock> lock(m_service_queue_mutex);
    bool is_queue_empty = m_service_queue.empty();

    m_service_queue.emplace_back(service_name);
    if (is_queue_empty && m_connection_handler_thread_id >= 0) {
        m_fiber_pool->Notify(m_connection_handler_thread_id);
    }
}

void RPCServer::handleMessageRequestRPC(RPCSession& proto) {
    // 从客户端获得服务名
    std::string service_name;
    proto.ParseServiceName(service_name);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
    Logger::info("Handle RPC:{} form IP: {}, port: {} ", service_name, proto.GetPeerIP()->GetIP(),
                 proto.GetPeerIP()->GetPort());
#endif
    // 根据服务名查找服务
    m_service_table_mutex.lock_shared();
    auto iter = m_service_table.find(service_name);
    if(iter == m_service_table.end()){
        m_service_table_mutex.unlock_shared();
        // TODO 错误处理：找不到指定的服务
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
        Logger::info("Failed to call RPC:{} form IP: {}, port: {}, service not found", service_name, proto.GetPeerIP()->GetIP(),
                     proto.GetPeerIP()->GetPort());
#endif
        return;
    }
    auto service_func = iter->second;
    m_service_table_mutex.unlock_shared();

    // 调用函数
    auto to_send = std::move(service_func(proto));

    // 将结果返回客户端
    proto.Send(to_send);
}
