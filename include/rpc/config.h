#ifndef MYRPC_CONFIG_H
#define MYRPC_CONFIG_H

#include <memory>

#include <string>

#include "net/serializer.h"
#include "net/deserializer.h"

namespace MyRPC{
    class Config{
    public:
        using ptr = std::shared_ptr<Config>;
        Config() = default;

        friend JsonSerializer;
        friend JsonDeserializer;
        static Config::ptr LoadFromJson(const std::string& json_file);
        void SaveToJson(const std::string& json_file) const;

        int GetThreadsNum() const{return m_threads_num;}
        int GetTimeout() const{return m_timeout;}
        int GetKeepalive() const{return m_keepalive;}
        bool IsIPv6() const{return m_ipv6;}

    private:
        int m_threads_num = 8;
        bool m_ipv6 = false;

        int m_timeout = 2;
        int m_keepalive = 5;

        LOAD_BEGIN
            LOAD_ALIAS_ITEM(ThreadsNum, m_threads_num)
            LOAD_ALIAS_ITEM(IsIPv6, m_ipv6)
            LOAD_ALIAS_ITEM(SocketTimeout, m_timeout)
            LOAD_ALIAS_ITEM(KeepAlive, m_keepalive)
        LOAD_END

        SAVE_BEGIN
            SAVE_ALIAS_ITEM(ThreadsNum, m_threads_num)
            SAVE_ALIAS_ITEM(IsIPv6, m_ipv6)
            SAVE_ALIAS_ITEM(SocketTimeout, m_timeout)
            SAVE_ALIAS_ITEM(KeepAlive, m_keepalive)
        SAVE_END
    };
}

#endif //MYRPC_CONFIG_H
