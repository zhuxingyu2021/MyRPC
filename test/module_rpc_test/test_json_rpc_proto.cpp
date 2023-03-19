#include "net/tcp_server.h"
#include "rpc/jsonrpc/jsonrpc_proto.h"
#include "traits.h"

#include <vector>
#include <iostream>

using namespace MyRPC;
using namespace std;

int sum(vector<int>& v){
    int ret_val = 0;
    for(auto num: v)
        ret_val += num;
    return ret_val;
}


class SimpleJsonRPCServer: public TCPServer{
public:
    SimpleJsonRPCServer(const InetAddr::ptr& bind_addr, FiberPool::ptr& fiberPool, ms_t timeout=0)
            : TCPServer(bind_addr, fiberPool, timeout){
        config_timeout = timeout;

        SetConnectionClass<TCPServerConn>();
        AddConnectionHandler(JsonRPCConnHandler);
    }

private:
    using Func = function_traits<decltype(sum)>;
    using ArgType = function_traits<decltype(sum)>::arg_type;
    using ResultType = function_traits<decltype(sum)>::result_type;

    inline static ms_t config_timeout;

    static void JsonRPCConnHandler(TCPServerConn::ptr conn){
        ReadRingBuffer rd_buf(conn->GetSock(), config_timeout);
        WriteRingBuffer wr_buf(conn->GetSock(), config_timeout);
        JsonRPC::Proto proto(rd_buf, wr_buf, conn);

        while(true) {
            ArgType args;
            ResultType result;
            // 解析客户端传过来的方法名
            auto error = proto.ParseMethod();
            Logger::info("Method name is given, error:{}", JsonRPC::ToString(error));
            if(error == JsonRPC::NO_ERROR) {
                if (proto.RequestStruct().method == "sum") {
                    // 解析客户端传过来的参数
                    Logger::info("Method name: sum");
                    error = proto.ParseRequest(args);
                    Logger::info("Arguments is given, error:{}", JsonRPC::ToString(error));
                    result = sum(std::get<0>(args));
                } else {
                    Logger::info("Method name: {}", proto.RequestStruct().method);
                    error = JsonRPC::METHOD_NOT_FOUND;
                    proto.SetError(JsonRPC::METHOD_NOT_FOUND);
                }
            }
            if(error == JsonRPC::CLIENT_CLOSE || error == JsonRPC::_OTHER_NET_ERROR){
                return;
            }
            proto.SendResponse(result);
            wr_buf.Flush();
            if(error != JsonRPC::NO_ERROR){
                return;
            }
        }
    }
};

int main(){
    FiberPool::ptr pool(new FiberPool(4));
    int port;
    cin >> port;
    SimpleJsonRPCServer server(make_shared<InetAddr>("127.0.0.1", port),pool, 0);

    pool->Start();

    if(!server.bind()){
        Logger::error("bind error");
        return -1;
    }
    server.Start();
    server.Loop();

}
