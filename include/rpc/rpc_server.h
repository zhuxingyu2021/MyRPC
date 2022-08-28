#ifndef MYRPC_RPC_SERVER_H
#define MYRPC_RPC_SERVER_H

#include "function_traits.h"
#include "stringbuffer.h"

#include "net/tcp_server.h"
#include "rpc/rpc_registry_client.h"
#include "rpc/rpc_session.h"
#include "rpc/config.h"

#include <memory>
#include <unordered_map>
#include <functional>
#include <exception>

namespace MyRPC{
    class RPCServer:public TCPServer{
    public:
        using ptr = std::shared_ptr<RPCServer>;
        explicit RPCServer(const Config::ptr& config);

        template<class Func>
        void RegisterMethod(std::string_view service_name,Func func){
            m_service_table.emplace(service_name, [&func](RPCSession& proto) -> StringBuffer{
                using func_traits = function_traits<Func>;

                typename func_traits::arg_type args;
                proto.ParseContent(args);
                try {
                    auto ret_val = func_traits::apply(func, args);
                    return proto.Prepare(RPCSession::MESSAGE_RESPOND_OK, ret_val);
                }catch(std::exception& e){
                    std::string msg(e.what());
                    return proto.Prepare(RPCSession::MESSAGE_RESPOND_EXCEPTION, msg);
                }
            });
        }

    protected:
        void handleConnection(const Socket::ptr& sock) override;

    private:
        int m_keepalive;
        useconds_t m_timeout;

        // 服务表，用于记录及查询服务对应的函数
        std::unordered_map<std::string, std::function<StringBuffer(RPCSession&)>> m_service_table;
    };
}

#endif //MYRPC_RPC_SERVER_H
