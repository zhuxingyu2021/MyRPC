#include "rpc/rpc_client_connection.h"

#include <shared_mutex>
#include <queue>

using namespace MyRPC;

RPCClientException::ErrorType RPCClientConnection::SendRecv(const StringBuffer& to_send, StringBuffer& recv) {
    std::unique_lock<FiberSync::Mutex> lock(m_rpc_queue_mutex);
    if(IsClosed()){
        // 如果连接已经被关闭
        return RPCClientException::HAVENT_BEEN_CALLED;
    }
    bool is_queue_empty = m_rpc_queue.empty();
    auto node_ptr = m_rpc_queue.emplace_back(new RPCQueueNode(to_send));
    lock.unlock();

    if(is_queue_empty && m_connection_handler_thread_id >= 0) m_fiber_pool->Notify(m_connection_handler_thread_id);

    // 等待接收到信息
    node_ptr->m_waiter.lock();
    node_ptr->m_waiter.lock();

    if(node_ptr->m_ret){
        recv = std::move(node_ptr->m_ret.value());
    }
    return node_ptr->m_err;
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

    FiberSync::Mutex wait_send_msg_task_exit_mutex; // 用于等待消息发送子协程退出
    wait_send_msg_task_exit_mutex.lock();
    // 开启子协程，从消息队列中读取消息并发送
    m_fiber_pool->Run([this, &kill_subtask, &wait_send_msg_task_exit_mutex, &wait_recv_queue](){
        while(!kill_subtask){
            {
                // 从消息队列中读取数据
                std::unique_lock<FiberSync::Mutex> lock(m_rpc_queue_mutex);
                while (!kill_subtask && m_rpc_queue.empty()) { // 如果消息队列为空，就等待直到有新的消息进来
                    lock.unlock();
                    Fiber::Suspend();
                    lock.lock();
                }
                if(kill_subtask) break;

                for(auto iter = m_rpc_queue.begin(); iter != m_rpc_queue.end(); ++iter){
                    // 发送消息队列的每一个消息，并将它加入等待接收队列中
                    m_session->Send((*iter)->m_send);
                    wait_recv_queue.emplace(std::move(*iter));
                }

                m_rpc_queue.clear(); // 清空消息队列
            }
            wait_send_msg_task_exit_mutex.unlock();
        }
    }, m_connection_handler_thread_id);

    // 从服务器中接收返回数据
    while(!IsClosing()){
        auto message_type = m_session->RecvAndParseHeader();
        switch(message_type){
            case MESSAGE_RESPOND_OK:
                // 读取服务器发过来的数据
                wait_recv_queue.front()->m_ret = m_session->GetContent();
                wait_recv_queue.front()->m_waiter.unlock(); // 唤醒等待在消息队列上的协程

                // 删除队头
                wait_recv_queue.pop();
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
    wait_send_msg_task_exit_mutex.lock();

    m_connection_closed = true;

    // 处理消息队列中未处理的消息
    {
        std::unique_lock<FiberSync::Mutex> lock(m_rpc_queue_mutex);
        for(const auto& node_ptr: m_rpc_queue){
            node_ptr->m_waiter.unlock(); // 唤醒等待在消息队列上的协程
        }
    }


    delete m_session;
    m_session = nullptr;
}
