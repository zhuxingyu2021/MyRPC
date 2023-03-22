#ifndef MYRPC_RPC_SERVER_BASE_H
#define MYRPC_RPC_SERVER_BASE_H

#include <memory>
#include <mutex>

#include "net/tcp_server.h"
#include "rpc/rpc_common.h"
#include "traits.h"
#include "spinlock.h"

namespace MyRPC{
    template <class Derived, class CTXType>
    class RPCServerBase:public TCPServer{
    public:
        using ptr = std::shared_ptr<RPCServerBase<Derived, CTXType>>;

        RPCServerBase(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, ms_t timeout=0):
                TCPServer(bind_addr, fiberPool, timeout){}

        // CRTP类，子类需要实现以下方法
        template<class Args>
        Common::Errortype ParseArgs(CTXType& context, Args&& args){
            static_cast<Derived*>(this)->ParseArgs(context, std::forward<Args>(args));
        }

        template<class Result>
        Common::Errortype Response(CTXType& context, Result&& result, Common::Errortype err){
            static_cast<Derived*>(this)->Response(context, std::forward<Result>(result), err);
        }

        template<class Func>
        void AddMethod(const std::string& service_name,Func&& func) {
            auto method_func = [this, func](CTXType& context)->Common::Errortype{
                // 该函数的作用：在解析RPC请求中的方法名字段后，会调用对应方法的该函数。该函数负责提取函数参数、调用函数，当
                // 函数调用成功后，该函数会发送响应

                using func_traits = function_traits<std::decay_t<Func>>;

                // 提取函数参数类型
                typename func_traits::arg_type args;

                // 从网络I/O中的RPC请求中提取函数参数
                Common::Errortype err = ParseArgs(context, args);
                try {
                    if(err == Common::NO_ERROR) {
                        // 调用对应的函数
                        auto ret_val = func_traits::apply(func, args);
                        err = Response(context, ret_val, err);
                    }
                } catch (...) {
                    err = Common::EXCEPTION;
                }

                return err;
            };

            std::unique_lock<SpinLock> lock(m_service_table_lock);
            m_service_table[service_name] = method_func;
        }

        std::function<Common::Errortype(CTXType&)>* FindService(const std::string& service_name){
            std::unique_lock<SpinLock> lock(m_service_table_lock);
            auto iter = m_service_table.find(service_name);
            if(iter != m_service_table.end()){
                return &(iter->second);
            }else{
                return nullptr;
            }
        }

    protected:
        // 服务表，用于记录及查询服务对应的函数
        std::unordered_map<std::string, std::function<Common::Errortype(CTXType&)>> m_service_table;

        SpinLock m_service_table_lock;
    };
}

#endif //MYRPC_RPC_SERVER_BASE_H
