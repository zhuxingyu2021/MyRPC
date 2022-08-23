#ifndef MYRPC_CONFIG_H
#define MYRPC_CONFIG_H

#include <memory>

#include <string>

#include "net/serializer.h"
#include "net/deserializer.h"
#include "net/inetaddr.h"

#include "noncopyable.h"

namespace MyRPC{
    class Config:public NonCopyable{
    public:
        using ptr = std::shared_ptr<Config>;
        using unique_ptr = std::unique_ptr<Config>;

        friend JsonSerializer;
        friend JsonDeserializer;

        Config(): m_registry_server_ip(std::move(std::make_shared<InetAddr>("127.0.0.1", 9000))){}

        static Config::ptr LoadFromJson(const std::string& json_file);
        void SaveToJson(const std::string& json_file) const;

        int GetThreadsNum() const{return m_threads_num;}
        int GetTimeout() const{return m_timeout;}
        int GetKeepalive() const{return m_keepalive;}
        bool IsIPv6() const{return m_ipv6;}
        const InetAddr::ptr& GetRegistryServerIP() const{return m_registry_server_ip;}

    private:
        int m_threads_num = 8;
        bool m_ipv6 = false;

        int m_timeout = 2000; // 超时时间 单位ms
        int m_keepalive = 5;

        InetAddr::ptr m_registry_server_ip;

        LOAD_BEGIN
            LOAD_ALIAS_ITEM(ThreadsNum, m_threads_num)

            LOAD_ALIAS_ITEM(SocketTimeout, m_timeout)
            LOAD_ALIAS_ITEM(KeepAlive, m_keepalive)
            LOAD_ALIAS_ITEM(RegistryServerIP, m_registry_server_ip)
            m_ipv6 = m_registry_server_ip->IsIPv6();
        LOAD_END

        SAVE_BEGIN
            SAVE_ALIAS_ITEM(ThreadsNum, m_threads_num)
            SAVE_ALIAS_ITEM(SocketTimeout, m_timeout)
            SAVE_ALIAS_ITEM(KeepAlive, m_keepalive)
            SAVE_ALIAS_ITEM(RegistryServerIP, m_registry_server_ip)
        SAVE_END
    };
}

#endif //MYRPC_CONFIG_H
