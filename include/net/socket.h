#ifndef MYRPC_SOCKET_H
#define MYRPC_SOCKET_H

#include <memory>
#include <unistd.h>
#include <mutex>
#include <sys/uio.h>

#include "fiber/timeout_io.h"
#include "fiber/fiber_sync.h"
#include "net/exception.h"
#include "net/inetaddr.h"

#include "macro.h"
#include "fd_raii.h"


namespace MyRPC{
    // RAII Socket对象
    template<class MutexType>
    class MutexSocket: public FdRAIIWrapper{
    public:
        using ptr = std::shared_ptr<MutexSocket<MutexType>>;
        using unique_ptr = std::unique_ptr<MutexSocket<MutexType>>;
        using weak_ptr = std::weak_ptr<MutexSocket<MutexType>>;

        MutexSocket(int sockfd): FdRAIIWrapper(sockfd){}

        ~MutexSocket(){
            if(Closefd()){
#if MYRPC_DEBUG_LEVEL >= MYRPC_DEBUG_NET_LEVEL
                Logger::debug("socket fd:{} closed", m_fd);
#endif
            }
        }

        inline int GetSocketfd() const{
            return m_fd;
        }

        ssize_t Send(const void *buf, size_t len, int flags){
            std::unique_lock<MutexType> lock(m_send_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = send(m_fd, buf,len,flags))>=0, throw NetException("TCP send", NetException::SYS));
            return ret;
        }

