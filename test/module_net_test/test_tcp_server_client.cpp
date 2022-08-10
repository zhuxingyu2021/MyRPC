#include "net/tcpserver.h"
#include "net/tcpclient.h"
#include "fiber/timeoutio.h"
#include "fiber/fiberpool.h"

#define TIME_OUT 2000000

using namespace MyRPC;
using namespace std;

int main(){
    FiberPool::ptr fiberPool = make_shared<FiberPool>(1);
    TCPServer::ptr server = make_shared<TCPServer>(fiberPool, TIME_OUT, false);
    if(!server->Bind(make_shared<InetAddr>("127.0.0.1", 9999))){
        Logger::error("bind error");
        return -1;
    }
    if(!server->Start()){
        Logger::error("start error");
        return -1;
    }

    fiberPool->Start();

    while(true){
        sleep(1);
    }
}
