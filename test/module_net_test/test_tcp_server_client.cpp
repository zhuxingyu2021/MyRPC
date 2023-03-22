#include "net/tcp_server.h"
#include "fiber/fiber_pool.h"

#include <iostream>
#include <string>

#define TIME_OUT 2000 // 2秒

using namespace MyRPC;
using namespace std;

class EchoServer : public TCPServer {
public:
    EchoServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, ms_t timeout=0)
        : TCPServer(bind_addr, fiberPool, timeout){
        SetConnectionClass<TCPServerConn>();
        AddConnectionHandler([this](TCPServerConn::ptr conn){return handle_connection(conn->GetSock());});
    }
private :
    void handle_connection(Socket::unique_ptr& sock){

        int sock_fd = sock->GetSocketfd();
        char buf[1024];
        while(true) {
            try {
                int recv_sz = sock->RecvTimeout(buf, sizeof(buf), 0, TIME_OUT);
                if (recv_sz > 0) {
                    sock->SendAll(buf, recv_sz, 0);
                }
            }
            catch (const NetException &e) {
                switch (e.GetErrType()) {
                    case NetException::CONN_CLOSE:
                        Logger::info("socket fd:{}, client close connection", sock->GetSocketfd());
                        return;
                    case NetException::TIMEOUT:
                        Logger::info("socket fd:{}, client time out", sock->GetSocketfd());
                        continue;
                    default:
                        Logger::error(e.what());
                }
            }
        }
    }
};

class EchoClient{
public:
    using ptr = std::shared_ptr<EchoClient>;

    EchoClient(const InetAddr::ptr& remote_addr):sock(SocketSTDMutex::Connect(remote_addr)){}

    void Post(const string& msg, bool* err){
        *err = false;

        try {
            sock->SendAll(msg.c_str(), msg.size(), 0);

            int recv_sz = sock->RecvTimeout(buf, sizeof(buf), 0, TIME_OUT);
            if (recv_sz > 0) {
                std::cout << "recv: " << buf << std::endl;
                memset(buf, 0, recv_sz);
            } else if (recv_sz == MYRPC_ERR_TIMEOUT_FLAG) {
                Logger::error("recv timeout");
                *err = true;
            } else{
                Logger::error("connection reset");
                *err = true;
            }
        }catch (const std::exception& e){
            Logger::error(e.what());
            *err = true;
        }
    }

private:
    SocketSTDMutex::ptr sock;
    char buf[1024]{};
};

int main(){
    FiberPool::ptr pool(new FiberPool(8));
    EchoServer server(make_shared<InetAddr>("127.0.0.1", 9998),pool, TIME_OUT);

    pool->Start();

    if(!server.bind()){
        Logger::error("bind error");
        return -1;
    }
    server.Start();
    sleep(1);

    EchoClient client(make_shared<InetAddr>("127.0.0.1", 9999));

    std::string cmdline;
    std::cin >> cmdline;
    while(cmdline != "q"){
        bool err;
        client.Post(cmdline, &err);
        std::cin >> cmdline;
        if(err) break;
    }

}
