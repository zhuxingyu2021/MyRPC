#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H

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

    class Protocol: public NonCopyable{
    public:
        using ptr = std::shared_ptr<Protocol>;
        const static uint8_t MAGIC_NUMBER = 0xE5;
        const static uint8_t VERSION = 0x00;
        const static int HEADER_LENGTH = 7;

        Protocol(Socket& s, __useconds_t socket_timeout = 0):m_sock(s), m_sock_timeout(socket_timeout), m_content(){}

        enum MessageType{ // 请求类型，协议的第三个字节
            MESSAGE_HEARTBEAT = 0,

            MESSAGE_REQUEST_SUBSCRIBE,
            MESSAGE_REQUEST_REGISTRATION,

            MESSAGE_RESPOND_OK,

            ERROR,
            ERROR_TIMEOUT,
            ERROR_CLIENT_CLOSE_CONN,
            ERROR_OTHERS
        };

        MessageType ParseHeader();

        template<class ContentType>
        void ParseContent(ContentType& content){
            if(m_content.size > 0) {
                // 反序列化
                Deserializer des(m_content);
                des.Load(content);
            }
        }

        template<class ContentType>
        void Send(MessageType msg_type, const ContentType& content){
            StringBuilder sb;

            // 构造协议header（内容长度除外）
            StringBuffer header(7);
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

            // 发送package
            m_sock.SendAll(buffer.data, buffer.size, 0);
        }

    private:
        Socket& m_sock;

        const __useconds_t m_sock_timeout = 0;

        StringBuffer m_content;
    };
}

#endif //MYRPC_PROTOCOL_H
