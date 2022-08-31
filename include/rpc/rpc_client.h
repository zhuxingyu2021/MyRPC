#ifndef MYRPC_RPC_CLIENT_H
#define MYRPC_RPC_CLIENT_H

#include "noncopyable.h"

#include "fiber/fiber_sync.h"
#include "fiber/fiber_pool.h"
#include "rpc/config.h"
#include "rpc/exception.h"
#include "net/inetaddr.h"
#include "net/serializer.h"

#include "rpc/rpc_session.h"
#include "rpc/rpc_client_connection.h"

#include <memory>
#include <string>
#include <future>
#include <unordered_map>
#include <shared_mutex>
#include <tuple>
#include <string_view>

namespace MyRPC{
    class RPCClient:public NonCopyable{
    public:
        using ptr = std::shared_ptr<RPCClient>;
        explicit RPCClient(Config::ptr config);

        bool ConnectToRegistryServer(){return m_registry.Connect();}
        bool ConnectToRPCServer();

        template <class ReturnType, class ...Args>
        std::future<ReturnType> InvokeAsync(const std::string& service_name, Args&&... args){
            using promise_type = std::promise<ReturnType>;
            std::shared_ptr<promise_type> promise = std::make_shared<promise_type>();

            // 序列化函数参数
            auto service_arg = std::make_tuple(std::forward<Args>(args)...);
            auto to_send = std::make_shared<StringBuffer>(RPCSession::Prepare(MESSAGE_REQUEST_RPC, service_name, service_arg));

            m_fiber_pool->Run([promise, this, service_name, to_send](){
                // 根据ServiceName，查找服务器IP地址
                InetAddr::ptr server_addr;
                {
                    std::shared_lock<FiberSync::RWMutex> lock_shared(m_service_table_mutex);
                    auto iter = m_service_table.find(service_name);
                    while (iter == m_service_table.end()) {
                        // 没有找到对应的服务
                        lock_shared.unlock();

                        // 向Registry Server查询服务器IP
                        if(m_registry.IsClosed()){ // 如果Registry Server关闭
                            promise->set_exception(
                                    std::make_exception_ptr(RPCClientException(RPCClientException::REGISTRY_SERVER_CLOSED)));
                            return;
                        }
                        if(!m_registry.Query(service_name)) {
                            // 查询结果：不存在service_name
                            promise->set_exception(
                                    std::make_exception_ptr(RPCClientException(RPCClientException::SERVICE_NOT_FOUND)));
                            return;
                        }
                        lock_shared.lock();
                        iter = m_service_table.find(service_name);
                    }
                    server_addr = iter->second;
                }

                {
                    // 根据服务器的IP地址，查找对应的连接
                    auto server_addr_str = server_addr->ToString();
                    m_conn_table_mutex.lock_shared();
                    auto iter = m_conn_table.find(server_addr_str);
                    if (iter == m_conn_table.end()){
                        // 没有连接到对应的服务器
                        m_conn_table_mutex.unlock_shared();

                        // 尝试连接
                        auto conn = std::make_shared<RPCClientConnection>(server_addr, m_fiber_pool, m_timeout, m_keepalive);
                        if(!conn->Connect()){
                            // 若超时，则抛出异常
                            promise->set_exception(std::make_exception_ptr(RPCClientException(RPCClientException::CONNECT_TIME_OUT)));
                            return;
                        }else {
                            m_conn_table.emplace(std::move(server_addr_str), std::move(conn));
                        }
                    }else{
                        auto conn = iter->second;
                        m_conn_table_mutex.unlock_shared();

                        StringBuffer recv_buf;
                        ReturnType ret_val;

                        // 将序列化的函数参数发送到服务器
                        conn->SendRecv(*to_send, recv_buf);

                        // 解析服务器的返回值并返回
                        RPCSession::ParseContent(recv_buf, ret_val);
                        promise->set_value(ret_val);
                    }
                }
            });
            return promise->get_future();
        }

    private:
        useconds_t m_timeout;
        int m_keepalive;

        // 服务提供者表
        // Service Name -> Service Provider 1 (IP)
        //              -> Service Provider 2 (IP)
        //              -> ...
        std::unordered_multimap<std::string, InetAddr::ptr> m_service_table;
        FiberSync::RWMutex m_service_table_mutex;

        // 连接表
        // Server IP Address -> RPC Connection 1
        //                   -> RPC Connection 2
        //                   -> ...
        std::unordered_map<std::string, RPCClientConnection::ptr> m_conn_table;
        FiberSync::RWMutex m_conn_table_mutex;

        FiberPool::ptr m_fiber_pool;

        class RegistryClientSession: public TCPClient {
        public:
            RegistryClientSession(InetAddr::ptr server_addr, RPCClient& client): TCPClient(server_addr, client.m_fiber_pool, client.m_timeout),
                                                                                 m_client(client), m_keepalive(client.m_keepalive){}
            bool Query(std::string_view service_name);

        private:
            int m_keepalive;

            RPCClient& m_client;

            struct ServiceQueueNode{
                using ptr = std::shared_ptr<ServiceQueueNode>;
                ServiceQueueNode(std::string_view service_name):m_service_name(service_name){}

                std::string m_service_name;
                FiberSync::Mutex wait_mutex;
                bool is_exist = false; // 服务是否存在（服务查询结果）
            };

            // 需要从注册服务器接收的服务
            std::list<ServiceQueueNode::ptr> m_service_queue;
            SpinLock m_service_queue_mutex;

            int m_connection_handler_thread_id = -1;

        protected:
            virtual void handleConnect() override;
        };

        RegistryClientSession m_registry;
    };
}

#endif //MYRPC_RPC_CLIENT_H
