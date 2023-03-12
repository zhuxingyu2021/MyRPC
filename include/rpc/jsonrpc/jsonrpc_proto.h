#ifndef MYRPC_JSONRPC_PROTO_H
#define MYRPC_JSONRPC_PROTO_H

#include <string>

#include "buffer/ringbuffer.h"
#include "net/tcp_server_conn.h"
#include "serialization/json_deserializer.h"
#include "serialization/json_serializer.h"

#include "rpc/jsonrpc/jsonrpc_struct.h"

namespace MyRPC{
    namespace JsonRPC {
        class Proto {
        public:
            Proto(ReadRingBuffer &rd, WriteRingBuffer &wr, TCPServerConn* conn) : m_ser(wr), m_deser(rd), m_conn(conn){}

            /**
             * @brief 解析客户端传过来的方法名，由服务端调用 （解析请求的第一阶段）
             * @param method_name 输出值，输出客户端的方法名
             */
            void ParseMethod(){
                m_conn->AddAsyncTask([this]() {
                    m_deser.Load(m_request);
                });
                // 等待前两个字段读完
                m_request.m_sync.Pop();
            }

            /**
             * @brief 解析客户端传过来的参数，由服务端调用（解析请求的第二阶段）
             */
            template<class ParamsType>
            Errortype ParseRequest(ParamsType &&params) noexcept{
                try {
                    // 读第三个字段
                    m_deser.Load(params);

                    // 让出协程，等之后的字段被读完
                    m_request.m_sync.Push(true);
                    Fiber::Suspend();
                    m_request.m_sync.Pop();
                }catch(JsonDeserializerException& e){
                    m_request.m_internal_err = INVALID_REQUEST;
                    m_request.m_sync.Push(false);
                }catch(NetException& e){
                    switch(e.GetErrType()){
                        case NetException::SYS:
                            m_request.m_internal_err = _OTHER_NET_ERROR;
                            m_request.m_sync.Push(false);
                            break;
                        case NetException::TIMEOUT:
                            m_request.m_internal_err = TIMEOUT_ERROR;
                            m_request.m_sync.Push(false);
                            break;
                        case NetException::CONN_CLOSE:
                            m_request.m_internal_err = CLIENT_CLOSE;
                            m_request.m_sync.Push(false);
                            break;
                        case NetException::BUFFER:
                            m_request.m_internal_err = INTERNAL_ERROR;
                            m_request.m_sync.Push(false);
                            break;
                        default:
                            m_request.m_internal_err = SERVER_ERROR;
                            m_request.m_sync.Push(false);
                    }
                }catch(...){
                    m_request.m_internal_err = SERVER_ERROR;
                    m_request.m_sync.Push(false);
                }
                return m_request.m_internal_err;
            }

            /**
             * @brief 设置错误类型，由服务端调用
             */
            void SetError(Errortype e){m_request.m_internal_err = e;}

            template<class ResultType>
            Errortype SendResponse(ResultType &&result) noexcept{
                Errortype ret = NO_ERROR;

                // 填写应答字段
                m_response.version = "2.0";
                m_response.id = m_request.id;
                m_response.error.err = m_request.m_internal_err;

                if(m_response.error.err != NO_ERROR){
                    m_ser.Save(m_response);
                }else {
                    m_conn->AddAsyncTask([this]() {
                        m_ser.Save(m_response);
                    });
                    // 等待第一个字段写完
                    m_response.m_sync.Pop();
                    // 写第二个字段
                    m_ser.Save(result);

                    // 让出协程，等之后的字段被写完
                    m_request.m_sync.Push(true);
                    Fiber::Suspend();
                    m_request.m_sync.Pop();
                }
                return ret;
            }

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

            inline const Request& RequestStruct(){return m_request;}
        private:
            TCPServerConn* m_conn;

            JsonSerializer m_ser;
            JsonDeserializer m_deser;

            Request m_request;
            Response m_response;
        };
    }
}

#endif //MYRPC_JSONRPC_PROTO_H
