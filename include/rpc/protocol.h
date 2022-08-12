#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H

#include <memory>

namespace MyRPC{
    class Protocol: public std::enable_shared_from_this<Protocol>{
    public:
        using ptr = std::shared_ptr<Protocol>;

    private:

    };
}

#endif //MYRPC_PROTOCOL_H
