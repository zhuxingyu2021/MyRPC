#ifndef MYRPC_SOCKET_H
#define MYRPC_SOCKET_H

#include <memory>
#include <unistd.h>

#include "fiber/timeout_io.h"
#include "net/exception.h"

#include "macro.h"

namespace MyRPC{
    // RAII Socket对象
    class Socket: public std::enable_shared_from_this<Socket> {
    public:
        using ptr = std::shared_ptr<Socket>;
        using unique_ptr = std::unique_ptr<Socket>;

        Socket(int sockfd):m_socketfd(sockfd){}
        Socket(const Socket&) = delete;
        Socket(Socket&&) = delete;

        ~Socket(){
            if(m_socketfd != -1){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("socket fd:{} closed", m_socketfd);
#endif
                close(m_socketfd);
            }
        }

        int GetSocketfd() const{
            return m_socketfd;
        }

        ssize_t Send(const void *buf, size_t len, int flags){
            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = send(m_socketfd, buf,len,flags))>=0, SocketException("TCP send"));
            return ret;
        }

        ssize_t Recv(void *buf, size_t len, int flags){
            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = recv(m_socketfd, buf,len,flags))>=0, SocketException("TCP recv"));
            return ret;
        }

        ssize_t RecvTimeout(void *buf, size_t len, int flags, __useconds_t timeout){
            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = recv_timeout(m_socketfd, buf,len,flags, timeout))!=-1, SocketException("TCP send timeout"));
            return ret;
        }

        void SendAll(const void *buf, size_t len, int flags){
            size_t send_sz = 0;
            ssize_t ret;
            do{
                MYRPC_ASSERT_EXCEPTION((ret = send(m_socketfd, (const char*)buf+send_sz,len-send_sz,flags))>=0, SocketException("TCP send"));
                send_sz += ret;
            }while(send_sz < len);
        }

        /**
         * @brief 从远程计算机接收len个字节（保证接收到的字节数等于len）
         * @param buf 缓冲区
         * @param len 接收的字节数
         * @param flags
         * @return 若接收成功，返回len；若对方已关闭连接，返回0；若recv系统调用失败，则抛出SocketException异常
         */
        ssize_t RecvAll(void *buf, ssize_t len, int flags){
            size_t recv_sz = 0;
            ssize_t ret;
            MYRPC_ASSERT(len >= 0);
            do{
                MYRPC_ASSERT_EXCEPTION((ret = recv(m_socketfd, buf,len,flags))>=0, SocketException("TCP recv"));
                if(ret == 0) return 0; // 对方已关闭连接
                recv_sz += ret;
            }while(recv_sz < len);
        }

        /**
         * @brief 从远程计算机接收len个字节，并有超时时间限制（保证接收到的字节数为len）
         * @param buf 缓冲区
         * @param len 接收的字节数
         * @param flags
         * @param timeout 超时时间
         * @return 若接收成功，返回len；若超时，返回负数；若对方已关闭连接，返回0；若recv系统调用失败，则抛出SocketException异常；若第一次已接收到数据后，还发生超时，则抛出SocketNotSysCallException异常
         */
        ssize_t RecvAllTimeout(void *buf, ssize_t len, int flags, __useconds_t timeout){
            size_t recv_sz = 0;
            ssize_t ret;
            MYRPC_ASSERT(len >= 0);
            do{
                MYRPC_ASSERT_EXCEPTION((ret = recv(m_socketfd, buf,len,flags))!=-1, SocketException("TCP recv"));

                if(ret == 0) return 0; // 对方已关闭连接
                if(ret == MYRPC_ERR_TIMEOUT_FLAG){
                    // 超时
                    if(recv_sz == 0) return MYRPC_ERR_TIMEOUT_FLAG; // 第一次接收超时
                    else throw SocketNotSysCallException(1);
                }
                recv_sz += ret;
            }while(recv_sz < len);
            return true;
        }

    private:
        int m_socketfd = -1;
    };
}

#endif //MYRPC_SOCKET_H
