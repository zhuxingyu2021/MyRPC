#ifndef MYRPC_TCPCLIENT_H
#define MYRPC_TCPCLIENT_H

#include <memory>

namespace MyRPC{
class TCPClient: public std::enable_shared_from_this<TCPClient>{
        using ptr = std::shared_ptr<TCPClient>;


    };
}

#endif //MYRPC_TCPCLIENT_H
