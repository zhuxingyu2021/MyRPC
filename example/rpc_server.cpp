#include "rpc/rpc_server.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace MyRPC;

int sum(vector<int>& v){
    int ret_val = 0;
    for(auto num: v)
        ret_val += num;
    return ret_val;
}

int main(){
    int port;
    cin >> port;

    RPCServer server(make_shared<InetAddr>("127.0.0.1", port), Common::JSONRPC2);

    server.bind();
    server.Start();
    server.AddMethod("sum", sum);

    server.Loop();
}
