#include "rpc/rpc_session.h"
#include "rpc/exception.h"

using namespace MyRPC;

MessageType RPCSession::RecvAndParseHeader() {
    uint8_t buf[HEADER_LENGTH];
    auto recv_result = m_sock.RecvAllTimeout(buf, HEADER_LENGTH, 0, m_sock_timeout);

    if(recv_result > 0) {
        if(buf[0] != MAGIC_NUMBER) return ERROR_UNKNOWN_PROTOCOL;
        if(buf[1] != VERSION) return ERROR_UNKNOWN_PROTOCOL;

        // 解析内容长度并返回消息类型
        uint32_t content_length_net;
        memcpy(&content_length_net, buf + 3, 4);
        auto content_length = ntohl(content_length_net); // 网络字节序转化为本地字节序

        // 读取内容
        if(content_length > 0){
            StringBuffer content(content_length);
            content.size = content_length;
            auto recv_result2 = m_sock.RecvAllTimeout(content.data, content_length, 0, m_sock_timeout);
            if(recv_result2 == 0) return ERROR_CLIENT_CLOSE_CONN; // 客户端关闭连接
            else if(recv_result2 < 0) MYRPC_ASSERT(false);

            m_content = std::move(content);
        }else{
            StringBuffer null_content;
            m_content = std::move(null_content);
        }

        return MessageType(buf[2]);
    }else if(recv_result == 0){
        // 客户端关闭连接
        return ERROR_CLIENT_CLOSE_CONN;
    }else{
        // 超时
        return ERROR_TIMEOUT;
    }

}
