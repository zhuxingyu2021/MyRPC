#include "net/tcp_client.h"

using namespace MyRPC;

bool TCPClient::Connect(){
    if(m_conn_handler_array.empty()) {
        int tid = -1;
        Socket::ptr sock = Socket::Connect(m_server_addr, m_timeout);
        sock->m_destructor = m_close_handler;
        for(auto& func:m_conn_handler){
            if(tid == -1) {
                auto [fiber, _tid] = m_fiber_pool->Run([func, sock] { return func(sock); });
                tid = _tid;
                m_conn_handler_array.emplace_back(fiber);
            }else{
                auto [fiber, _tid] = m_fiber_pool->Run([func, sock] { return func(sock); }, tid);
                m_conn_handler_array.emplace_back(fiber);
            }
        }
        m_conn_thread_id = tid;

#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
        Logger::info("Thread: {}: A new connection to server IP:{}, port:{}, connection fd:{}", tid,
                     m_server_addr->GetIP(), m_server_addr->GetPort(), sock->GetSocketfd());
#endif
        return true;
    }
    return false;
}

void TCPClient::DisConnect() {
    using a_T = std::vector<Fiber::ptr>;
    std::shared_ptr<a_T> conn_handler_array_copy_ptr(new a_T(std::move(m_conn_handler_array)));
    m_fiber_pool->Run([conn_handler_array_copy_ptr]{
        auto vec = *conn_handler_array_copy_ptr;
        for(auto& fiber:vec){
            fiber->Term();
        }
    }, m_conn_thread_id);

    m_conn_handler_array = {};
}
