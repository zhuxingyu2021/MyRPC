#include "fiber/fiber_pool.h"
#include "rpc/jsonrpc/jsonrpc_server_base.h"

#include <string>
#include <cstdio>
#include <iostream>

using namespace std;
using namespace MyRPC;

int sum(vector<int>& v){
    int ret_val = 0;
    for(auto num: v)
        ret_val += num;
    return ret_val;
}

int main(int argc, char** argv){
    FiberPool::ptr pool(new FiberPool(1));
    int port;
    cin >> port;

    JsonRPC::JsonRPCServerBase::ptr server(
            new JsonRPC::JsonRPCServerBase(make_shared<InetAddr>("127.0.0.1", port),pool, 0));

    if(!server->bind()){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server->Start();


    std::function<std::string(const std::string&)> func_echo = [](const std::string& in)->std::string{
        return in;
    };
    server->AddMethod("echo", func_echo);
    server->AddMethod("sum", sum);

    server->Loop();
}
