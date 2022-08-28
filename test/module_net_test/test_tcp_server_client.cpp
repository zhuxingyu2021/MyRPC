#include "net/tcp_server.h"
#include "fiber/fiber_pool.h"

#include <iostream>
#include <string>

#define TIME_OUT 2000000 // 2秒

using namespace MyRPC;
using namespace std;

class EchoServer : public TCPServer {
public:
    using TCPServer::TCPServer;
protected:
    void handleConnection(const Socket::ptr& sock) override {
        TCPServer::handleConnection(sock);

        int sock_fd = sock->GetSocketfd();
        char buf[1024];
        try {
            while (true) {
                int recv_sz = sock->RecvTimeout(buf, sizeof(buf), 0, TIME_OUT);
                if (recv_sz > 0) {
                    sock->SendAll(buf, recv_sz, 0);
                } else if (recv_sz < 0) {
                    //Logger::info("recv timeout, retrying...");
                } else{
                    Logger::info("socket fd:{}, client close connection", sock->GetSocketfd());
                    break;
                }
            }
        }
        catch (const std::exception& e){
            Logger::error(e.what());
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
    EchoServer server(8, TIME_OUT);

    if(!server.bind(make_shared<InetAddr>("127.0.0.1", 9999))){
        Logger::error("bind error");
        return -1;
    }
    server.Start();

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
