#include "fiber/synchronization.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_session.h"
#include "rpc/exception.h"

using namespace MyRPC;

RPCServer::RPCServer(const Config::ptr &config) :TCPServer(config->GetThreadsNum(), 1000*config->GetTimeout(),
                                                           config->IsIPv6()), m_keepalive(config->GetKeepalive()),
                                                 m_timeout(1000*config->GetTimeout()){
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
        }while(heartbeat_flag.exchange(false));
        heartbeat_stopped_flag = true;
        wait_subtask_exit_mutex.unlock();
    });

    while (!IsStopping()) {
        RPCSession::MessageType message_type;
        try {
            message_type = proto.RecvAndParseHeader();
        }catch(std::exception& e){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("Internal Error: {}, connection form IP: {}, port: {}", e.what(), proto.GetPeerIP()->GetIP(),
                         proto.GetPeerIP()->GetPort());
            message_type = RPCSession::ERROR_OTHERS;
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


