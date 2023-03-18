#include "net/tcp_server.h"
#include "buffer/ringbuffer.h"
#include "fiber/fiber_pool.h"
#include "fiber/sync_queue.h"

#include <iostream>
#include <string>

#define TIME_OUT 0

using namespace MyRPC;
using namespace std;

struct EchoServerConnection: public TCPServerConn{
    using ptr = std::shared_ptr<EchoServerConnection>;

    ~EchoServerConnection() override{
        Logger::info("Connection class deconstructed!");
    }

    void Terminate() override{
        TCPServerConn::Terminate();
        q.Clear(); // 同步队列在堆区，需要清空
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
        AddConnectionHandler([this](TCPServerConn::ptr conn){
            return read_buffer(static_pointer_cast<EchoServerConnection>(conn));
        });
        AddConnectionHandler([this](TCPServerConn::ptr conn){
            return write_buffer(static_pointer_cast<EchoServerConnection>(conn));
        });
    }
private :

    void read_buffer(EchoServerConnection::ptr conn){
        ReadRingBuffer read_buf(conn->GetSock(), TIME_OUT);
        while(true) {
            string s;
            try {
                // TODO BUG: 当ReadUntil读到两个连续的.时，触发该BUG
                read_buf.ReadUntil<'.'>(s);
                read_buf.GetChar();
                conn->q.Push(s);
                read_buf.Commit();
            }
            catch (const NetException &e) {
                switch (e.GetErrType()) {
                    case NetException::CONN_CLOSE:
                        Logger::info("socket fd:{}, client close connection", conn->GetSock()->GetSocketfd());
                        conn->Terminate();
                        return;
                    case NetException::TIMEOUT:
                        Logger::info("socket fd:{}, client time out", conn->GetSock()->GetSocketfd());
                        continue;
                    default:
                        Logger::error(e.what());
                        conn->Terminate();
                        return;
                }
            }
        }
    }

    void write_buffer(EchoServerConnection::ptr conn){
        WriteRingBuffer write_buf(conn->GetSock(), TIME_OUT);
        while(true) {
            try {
                write_buf.Append(conn->q.Pop());
                write_buf.Flush();
            }
            catch (const NetException &e) {
                switch (e.GetErrType()) {
                    case NetException::TIMEOUT:
                        Logger::info("socket fd:{}, client time out", conn->GetSock()->GetSocketfd());
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

    int port;
    cin >> port;

    EchoServer server(make_shared<InetAddr>("127.0.0.1", port),pool, TIME_OUT);

    pool->Start();

    if(!server.bind()){
        Logger::error("bind error");
        return -1;
    }
    server.Start();
    server.Loop();

}
