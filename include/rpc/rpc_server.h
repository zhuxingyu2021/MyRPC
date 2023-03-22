#ifndef MYRPC_RPC_SERVER_H
#define MYRPC_RPC_SERVER_H

#include "rpc/rpc_common.h"
#include "rpc/jsonrpc/jsonrpc_server_base.h"

namespace MyRPC{
    class RPCServer{
    public:
        RPCServer(const InetAddr::ptr& bind_addr,Common::Prototype proto_type, int thread_count = 1, ms_t timeout=0){
            m_proto_type = proto_type;
            m_fiber_pool = std::make_shared<FiberPool>(thread_count);
            switch(proto_type){
                case Common::JSONRPC2:
                    m_server = new JsonRPC::JsonRPCServerBase(bind_addr, m_fiber_pool,timeout);
                    break;
                default:
                    MYRPC_ASSERT(false);
            }
        }

        template<class Func>
        void AddMethod(const std::string& service_name,Func&& func) {
            switch(m_proto_type) {
                case Common::JSONRPC2:
                    return static_cast<JsonRPC::JsonRPCServerBase*>(m_server)->AddMethod(service_name,
                                                                                         std::forward<Func>(func));
                default:
                    MYRPC_ASSERT(false);
            }
        }

        bool bind(){
            return m_server->bind();
        }

        void Start(){
            return m_server->Start();
        }

        void Loop(){
            return m_server->Loop();
        }
        ~RPCServer(){
            delete m_server;
        }

    private:
        FiberPool::ptr m_fiber_pool = nullptr;

        Common::Prototype m_proto_type;
        TCPServer* m_server = nullptr;
    };
}

#endif //MYRPC_RPC_SERVER_H
