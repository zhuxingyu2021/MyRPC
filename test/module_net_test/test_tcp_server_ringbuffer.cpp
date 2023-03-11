#include "net/tcp_server.h"
#include "net/ringbuffer.h"
#include "fiber/fiber_pool.h"
#include "fiber/sync_queue.h"

#include <iostream>
#include <string>

#define TIME_OUT 2000000 // 2秒

using namespace MyRPC;
using namespace std;

class EchoServer : public TCPServer {
public:
    EchoServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, useconds_t timeout=0)
            : TCPServer(bind_addr, fiberPool, timeout){
        AddConnectionHandler([this](Socket::ptr sock){return read_buffer(sock);});
        AddConnectionHandler([this](Socket::ptr sock){return write_buffer(sock);});
    }
private :
    SyncQueue<std::string> q;

    void read_buffer(Socket::ptr& sock){
        ReadRingBuffer read_buf(sock, TIME_OUT);
        while(true) {
            try {
                q.Push(read_buf.ReadUntil<'.'>());
                read_buf.Commit();
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

    void write_buffer(Socket::ptr& sock){
        WriteRingBuffer write_buf(sock, TIME_OUT);
        while(true) {
            try {
                write_buf.Append(q.Pop());
                write_buf.Flush();
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

int main(){
    FiberPool::ptr pool(new FiberPool(1));
    EchoServer server(make_shared<InetAddr>("127.0.0.1", 9998),pool, TIME_OUT);

    pool->Start();

    if(!server.bind()){
        Logger::error("bind error");
        return -1;
    }
    server.Start();
    server.Loop();

}
