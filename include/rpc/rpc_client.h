#ifndef MYRPC_RPC_CLIENT_H
#define MYRPC_RPC_CLIENT_H

#include "noncopyable.h"
#include "traits.h"

#include "fiber/fiber_sync.h"
#include "fiber/fiber_pool.h"
#include "rpc/config.h"
#include "rpc/exception.h"
#include "net/inetaddr.h"
#include "serialization/json_serializer.h"

#include "rpc/rpc_session.h"
#include "rpc/rpc_client_connection.h"
#include "rpc/load_balance.h"

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

        void Start() { m_fiber_pool->Start(); }
        void Stop() { m_fiber_pool->Stop();}

        template <class Promise, class ...Args>
        auto InvokeAsync(Promise& promise,const std::string& service_name, Args&&... args){
            using ReturnType = typename promise_traits<Promise>::type;

            // 序列化函数参数
            auto service_arg = std::make_tuple(std::forward<Args>(args)...);
            auto to_send = std::make_shared<StringBuffer>(RPCSession::Prepare(MESSAGE_REQUEST_RPC, service_name, service_arg));

            m_fiber_pool->Run([&promise, this, service_name, to_send](){
                // 根据ServiceName，查找服务器IP地址
                InetAddr::ptr server_addr;

                decltype(m_service_table)::iterator it_service; // 需要连接的服务器IP地址在service_table中的位置
                {
                    std::shared_lock<FiberSync::RWMutex> lock_shared(m_service_table_mutex);
                    auto sn_req = m_service_table.equal_range(service_name);
                    while (sn_req.first == sn_req.second) {
                        // 没有找到对应的服务
                        lock_shared.unlock();

                        // 向Registry Server查询服务器IP
                        if(m_registry.IsClosed()){ // 如果Registry Server关闭
                            promise.set_exception(
                                    std::make_exception_ptr(RPCClientException(RPCClientException::REGISTRY_SERVER_CLOSED)));
                            return;
                        }
                        if(!m_registry.Query(service_name)) {
                            // 查询结果：不存在service_name
                            promise.set_exception(
                                    std::make_exception_ptr(RPCClientException(RPCClientException::SERVICE_NOT_FOUND)));
                            return;
                        }
                        lock_shared.lock();
                        // iter = m_service_table.find(service_name);
                        sn_req = m_service_table.equal_range(service_name);
                    }
                    // 负载均衡
                    it_service = m_lb->Select(sn_req.first, sn_req.second, service_name);

                    server_addr = it_service->second;
                }

                {
                    // 根据服务器的IP地址，查找对应的连接
                    auto server_addr_str = server_addr->ToString();
                    std::shared_lock<FiberSync::RWMutex> lock_shared(m_conn_table_mutex);
                    auto it_conn = m_conn_table.find(server_addr_str);
                    decltype(it_conn->second) conn;
                    if (it_conn == m_conn_table.end()){
                        // 没有连接到对应的服务器
                        lock_shared.unlock();

                        // 尝试连接
                        conn = std::make_shared<RPCClientConnection>(server_addr, m_fiber_pool, m_timeout, m_keepalive);
                        if(!conn->Connect()){
                            // 若超时，则从客户端中删除该服务器，并抛出异常
                            std::unique_lock<FiberSync::RWMutex> lock(m_service_table_mutex);
                            m_service_table.erase(it_service);
                            promise.set_exception(std::make_exception_ptr(RPCClientException(RPCClientException::CONNECT_TIME_OUT)));
                            return;
                        }else {
                            std::unique_lock<FiberSync::RWMutex> lock(m_conn_table_mutex);
                            it_conn = m_conn_table.emplace(std::move(server_addr_str), conn).first;
                        }
                    }else{
                        conn = it_conn->second;
                        lock_shared.unlock();
                    }

                    StringBuffer recv_buf;
                    ReturnType ret_val;

                    // 将序列化的函数参数发送到服务器
                    auto err_type = conn->SendRecv(*to_send, recv_buf);
                    if(err_type == RPCClientException::SERVER_CLOSED){
                        // 删除服务器IP地址对应的连接
                        std::unique_lock<FiberSync::RWMutex> lock(m_conn_table_mutex);
                        m_conn_table.erase(it_conn);
                        promise.set_exception(std::make_exception_ptr(RPCClientException(RPCClientException::SERVER_CLOSED)));
                        return;
                    }

                    // 解析服务器的返回值并返回
                    try {
                        RPCSession::ParseContent(recv_buf, ret_val);
                    }catch(std::exception& e){
                        promise.set_exception(std::make_exception_ptr(e));
                        return;
                    }
                    promise.set_value(ret_val);

                }
            });
            return promise.get_future();
        }

    private:
        useconds_t m_timeout;
        int m_keepalive;

        // 服务提供者表
        // Service Name -> Service Provider 1 (IP)
        //              -> Service Provider 2 (IP)
        //              -> ...
        std::multimap<std::string, InetAddr::ptr> m_service_table;
        FiberSync::RWMutex m_service_table_mutex;

        // 连接表
        // Server IP Address -> RPC Connection 1
        //                   -> RPC Connection 2
        //                   -> ...
        std::unordered_map<std::string, RPCClientConnection::ptr> m_conn_table;
        FiberSync::RWMutex m_conn_table_mutex;

        FiberPool::ptr m_fiber_pool;

        std::unique_ptr<LoadBalancer> m_lb;

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
