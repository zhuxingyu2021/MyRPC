#ifndef MYRPC_RPC_SERVER_H
#define MYRPC_RPC_SERVER_H

#include "function_traits.h"
#include "stringbuffer.h"
#include "spinlock.h"

#include "net/tcp_server.h"
#include "net/tcp_client.h"
#include "rpc/rpc_session.h"
#include "rpc/config.h"

#include <memory>
#include <unordered_map>
#include <functional>
#include <exception>
#include <list>

namespace MyRPC{
    class RPCServer:public TCPServer{
    public:
        using ptr = std::shared_ptr<RPCServer>;
        explicit RPCServer(InetAddr::ptr bind_addr, Config::ptr config);

        template<class Func>
        void RegisterMethod(const std::string& service_name,Func&& func){
            auto register_func = [this](std::string service_name, Func func){
                {
                    std::unique_lock<FiberSync::RWMutex> lock(m_service_table_mutex);
                    m_service_table.emplace(service_name, [func](RPCSession &proto) -> StringBuffer {
                        using func_traits = function_traits<std::decay_t<Func>>;

                        typename func_traits::arg_type args;
                        proto.ParseContent(args);
                        try {
                            auto ret_val = func_traits::apply(func, args);
                            return proto.Prepare(MESSAGE_RESPOND_OK, ret_val);
                        } catch (std::exception &e) {
                            std::string msg(e.what());
                            return proto.Prepare(MESSAGE_RESPOND_EXCEPTION, msg);
                        }
                    });
                }
                m_registry.Update(service_name);
            };
            m_fiber_pool-> Run(std::bind(register_func, service_name, std::forward<Func>(func)));
        }

        bool ConnectToRegistryServer(){return m_registry.Connect();}

    protected:
        void handleConnection(const Socket::ptr& sock) override;

    private:
        int m_keepalive;

        // 服务表，用于记录及查询服务对应的函数
        std::unordered_map<std::string, std::function<StringBuffer(RPCSession&)>> m_service_table;

        FiberSync::RWMutex m_service_table_mutex;

        class RegistryClientSession: public TCPClient{
        public:
            RegistryClientSession(InetAddr::ptr server_addr, FiberPool::ptr& fiberPool, useconds_t timeout, int keep_alive):
                                                        TCPClient(server_addr, fiberPool, timeout),m_keepalive(keep_alive){}
            void Update(std::string_view service_name);
        private:
            int m_keepalive;

            // 需要更新到注册服务器的新服务
            std::list<std::string> m_service_queue;
            SpinLock m_service_queue_mutex;

            std::atomic<bool> m_connection_closed = {true};

            int m_connection_handler_thread_id = -1;

        protected:

            virtual void handleConnect() override;
        };
        RegistryClientSession m_registry;

        void handleMessageRequestRPC(RPCSession&);
    };
}

#endif //MYRPC_RPC_SERVER_H
