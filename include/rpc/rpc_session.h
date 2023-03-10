#ifndef MYRPC_RPC_SESSION_H
#define MYRPC_RPC_SESSION_H

#include <memory>
#include <atomic>

#include "stringbuffer.h"
#include "net/socket.h"
#include "net/serializer.h"
#include "net/deserializer.h"
#include <arpa/inet.h>

#include "noncopyable.h"

namespace MyRPC{

/*
 * 私有通信协议
 * +--------+--------+-----------------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * |  BYTE  |        |                 |        |        |        |             ........                                                           |
 * +--------------------------------------------+--------+--------------------------+--------+-----------------+--------+--------+--------+--------+
 * |  magic | version|  message type  |          content length           |             content byte[]                                             |
 * +--------+-----------------------------------------------------------------------------------------------------------------------------+---------
 *
 * Header一共七个字节
 * 第一个字节是魔法数。
 * 第二个字节代表协议版本号，以便对协议进行扩展，使用不同的协议解析器。
 * 第三个字节是请求类型。
 * 第四-七个字节是接收的内容长度。
 */

    enum MessageType{
        MESSAGE_HEARTBEAT,
        MESSAGE_REQUEST_SUBSCRIBE,
        MESSAGE_REQUEST_REGISTRATION,
        MESSAGE_REQUEST_RPC,
        MESSAGE_RESPOND_OK,
        MESSAGE_RESPOND_EXCEPTION,
        MESSAGE_RESPOND_ERROR,
        MESSAGE_PUSH,
        ERROR_TIMEOUT,
        ERROR_CLIENT_CLOSE_CONN,
        ERROR_UNKNOWN_PROTOCOL,
        ERROR_OTHERS
    };

    inline const char* ToString(MessageType e){
        switch(e){
            case MESSAGE_HEARTBEAT: return "MESSAGE_HEARTBEAT";
            case MESSAGE_REQUEST_SUBSCRIBE: return "MESSAGE_REQUEST_SUBSCRIBE";
            case MESSAGE_REQUEST_REGISTRATION: return "MESSAGE_REQUEST_REGISTRATION";
            case MESSAGE_REQUEST_RPC: return "MESSAGE_REQUEST_RPC";
            case MESSAGE_RESPOND_OK: return "MESSAGE_RESPOND_OK";
            case MESSAGE_RESPOND_EXCEPTION: return "MESSAGE_RESPOND_EXCEPTION";
            case MESSAGE_RESPOND_ERROR: return "MESSAGE_RESPOND_ERROR";
            case MESSAGE_PUSH: return "MESSAGE_PUSH";
            case ERROR_TIMEOUT: return "ERROR_TIMEOUT";
            case ERROR_CLIENT_CLOSE_CONN: return "ERROR_CLIENT_CLOSE_CONN";
            case ERROR_UNKNOWN_PROTOCOL: return "ERROR_UNKNOWN_PROTOCOL";
            case ERROR_OTHERS: return "ERROR_OTHERS";
        }
    }

    class RPCSession: public NonCopyable{
    public:
        using ptr = std::shared_ptr<RPCSession>;
        using weak_ptr = std::weak_ptr<RPCSession>;

        const static uint8_t MAGIC_NUMBER = 0xE5;
        const static uint8_t VERSION = 0x00;
        const static int HEADER_LENGTH = 7;

        RPCSession(Socket& s, useconds_t socket_timeout = 0): m_sock(s), m_sock_timeout(socket_timeout), m_content(),
                                                              m_peer_ip(std::move(m_sock.GetPeerAddr())){}

        MessageType RecvAndParseHeader();

        StringBuffer GetContent(){
            StringBuffer ret_buf(std::move(m_content));
            return ret_buf;
        }

        template<class ContentType>
        inline static void ParseContent(StringBuffer& content_buffer, ContentType& content){
            if(content_buffer.size > 0) {
                // 反序列化
                Deserializer des(content_buffer);
                des.Load(content);
            }
        }

        template<class ContentType>
        void ParseContent(ContentType&& content){
            ParseContent(m_content, std::forward<ContentType>(content));
        }

        void ParseServiceName(std::string& service_name){
            // 反序列化
            Deserializer des(m_content);
            des.Load(service_name);
        }

        static StringBuffer Prepare(MessageType msg_type){
            // 构造协议header（内容长度为0）
            StringBuffer header(HEADER_LENGTH);
            header.size = HEADER_LENGTH;
            header.data[0] = MAGIC_NUMBER;
            header.data[1] = VERSION;
            header.data[2] = (uint8_t)msg_type;
            memset(header.data + 3, 0, sizeof(uint32_t)); // 内容长度设置为0

            return header;
        }

        template<class ContentType>
        static StringBuffer Prepare(MessageType msg_type, const ContentType& content){
            StringBuilder sb;

            // 构造协议header（内容长度除外）
            StringBuffer header(HEADER_LENGTH);
            header.size = HEADER_LENGTH;
            header.data[0] = MAGIC_NUMBER;
            header.data[1] = VERSION;
            header.data[2] = (uint8_t)msg_type;
            sb.Append(std::move(header));

            // 序列化
            Serializer ser(sb);
            ser.Save(content);

            // 构造协议package并计算内容长度
            StringBuffer buffer(sb.Concat());

            auto content_length_net = htonl(buffer.size - HEADER_LENGTH); // 计算内容长度，并转换为网络字节序

            memcpy(buffer.data + 3, &content_length_net, sizeof(uint32_t)); // 将内容长度写入package中

            return buffer;
        }

        template<class ContentType>
        static StringBuffer Prepare(MessageType msg_type, const std::string& service_name, const ContentType& content){
            StringBuilder sb;

            // 构造协议header（内容长度除外）
            StringBuffer header(HEADER_LENGTH);
            header.size = HEADER_LENGTH;
            header.data[0] = MAGIC_NUMBER;
            header.data[1] = VERSION;
            header.data[2] = (uint8_t)msg_type;
            sb.Append(std::move(header));

            // 序列化
            Serializer ser(sb);
            ser.Save(service_name);
            ser.Save(content);

            // 构造协议package并计算内容长度
            StringBuffer buffer(sb.Concat());

            auto content_length_net = htonl(buffer.size - HEADER_LENGTH); // 计算内容长度，并转换为网络字节序

            memcpy(buffer.data + 3, &content_length_net, sizeof(uint32_t)); // 将内容长度写入package中

            return buffer;
        }

        inline void Send(const StringBuffer& buffer){
            m_sock.SendAll(buffer.data, buffer.size, 0);
        }

        template <class ...Args>
        void PrepareAndSend(Args... args){
            StringBuffer sb = std::move(Prepare(std::forward<Args>(args)...));
            Send(sb);
        }

        Socket& GetSocket() const{return m_sock;}
        InetAddr::ptr GetPeerIP() const{return m_peer_ip;}

    private:
        Socket& m_sock;

        const useconds_t m_sock_timeout = 0;

        StringBuffer m_content;

        InetAddr::ptr m_peer_ip;
    };
}

#endif //MYRPC_RPC_SESSION_H
