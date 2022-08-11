#ifndef MYRPC_EXCEPTION_H
#define MYRPC_EXCEPTION_H

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
        InetInvalidAddrException(const std::string& msg):msg_(msg){}
        const char* what() const noexcept override{
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };

    class SocketException: public std::exception{
    public:
        SocketException(std::string_view api_name){
            std::stringstream ss;
            ss << api_name << " error: " << strerror(errno);
            msg_ = std::move(ss.str());
        }
        const char* what() const noexcept override{
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };

}

#endif //MYRPC_EXCEPTION_H
