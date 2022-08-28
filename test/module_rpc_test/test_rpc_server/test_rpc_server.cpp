#include "rpc/rpc_server.h"

#include <string>
#include <cstdio>
#include <iostream>

using namespace MyRPC;

int main(int argc, char** argv){
    Config::ptr config(nullptr);
    if(argc >= 2){
        std::string arg1(argv[1]);
        if(arg1 == "-c"){
            config = std::move(Config::LoadFromJson(arg1));
        }else{
            printf("Usage: %s -c <json_config_file>\n", argv[0]);
            return -1;
        }
    }else{
        config = std::move(std::make_shared<Config>());
    }

    RPCServer server(config);

    if(!server.bind(std::make_shared<InetAddr>("127.0.0.1", 5678))){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();

    server.ConnectToRegistryServer();

    std::function<std::string(const std::string&)> func_echo = [](const std::string& in)->std::string{
        return in;
    };
    server.RegisterMethod("echo", func_echo);

    server.Loop();
}