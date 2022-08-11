#include "net/tcp_server.h"
#include "net/tcp_client.h"
#include "fiber/timeout_io.h"
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
    void handleConnection(Socket::ptr sock) override {
        int sock_fd = sock->GetSocketfd();
        char buf[1024];
        while(true) {
            int recv_sz = recv_timeout(sock_fd, buf, sizeof(buf), 0, TIME_OUT);
            if (recv_sz > 0) {
                send(sock_fd, buf, recv_sz, 0);
            } else if (recv_sz == MYRPC_ERR_TIMEOUT_FLAG) {
                spdlog::info("recv timeout, retrying...");
            } else {
                spdlog::error("recv error, info: {}", strerror(errno));
                break;
            }
        }
    }
};

class EchoClient: public TCPClient<EchoClient>{
    using TCPClient<EchoClient>::TCPClient;
public:
    char buf[1024];
    void doConnect(const string& msg, bool* err){
        *err = false;
        send(sock->GetSocketfd(), msg.c_str(), msg.size(), 0);

        int recv_sz = recv_timeout(sock->GetSocketfd(), buf, sizeof(buf), 0, TIME_OUT);
        if (recv_sz > 0) {
            std::cout << "recv: " << buf << std::endl;
            memset(buf, 0, recv_sz);
        } else if (recv_sz == MYRPC_ERR_TIMEOUT_FLAG) {
            spdlog::error("recv timeout");
            *err = true;
        } else {
            spdlog::error("recv error, info: {}", strerror(errno));
            *err = true;
        }
    }
};

int main(){
    EchoServer server(8, TIME_OUT);

    if(!server.Bind(make_shared<InetAddr>("127.0.0.1", 9999))){
        Logger::error("bind error");
        return -1;
    }
    server.Start();

    auto client = EchoClient::connect(make_shared<InetAddr>("127.0.0.1", 9999));

    std::string cmdline;
    std::cin >> cmdline;
    while(cmdline != "q"){
        bool err;
        client->doConnect(cmdline, &err);
        std::cin >> cmdline;
        if(err) break;
    }

}
