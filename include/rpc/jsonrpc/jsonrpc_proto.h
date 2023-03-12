#ifndef MYRPC_JSONRPC_PROTO_H
#define MYRPC_JSONRPC_PROTO_H

#include <string>

#include "buffer/ringbuffer.h"
#include "serialization/json_deserializer.h"
#include "serialization/json_serializer.h"

#include "rpc/jsonrpc/jsonrpc_struct.h"

namespace MyRPC{
    namespace JsonRPC {
        class Proto {
        public:
            Proto(ReadRingBuffer &rd, WriteRingBuffer &wr) : m_ser(wr), m_deser(rd) {}

            /**
             * @brief 解析客户端传过来的方法名，由服务端调用
             * @param method_name 输出值，输出客户端的方法名
             */
            void ParseMethod(std::string &method_name);

            /**
             * @brief 解析客户端传过来的请求，由服务端调用
             */
            template<class ArgType>
            void ParseRequest(ArgType &&content) {

            }

            /**
             * @brief 设置错误类型，由服务端调用
             */
            void SetError(Errortype e);

            /**
             * @brief 设置方法名，由客户端调用
             */
            void SetMethod(const std::string &method_name);

            /**
             * @brief 向服务端发送请求，由客户端调用
             */
            template<class ArgType>
            void SendRequest(ArgType &&content) {

            }

            /**
             * @brief 解析服务端传过来的错误类型，由客户端调用
             */
            Errortype ParseError();

        private:
            JsonSerializer m_ser;
            JsonDeserializer m_deser;
        };
    }
}

#endif //MYRPC_JSONRPC_PROTO_H
