#ifndef MYRPC_JSONRPC_PROTO_H
#define MYRPC_JSONRPC_PROTO_H

#include <string>

#include "buffer/ringbuffer.h"
#include "macro.h"

namespace MyRPC{
    class JsonRPCProto{
    public:
        ENUM_DEF_2(JsonRPCError,
                   (NO_ERROR, 0),
                   (PARSE_ERROR, -32700),
                   (INVALID_REQUEST, -32600),
                   (METHOD_NOT_FOUND, -32601),
                   (INVALID_PARAMS, -32602),
                   (INTERNAL_ERROR, -32603),
                   (SERVER_ERROR, -32000)
                   )

        JsonRPCProto(ReadRingBuffer& rd, WriteRingBuffer& wr):m_read_buffer(rd), m_write_buffer(wr){}

        /**
         * @brief 解析客户端传过来的方法名，由服务端调用
         * @param method_name 输出值，输出客户端的方法名
         */
        void ParseMethod(std::string& method_name);

        /**
         * @brief 解析客户端传过来的参数，由服务端调用
         */
        template<class ArgType>
        void ParseArg(ArgType&& content) {

        }

        /**
         * @brief 设置错误类型，由服务端调用
         */
        void SetError(JsonRPCError e);

        /**
         * @brief 设置方法名，由客户端调用
         */
        void SetMethod(const std::string& method_name);

        /**
         * @brief 设置参数，由客户端调用
         */
        template<class ArgType>
        void SetArg(ArgType&& content) {

        }

        /**
         * @brief 解析服务端传过来的错误类型，由客户端调用
         */
        JsonRPCError ParseError();

    private:
        ReadRingBuffer& m_read_buffer;
        WriteRingBuffer& m_write_buffer;
    };
}

#endif //MYRPC_JSONRPC_PROTO_H
