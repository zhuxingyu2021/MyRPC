#ifndef MYRPC_JSONRPC_STRUCT_H
#define MYRPC_JSONRPC_STRUCT_H

#include <string>
#include "fiber/sync_queue.h"
#include "fiber/fiber.h"
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
            (SERVER_ERROR, -32000, Server error),

            (TIMEOUT_ERROR, -32010, Network timeout),  // 当客户端超时，发送回应并断开连接；当服务端超时，立即断开连接

            // 当以下错误发生时，会立即退出或断开连接
            (CLIENT_CLOSE, -32011, ),
            (_OTHER_NET_ERROR, -32012, )
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

            friend JsonSerializer;
            friend JsonDeserializer;
            friend class Proto;
        private:
            SyncQueue<bool> m_sync;
            Errortype m_internal_err = NO_ERROR;

            LOAD_BEGIN_DEF
                try {
                    LOAD_BEGIN_READ

                    LOAD_ALIAS_ITEM(jsonrpc, version)
                    LOAD_ITEM(id)
                    LOAD_ITEM(method)

                    LOAD_KEY_BEG(params)

                    // 切换协程，以读取Placeholder部分
                    m_sync.Push(true);
                    Fiber::Suspend();
                    if(!m_sync.Pop())
                        return; // Placeholder部分的json解析出现了错误，那就不用继续读下去了，直接退出

                    LOAD_KEY_END
                    LOAD_END_READ

                    m_sync.Push(true);
                }catch(JsonDeserializerException& e){
                    m_internal_err = INVALID_REQUEST;
                    m_sync.Push(false);
                }catch(NetException& e){
                    switch(e.GetErrType()){
                        case NetException::SYS:
                            m_internal_err = _OTHER_NET_ERROR;
                            m_sync.Push(false);
                            break;
                        case NetException::TIMEOUT:
                            m_internal_err = TIMEOUT_ERROR;
                            m_sync.Push(false);
                            break;
                        case NetException::CONN_CLOSE:
                            m_internal_err = CLIENT_CLOSE;
                            m_sync.Push(false);
                            break;
                        case NetException::BUFFER:
                            m_internal_err = INTERNAL_ERROR;
                            m_sync.Push(false);
                            break;
                        default:
                            m_internal_err = SERVER_ERROR;
                            m_sync.Push(false);
                    }
                }catch(...){
                    m_internal_err = SERVER_ERROR;
                    m_sync.Push(false);
                }
            LOAD_END_DEF
        };

        /**
         * @brief 服务器端向客户端发送的回应格式
         */
         struct Response{
         public:
             std::string version; // json-rpc版本号

             Placeholder result; // 返回值
             int id;

             struct ErrorStruct{
             public:
                 Errortype err;

                 friend JsonSerializer;
                 friend JsonDeserializer;
             private:
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
             } error;

             friend JsonSerializer;
             friend JsonDeserializer;
             friend class Proto;
         private:
             SyncQueue<bool> mutable m_sync;
             Errortype mutable m_internal_err = NO_ERROR;

             SAVE_BEGIN_DEF
                 try {
                     SAVE_BEGIN_WRITE

                     SAVE_ALIAS_ITEM(jsonrpc, version)
                     if(error.err == NO_ERROR){
                         // 切换协程，以写入Placeholder部分
                         SAVE_KEY_BEG(result)
                         m_sync.Push(true);
                         Fiber::Suspend();
                         if(!m_sync.Pop())
                             return; // Placeholder部分的json解析出现了错误，那就不用继续写下去了，直接退出

                         SAVE_KEY_END
                         SAVE_ITEM(id)
                         SAVE_END_WRITE

                         m_sync.Push(true);
                     }else{
                         // 写入error部分
                         SAVE_ITEM(error)
                         SAVE_ITEM(id)
                         SAVE_END_WRITE
                     }

                 }catch(NetException& e){
                     switch(e.GetErrType()){
                         case NetException::SYS:
                         case NetException::TIMEOUT:
                             m_internal_err = _OTHER_NET_ERROR;
                             m_sync.Push(false);
                             break;
                         case NetException::CONN_CLOSE:
                             m_internal_err = CLIENT_CLOSE;
                             m_sync.Push(false);
                             break;
                         default:
                             m_internal_err = SERVER_ERROR;
                             m_sync.Push(false);
                     }
                 }catch(...){
                     m_internal_err = SERVER_ERROR;
                     m_sync.Push(false);
                 }
             SAVE_END_DEF
         };
    }
}

#endif //MYRPC_JSONRPC_STRUCT_H
