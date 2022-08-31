#include "rpc/rpc_client.h"

#include <iostream>
#include <string>

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

    RPCClient client(config);

    if(!client.ConnectToRegistryServer()){
        Logger::error("Can't connect to registry server!");
        exit(-1);
    }

    auto future = client.InvokeAsync<std::string>("echo", std::string("Hello world!"));

    std::cout << future.get() << std::endl;

}
