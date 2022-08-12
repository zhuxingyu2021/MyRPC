#ifndef MYRPC_NET_EXCEPTION_H
#define MYRPC_NET_EXCEPTION_H

#include <exception>
#include <string>
#include <string_view>
#include <sstream>

#include <cstring>

namespace MyRPC{
    /**
     * @brief 当IP地址非法时，抛出该异常
     */
    class InetInvalidAddrException: public std::exception{
    public:
        InetInvalidAddrException(std::string_view msg):msg_(msg){}
        const char* what() const noexcept override{
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };

    /**
     * @brief 当Socket系统调用失败时，抛出异常。可以使用GetErrno()函数获得抛出异常时的系统调用错误信息
     */
    class SocketException: public std::exception{
    public:
        SocketException(std::string_view api_name){
            std::stringstream ss;
            errno_ = errno;
            ss << api_name << " error: " << strerror(errno_);
            msg_ = std::move(ss.str());
        }
        const char* what() const noexcept override{
            return msg_.c_str();
        }
        int GetErrno() const{return errno_;}
    private:
        std::string msg_;
        int errno_;
    };

    /**
     * @brief 当Socket发生非系统调用原因造成的错误时，抛出异常
     * @note  err_type - 1. 在Socket::RecvAllTimeout函数中，如果在第一次已接收到数据后发生接收超时，会抛出此异常
     */
    class SocketNotSysCallException: public std::exception{
    public:
        SocketNotSysCallException(int err_type):m_err_type(err_type){}
        const char* what() const noexcept override{
            switch(m_err_type){
                case 1:
                    return "TCP recv-all error: timeout after first reception";
                default:
                    return nullptr;
            }
        }

        int GetErrType() const{return m_err_type;}

    private:
        int m_err_type;
    };

    /**
     * @brief Json解析异常
     */
    class JsonDeserializerException: public std::exception{
    public:
        JsonDeserializerException(int pos):msg_("Json deserializer error in position: " + std::to_string(pos)){}
        const char* what() const noexcept override{
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };
}

#endif //MYRPC_NET_EXCEPTION_H
