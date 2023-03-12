#ifndef MYRPC_CONFIG_H
#define MYRPC_CONFIG_H

#include <memory>

#include <string>

#include "net/serializer.h"
#include "net/deserializer.h"
#include "net/inetaddr.h"
#include "buffer/stringbuffer.h"
#include "noncopyable.h"

namespace MyRPC{
    class Config:public NonCopyable{
    public:
        using ptr = std::shared_ptr<Config>;
        using unique_ptr = std::unique_ptr<Config>;

        friend JsonSerializer;
        friend JsonDeserializer;

        Config(): m_registry_server_addr(std::move(std::make_shared<InetAddr>("127.0.0.1", 9000))){}

        static Config::ptr LoadFromJson(const std::string& json_file);
        void SaveToJson(const std::string& json_file) const;

        int GetThreadsNum() const{return m_threads_num;}
        int GetTimeout() const{return m_timeout;}
        int GetKeepalive() const{return m_keepalive;}
        const InetAddr::ptr& GetRegistryServerAddr() const{return m_registry_server_addr;}
        const std::string& GetLoadBalancer() const{return m_load_balancer;}

    private:
        int m_threads_num = 8;

        int m_timeout = 0; // 超时时间 单位ms
        int m_keepalive = 600; // Heartbeat检测间隔 单位s

        InetAddr::ptr m_registry_server_addr;
        std::string m_load_balancer;

        LOAD_BEGIN
            LOAD_ALIAS_ITEM(ThreadsNum, m_threads_num)
            LOAD_ALIAS_ITEM(SocketTimeout, m_timeout)
            LOAD_ALIAS_ITEM(KeepAlive, m_keepalive)
            LOAD_ALIAS_ITEM(RegistryServerAddr, m_registry_server_addr)
            LOAD_ALIAS_ITEM(LoadBalancer, m_load_balancer)
        LOAD_END

        SAVE_BEGIN
            SAVE_ALIAS_ITEM(ThreadsNum, m_threads_num)
            SAVE_ALIAS_ITEM(SocketTimeout, m_timeout)
            SAVE_ALIAS_ITEM(KeepAlive, m_keepalive)
            SAVE_ALIAS_ITEM(RegistryServerAddr, m_registry_server_addr)
            SAVE_ALIAS_ITEM(LoadBalancer, m_load_balancer)
        SAVE_END
    };
}

#endif //MYRPC_CONFIG_H
