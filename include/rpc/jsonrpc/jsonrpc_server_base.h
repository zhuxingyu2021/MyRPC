#ifndef MYRPC_JSONRPC_SERVER_BASE_H
#define MYRPC_JSONRPC_SERVER_BASE_H

#include <memory>
#include <map>
#include "buffer/ringbuffer.h"
#include "net/tcp_server_conn.h"
#include "rpc/rpc_server_base.h"
#include "rpc/jsonrpc/jsonrpc_proto.h"

namespace MyRPC{
    namespace JsonRPC {
        struct RPCServerConn : public TCPServerConn {
            using ptr = std::shared_ptr<RPCServerConn>;

            ~RPCServerConn() {
                delete p_rd;
                delete p_wr;
            }

            ReadRingBuffer *p_rd = nullptr;
            WriteRingBuffer *p_wr = nullptr;
        };

        class JsonRPCServerBase : public RPCServerBase<JsonRPCServerBase, JsonRPC::Proto> {
        public:
            using CTXType = JsonRPC::Proto;

            template<class Args>
            Common::Errortype ParseArgs(CTXType& context, Args &&args) {
                JsonRPC::Errortype err = context.ParseRequest(std::forward<Args>(args));
                return _jsonrpc_to_common_error.at(err);
            }

            template<class Result>
            Common::Errortype Response(CTXType& context, Result &&result, Common::Errortype err) {
                if(err == Common::NO_ERROR) {
                    JsonRPC::Errortype ret_err = context.SendResponse(result);
                    return _jsonrpc_to_common_error.at(ret_err);
                }else{
                    context.SetError(_common_to_jsonrpc_error.at(err));
                    return err;
                }
            }

            void SetError(CTXType& context, Common::Errortype err) {
                context.SetError(_common_to_jsonrpc_error.at(err));
            }

        private:
            void _jsonrpc_conn_handler(RPCServerConn::ptr conn);

            const std::map<JsonRPC::Errortype, Common::Errortype> _jsonrpc_to_common_error = {
                    {JsonRPC::NO_ERROR, Common::NO_ERROR},
                    {JsonRPC::PARSE_ERROR, Common::SYNTAX_ERROR},
                    {JsonRPC::INVALID_REQUEST, Common::INVALID_REQUEST},
                    {JsonRPC::INVALID_PARAMS, Common::INVALID_PARAMS},
                    {JsonRPC::INTERNAL_ERROR, Common::INTERNAL_ERROR},
                    {JsonRPC::SERVER_ERROR, Common::SERVER_ERROR},
                    {JsonRPC::TIMEOUT_ERROR, Common::NET_TIMEOUT},
                    {JsonRPC::CLIENT_CLOSE, Common::NET_PEER_CLOSE},
                    {JsonRPC::_OTHER_NET_ERROR, Common::NET_OTHER}
            };
            const std::map<Common::Errortype, JsonRPC::Errortype> _common_to_jsonrpc_error = {
                    {Common::NO_ERROR, JsonRPC::NO_ERROR},
                    {Common::SYNTAX_ERROR, JsonRPC::PARSE_ERROR},
                    {Common::INVALID_REQUEST, JsonRPC::INVALID_REQUEST},
                    {Common::INVALID_PARAMS, JsonRPC::INVALID_PARAMS},
                    {Common::INTERNAL_ERROR, JsonRPC::INTERNAL_ERROR},
                    {Common::SERVER_ERROR, JsonRPC::SERVER_ERROR},
                    {Common::NET_TIMEOUT, JsonRPC::TIMEOUT_ERROR},
                    {Common::NET_PEER_CLOSE, JsonRPC::CLIENT_CLOSE},
                    {Common::NET_OTHER, JsonRPC::_OTHER_NET_ERROR},

                    {Common::EXCEPTION, JsonRPC::INTERNAL_ERROR}
            };
        };
    }
}

#endif //MYRPC_JSONRPC_SERVER_BASE_H
