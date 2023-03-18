#ifndef MYRPC_TCP_SERVER_CONN_H
#define MYRPC_TCP_SERVER_CONN_H

#include "fiber/fiber.h"
#include "fiber/fiber_pool.h"
#include "net/socket.h"
#include "noncopyable.h"
#include <vector>
#include <memory>

namespace MyRPC{
    class TCPServerConn:public NonCopyable{
    public:
        using ptr = std::shared_ptr<TCPServerConn>;
        using weak_ptr = std::weak_ptr<TCPServerConn>;

        virtual ~TCPServerConn() = default;

        virtual void Terminate(){
            auto current_id = Fiber::GetCurrentId();
            for(auto& fiber:m_active_handler){
                if(!fiber.expired()) {
                    if (fiber.lock()->GetId() != current_id) {
                        fiber.lock()->Term();
                    }
                }
            }
        }

        template<class Func>
        void AddAsyncTask(Func&& func){
            m_active_handler.push_back(FiberPool::GetThis()->AddAsyncTask(std::forward<Func>(func)));
        }

        Socket::unique_ptr& GetSock(){
            return m_sock;
        }

    private:
        friend class TCPServer;
        Socket::unique_ptr m_sock;

        std::vector<Fiber::weak_ptr> m_active_handler;
    };
}

#endif //MYRPC_TCP_SERVER_CONN_H
