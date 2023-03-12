#include "net/tcp_server.h"
#include "buffer/ringbuffer.h"
#include "fiber/fiber_pool.h"
#include "fiber/sync_queue.h"

#include <iostream>
#include <string>

#define TIME_OUT 2000000 // 2秒

using namespace MyRPC;
using namespace std;

struct EchoServerConnection: public TCPServerConn{
    ~EchoServerConnection(){
        Logger::info("Connection class deconstructed!");
    }

    SyncQueue<std::string> q;
};

class EchoServer : public TCPServer {
public:
    EchoServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, ms_t timeout=0)
            : TCPServer(bind_addr, fiberPool, timeout){
        // 设定连接类
        SetConnectionClass<EchoServerConnection>();

        // 添加连接处理函数
        AddConnectionHandler([this](Socket::ptr sock, TCPServerConn* conn){
            return read_buffer(sock, (EchoServerConnection*)conn);
        });
        AddConnectionHandler([this](Socket::ptr sock, TCPServerConn* conn){
            return write_buffer(sock, (EchoServerConnection*)conn);
        });
    }
private :

    void read_buffer(Socket::ptr& sock, EchoServerConnection* conn){
        ReadRingBuffer read_buf(sock, TIME_OUT);
        while(true) {
            string s;
            try {
                read_buf.ReadUntil<'.'>(s);
                conn->q.Push(s);
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

    void write_buffer(Socket::ptr& sock, EchoServerConnection* conn){
        WriteRingBuffer write_buf(sock, TIME_OUT);
        while(true) {
            try {
                write_buf.Append(conn->q.Pop());
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
