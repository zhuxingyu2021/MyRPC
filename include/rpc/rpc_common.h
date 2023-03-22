#ifndef MYRPC_RPC_COMMON_H
#define MYRPC_RPC_COMMON_H

#include <macro.h>

namespace MyRPC{
    namespace Common{
        ENUM_DEF_2(Errortype, (NO_ERROR, 0),
                   (SYNTAX_ERROR, 1),
                   (INVALID_REQUEST, 2),
                   (INVALID_PARAMS, 3),
                   (INTERNAL_ERROR, 4),
                   (SERVER_ERROR, 5),
                   (METHOD_NOT_FOUND, 10),
                   (EXCEPTION, 20),

                   (NET_TIMEOUT, -1),
                   (NET_PEER_CLOSE, -2),
                   (NET_OTHER, -3)
                   )

        ENUM_DEF_2(Prototype, (JSONRPC2, 0))
    }
}

#endif //MYRPC_RPC_COMMON_H
