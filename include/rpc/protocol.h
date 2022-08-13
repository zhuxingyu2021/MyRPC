#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H

#include <memory>
#include <atomic>

#include "stringbuffer.h"
#include "net/socket.h"
#include "net/serializer.h"
#include "net/deserializer.h"

#include "noncopyable.h"

namespace MyRPC{
    class Protocol: public NonCopyable{
    public:
        using ptr = std::shared_ptr<Protocol>;
        Protocol(Socket& s):m_sock(s){}

        enum MessageType{
            MESSAGE_HEARTBEAT,

            MESSAGE_REQUEST_SUBSCRIBE,
            MESSAGE_REQUEST_REGISTRATION,

            MESSAGE_RESPOND_NORMAL,

            ERROR_TIMEOUT
        };

        MessageType ParseHeader();
        StringBuffer GetContent();

        template<class ContentType>
        void ParseContent(ContentType& content){
            StringBuffer buffer(GetContent());
            Deserializer des(buffer);
            des.Load(content);
        }

        template<class ContentType>
        void Send(MessageType msg_type, const ContentType& content){
            StringBuilder sb;
            Serializer ser(sb);
            ser.Save(content);
            StringBuffer buffer(sb.Concat());
        }

    private:
        Socket& m_sock;
    };
}

#endif //MYRPC_PROTOCOL_H
