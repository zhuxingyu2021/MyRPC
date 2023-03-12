#ifndef MYRPC_TCP_SERVER_CONN_H
#define MYRPC_TCP_SERVER_CONN_H

#include "fiber/fiber.h"
#include "fiber/fiber_pool.h"
#include "noncopyable.h"
#include <vector>

namespace MyRPC{
    class TCPServerConn:public NonCopyable{
    public:
        virtual ~TCPServerConn() = default;

        void Terminate(){
            auto current_id = Fiber::GetCurrentId();
            for(auto& fiber:m_active_handler){
                if(fiber->GetId() != current_id){
                    fiber->Term();
                }
            }
        }

        template<class Func>
        void AddAsyncTask(Func&& func){
            m_active_handler.push_back(FiberPool::GetThis()->AddAsyncTask(std::forward<Func>(func)));
        }

    private:
        friend class TCPServer;
        std::vector<Fiber::ptr> m_active_handler;
    };
}

#endif //MYRPC_TCP_SERVER_CONN_H
