#include "rpc/jsonrpc/jsonrpc_server_base.h"

using namespace MyRPC;
using namespace JsonRPC;

void JsonRPCServerBase::_jsonrpc_conn_handler(RPCServerConn::ptr conn) {
    conn->p_rd = new ReadRingBuffer(conn->GetSock(), m_timeout);
    conn->p_wr = new WriteRingBuffer(conn->GetSock(), m_timeout);

    JsonRPC::Proto proto(*conn->p_rd, *conn->p_wr, conn);

    while(true) {
        // 解析客户端传过来的方法名
        auto error = proto.ParseMethod();
#ifdef MYRPC_DEBUG_JSONRPC
        Logger::debug("Method name is given, error:{}, from sockfd: {}", JsonRPC::ToString(error), conn->GetSock()->GetSocketfd());
#endif
        if(error == JsonRPC::NO_ERROR) {
            auto p_service = FindService(proto.RequestStruct().method);
            if (p_service) {
                Common::Errortype err_comm = (*p_service)(proto);
                error = _common_to_jsonrpc_error.at(err_comm);
            } else {
                // 查找不到对应的服务
#ifdef MYRPC_DEBUG_JSONRPC
                Logger::debug("Unknown Method name: {}, from sockfd: {}", proto.RequestStruct().method, conn->GetSock()->GetSocketfd());
#endif
                error = JsonRPC::METHOD_NOT_FOUND;
            }
        }

        proto.SetError(error);

        if(error == JsonRPC::CLIENT_CLOSE || error == JsonRPC::_OTHER_NET_ERROR){
            conn->p_wr->Flush();
            conn->Terminate();
            return;
        }
        if(error != JsonRPC::NO_ERROR){
            proto.SendResponse(0);
            conn->p_wr->Flush();
            conn->Terminate();
            return;
        }

        conn->p_wr->Flush();
    }
}
