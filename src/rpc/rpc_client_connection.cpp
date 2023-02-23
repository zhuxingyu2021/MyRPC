#include "rpc/rpc_client_connection.h"

#include <shared_mutex>

using namespace MyRPC;

RPCClientException::ErrorType RPCClientConnection::SendRecv(const StringBuffer& to_send, StringBuffer& recv) {
    if(IsClosed()){
        // 如果连接已经被关闭
        return RPCClientException::HAVENT_BEEN_CALLED;
    }

    // 发送消息到发送队列
    m_send_queue.Emplace(new RPCQueueNode(to_send));

    // 从接收队列中接收消息
    auto ret_ptr = m_recv_queue.Pop();

    if(ret_ptr->m_ret){
        recv = std::move(ret_ptr->m_ret.value());
    }
    return ret_ptr->m_err;
}

void RPCClientConnection::handleConnect() {
    TCPClient::handleConnect();

    m_connection_handler_thread_id = FiberPool::GetCurrentThreadId();

    std::queue<RPCQueueNode::ptr> wait_recv_queue; // 已经发送，等待接收的队列

    m_session = new RPCSession(*m_sock, m_timeout);

    bool kill_subtask = false; // 用于杀死所有子协程

    FiberSync::Mutex wait_heartbeat_task_exit_mutex; // 用于等待心跳定时发送子协程退出
    wait_heartbeat_task_exit_mutex.lock();
    // 开启子协程，定时发送Heartbeat包
    m_fiber_pool->Run([this, &kill_subtask, &wait_heartbeat_task_exit_mutex](){
        usleep(m_keepalive * 800000);
        while(!kill_subtask){
            m_session->PrepareAndSend(MESSAGE_HEARTBEAT);
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
            Logger::info("RPC server: IP: {}, port: {}, heartbeats package has already sent",
                         m_server_addr->GetIP(), m_server_addr->GetPort());
#endif
            usleep(m_keepalive * 800000);
        }
        wait_heartbeat_task_exit_mutex.unlock();
    }, m_connection_handler_thread_id);

    // 从服务器中接收返回数据
    while(!IsClosing()){
        auto node = m_send_queue.Pop(); // 从发送队列中读取要发送的数据
        m_session->Send(node->m_send); // 发送数据
        auto message_type = m_session->RecvAndParseHeader();
        switch(message_type){
            case MESSAGE_RESPOND_OK:
                // 读取服务器发过来的数据
                node->m_ret = m_session->GetContent();

                // 将数据放到接收队列
                m_recv_queue.Push(node);
                break;
            default:
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_RPC_LEVEL
                Logger::error("Connection to rpc server: IP: {}, port: {}, error message:{}",
                              m_server_addr->GetIP(), m_server_addr->GetPort(), ToString(message_type));
#endif
                goto end_while;
        }
    }
    end_while:

    // 等待子协程退出
    kill_subtask = true;
    m_connection_handler_thread_id = -1;
    wait_heartbeat_task_exit_mutex.lock();

    m_connection_closed = true;

    m_recv_queue.Close();


    delete m_session;
    m_session = nullptr;
}
