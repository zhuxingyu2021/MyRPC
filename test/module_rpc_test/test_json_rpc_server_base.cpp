#include "fiber/fiber_pool.h"
#include "rpc/jsonrpc/jsonrpc_server_base.h"

#include <string>
#include <cstdio>
#include <iostream>

using namespace MyRPC;

int main(int argc, char** argv){
    JsonRPCServerBase server;

    if(!server.bind()){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();


    std::function<std::string(const std::string&)> func_echo = [](const std::string& in)->std::string{
        return in;
    };
    server.RegisterMethod("echo", func_echo);

    server.Loop();
}
