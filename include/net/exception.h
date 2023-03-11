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
    class NetException: public std::exception{
    public:
        enum ErrorType{
            SYS = 0,
            TIMEOUT,
            CONN_CLOSE,
            BUFFER
        };

        NetException(std::string_view api_name, ErrorType type){
            std::stringstream ss;
            switch(type) {
                case SYS:
                errno_ = errno;
                ss << api_name << " error: " << strerror(errno_);
                break;

                case TIMEOUT:
                ss << api_name << " error: timeout";
                break;

                case CONN_CLOSE:
                ss << api_name << " error: peer connection close";
                break;

                case BUFFER:
                ss << api_name << " error: buffer is full";
                break;

                default:
                MYRPC_ASSERT(false);
            }
            type_ = type;
            msg_ = std::move(ss.str());
        }
        const char* what() const noexcept override{
            return msg_.c_str();
        }
        int GetErrno() const{return errno_;}
        ErrorType GetErrType() const{return type_;}
    private:
        std::string msg_;
        int errno_;
        ErrorType type_;
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
