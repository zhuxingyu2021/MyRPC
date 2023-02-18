#ifndef MYRPC_LOAD_BALANCE_H
#define MYRPC_LOAD_BALANCE_H

#include <map>
#include <string>
#include "net/socket.h"

namespace MyRPC{
    class LoadBalancer{
    public:
        using service_table_it = std::multimap<std::string, InetAddr::ptr>::iterator;
        virtual service_table_it Select(service_table_it, service_table_it, const std::string& ) = 0;
    };

    // 随机负载均衡方案
    class RandomLoadBalancerImpl: public LoadBalancer{
    public:
        using service_table_it = std::multimap<std::string, InetAddr::ptr>::iterator;
        service_table_it Select(service_table_it beg, service_table_it end, const std::string& service_name){
            int sn_req_sz = std::distance(beg, end); // 服务器的数量
            int select = rand()%sn_req_sz;
            auto iter = beg;
            while(select>0){
                ++iter;
                --select;
            }
            return iter;
        }
    };

    // 一致性Hash负载均衡方案
    class HashLoadBalancerImpl: public LoadBalancer{
    public:
        using service_table_it = std::multimap<std::string, InetAddr::ptr>::iterator;
        HashLoadBalancerImpl(std::string_view local_host):m_local_host(local_host){}

        service_table_it Select(service_table_it beg, service_table_it end, const std::string& service_name){
            std::map<size_t, service_table_it> hash_circle;
            size_t local_hash = std::hash<std::string>()(m_local_host + service_name);

            for(auto it = beg; it != end; ++it){
                hash_circle.emplace(std::hash<std::string>()(it->first), it);
            }
            auto it_next = hash_circle.upper_bound(local_hash);
            if(it_next == hash_circle.end()){
                return hash_circle[0];
            }else{
                return it_next->second;
            }
        }

    private:
        std::string m_local_host;
    };
}

#endif