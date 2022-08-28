#ifndef MYRPC_RPC_EXCEPTION_H
#define MYRPC_RPC_EXCEPTION_H

#include <exception>
#include <string>
#include <cstring>

namespace MyRPC{
    class FileException:public std::exception{
        public:
            FileException(const std::string& msg):msg_(msg){}
            const char* what() const noexcept override{return msg_.c_str();}
        private:
            std::string msg_;
    };
}

#endif //MYRPC_RPC_EXCEPTION_H