        ssize_t SendTimeout(const void *buf, size_t len, int flags, useconds_t timeout){
            std::unique_lock<MutexType> lock(m_send_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = send_timeout(m_fd, buf,len,flags, timeout))!=-1, throw NetException("TCP send timeout", NetException::SYS));
            return ret;
        }

        ssize_t Recv(void *buf, size_t len, int flags){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = recv(m_fd, buf,len,flags))>=0, throw NetException("TCP recv", NetException::SYS));
            if(ret == 0) throw NetException("TCP recv", NetException::CONN_CLOSE);
            return ret;
        }

        ssize_t RecvTimeout(void *buf, size_t len, int flags, useconds_t timeout){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = recv_timeout(m_fd, buf,len,flags, timeout))!=-1, throw NetException("TCP recv timeout", NetException::SYS));
            if(ret == MYRPC_ERR_TIMEOUT_FLAG)
                throw NetException("TCP recv timeout", NetException::TIMEOUT);
            if(ret == 0) throw NetException("TCP recv timeout", NetException::CONN_CLOSE);
            return ret;
        }

        ssize_t Readv(const struct iovec *iov, int iovcnt){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = readv(m_fd, iov, iovcnt))>=0, throw NetException("TCP readv", NetException::SYS));
            if(ret == 0) throw NetException("TCP readv", NetException::CONN_CLOSE);
            return ret;
        }

        ssize_t ReadvTimeout(const struct iovec *iov, int iovcnt, useconds_t timeout){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = readv_timeout(m_fd, iov, iovcnt, timeout))!=-1, throw NetException("TCP readv timeout", NetException::SYS));
            if(ret == MYRPC_ERR_TIMEOUT_FLAG)
                throw NetException("TCP readv timeout", NetException::TIMEOUT);
            if(ret == 0) throw NetException("TCP readv timeout", NetException::CONN_CLOSE);
            return ret;
        }

        ssize_t Writev(const struct iovec *iov, int iovcnt){
            std::unique_lock<MutexType> lock(m_send_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = writev(m_fd, iov, iovcnt))>=0, throw NetException("TCP writev", NetException::SYS));
            return ret;
        }

        ssize_t WritevTimeout(const struct iovec *iov, int iovcnt, useconds_t timeout){
            std::unique_lock<MutexType> lock(m_send_mutex);

            ssize_t ret;
            MYRPC_ASSERT_EXCEPTION((ret = writev_timeout(m_fd, iov, iovcnt, timeout))!=-1, throw NetException("TCP send timeout", NetException::SYS));
            return ret;
        }

        ssize_t SendAll(const void *buf, size_t len, int flags){
            std::unique_lock<MutexType> lock(m_send_mutex);

            size_t send_sz = 0;
            ssize_t ret;
            do{
                MYRPC_ASSERT_EXCEPTION((ret = send(m_fd, (const char*)buf+send_sz,len-send_sz,flags))>=0, throw NetException("TCP sendall", NetException::SYS));
                send_sz += ret;
            }while(send_sz < len);
            return ret;
        }

        /**
         * @brief 从远程计算机接收len个字节（保证接收到的字节数等于len）
         * @param buf 缓冲区
         * @param len 接收的字节数
         * @param flags
         */
        void RecvAll(void *buf, ssize_t len, int flags){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            size_t recv_sz = 0;
            ssize_t ret;
            MYRPC_ASSERT(len >= 0);
            do{
                MYRPC_ASSERT_EXCEPTION((ret = recv(m_fd, (const char*)buf+recv_sz,len-recv_sz,flags))>=0, throw NetException("TCP recvall", NetException::SYS));
                if(ret == 0) throw NetException("TCP recvall", NetException::CONN_CLOSE); // 对方已关闭连接
                recv_sz += ret;
            }while(recv_sz < len);
        }

        /**
         * @brief 从远程计算机接收len个字节，并有超时时间限制（保证接收到的字节数为len）
         * @param buf 缓冲区
         * @param len 接收的字节数
         * @param flags
         * @param timeout 超时时间
         */
        void RecvAllTimeout(void *buf, ssize_t len, int flags, useconds_t timeout){
            std::unique_lock<MutexType> lock(m_recv_mutex);

            size_t recv_sz = 0;
            ssize_t ret;
            MYRPC_ASSERT(len >= 0);
            do{
                MYRPC_ASSERT_EXCEPTION((ret = recv_timeout(m_fd, (const char*)buf+recv_sz,len-recv_sz,flags, timeout))!=-1, throw NetException("TCP recvall timeout", NetException::SYS));

                if(ret == 0) throw NetException("TCP recvall timeout", NetException::CONN_CLOSE); // 对方已关闭连接
                if(ret == MYRPC_ERR_TIMEOUT_FLAG){
                    // 超时
                    throw NetException("TCP recvall timeout", NetException::TIMEOUT);
                }
                recv_sz += ret;
            }while(recv_sz < len);
        }

        void SendAllTimeout(const void *buf, size_t len, int flags){
            std::unique_lock<MutexType> lock(m_send_mutex);

            size_t send_sz = 0;
            ssize_t ret;
            MYRPC_ASSERT(len >= 0);
            do{
                MYRPC_ASSERT_EXCEPTION((ret = send(m_fd, (const char*)buf+send_sz,len-send_sz,flags))>=0, throw NetException("TCP sendall timeout", NetException::SYS));
                MYRPC_ASSERT(ret != 0);

                if(ret == MYRPC_ERR_TIMEOUT_FLAG){
                    // 超时
                    throw NetException("TCP sendall timeout", NetException::TIMEOUT);
                }
                send_sz += ret;
            }while(send_sz < len);
        }

        static MutexSocket<MutexType>::ptr Connect(const InetAddr::ptr& addr, useconds_t conn_timeout=0){
            auto sock_fd = socket(addr->IsIPv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
            MYRPC_ASSERT_EXCEPTION(sock_fd >= 0, throw NetException("TCP connect", NetException::SYS));

            auto err = connect_timeout(sock_fd, addr->GetAddr(), addr->GetAddrLen(), conn_timeout);
            if(err >= 0){
                return std::make_shared<MutexSocket<MutexType>>(sock_fd);
            }else if(err == MYRPC_ERR_TIMEOUT_FLAG){ // 超时
                throw NetException("TCP connect", NetException::TIMEOUT);
            }else{
                throw NetException("TCP connect", NetException::SYS);
            }
        }

        inline InetAddr::ptr GetPeerAddr() const{return InetAddr::GetPeerAddr(m_fd);}

    private:
        MutexType m_send_mutex;
        MutexType m_recv_mutex;
    };

    using Socket = MutexSocket<FiberSync::Mutex>;
    using SocketSTDMutex = MutexSocket<std::mutex>;

    struct _no_lock_type{void lock(){} void unlock(){}};
    using SocketNoLock = MutexSocket<_no_lock_type>;
}

#endif //MYRPC_SOCKET_H
