#include "rpc/rpc_registry_server.h"

#include <string>
#include <cstdio>

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

    RpcRegistryServer server(config);

    if(!server.Bind(config->GetRegistryServerIP())){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();

    server.Loop();
}
