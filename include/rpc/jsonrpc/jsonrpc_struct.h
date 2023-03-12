#ifndef MYRPC_JSONRPC_STRUCT_H
#define MYRPC_JSONRPC_STRUCT_H

#include <string>
#include "fiber/sync_queue.h"
#include "macro.h"

namespace MyRPC{
    namespace JsonRPC{
        ENUM_DEF_3(Errortype,
            (NO_ERROR, 0, ),
            (PARSE_ERROR, -32700, Parse error),
            (INVALID_REQUEST, -32600, Invalid Request),
            (METHOD_NOT_FOUND, -32601, Method not found),
            (INVALID_PARAMS, -32602, Invalid params),
            (INTERNAL_ERROR, -32603, Internal error),
            (SERVER_ERROR, -32000, Server error)
        )

        /**
         * @brief 客户端向服务器端发送的请求格式
         */
        struct Request{
        public:
            std::string version; // json-rpc版本号
            std::string method; // 服务名

            Placeholder params; // 参数
            int id;

        private:
            SyncQueue<Errortype> sync;
        };

        /**
         * @brief 服务器端向客户端发送的回应格式
         */
         struct Response{
             std::string version; // json-rpc版本号

             Placeholder result; // 返回值
             int id;

             struct ErrorStruct{
                 Errortype err;

                 LOAD_BEGIN
                     int& _load_err = (int&)err;
                     std::string message;
                     LOAD_ALIAS_ITEM(code, _load_err)
                     LOAD_ALIAS_ITEM(message, message)
                 LOAD_END

                 SAVE_BEGIN
                     SAVE_ALIAS_ITEM(code, (int)err)
                     SAVE_ALIAS_ITEM(message, ToString(err))
                 SAVE_END
             };
         };
    }
}

#endif //MYRPC_JSONRPC_STRUCT_H
