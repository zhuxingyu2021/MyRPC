#ifndef MYRPC_RPC_CLIENT_CONNECTION_H
#define MYRPC_RPC_CLIENT_CONNECTION_H

#include "fiber/fiber_sync.h"
#include "fiber/sync_queue.h"

#include "net/tcp_client.h"
#include "net/inetaddr.h"

#include "rpc/rpc_session.h"
#include "rpc/exception.h"

#include "spinlock.h"
#include "buffer/stringbuffer.h"

#include <memory>
#include <list>

namespace MyRPC{
    class RPCClientConnection: public TCPClient{
    public:
        using ptr = std::shared_ptr<RPCClientConnection>;
        RPCClientConnection(InetAddr::ptr& server_addr, FiberPool::ptr& fiberPool, useconds_t timeout, int keep_alive):
                            TCPClient(server_addr, fiberPool, timeout),m_keepalive(keep_alive){}

        RPCClientException::ErrorType SendRecv(const StringBuffer& to_send, StringBuffer& recv);

        virtual void handleConnect() override;
    private:
        int m_keepalive;

        int m_connection_handler_thread_id = -1;

        RPCSession* m_session = nullptr;

        struct RPCQueueNode{
            using ptr = std::shared_ptr<RPCQueueNode>;
            RPCQueueNode(const StringBuffer& to_send):m_send(to_send){}

            const StringBuffer& m_send;
            std::optional<StringBuffer> m_ret = std::nullopt;
            RPCClientException::ErrorType m_err = RPCClientException::HAVENT_BEEN_CALLED;
        };

        // RPC消息队列
        SyncQueue<RPCQueueNode::ptr> m_send_queue;
        SyncQueue<RPCQueueNode::ptr> m_recv_queue;
    };
}

#endif //MYRPC_RPC_CLIENT_CONNECTION_H
