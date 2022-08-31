#ifndef MYRPC_RPC_EXCEPTION_H
#define MYRPC_RPC_EXCEPTION_H

#include <exception>
#include <string>
#include <cstring>

#include "enum2string.h"

namespace MyRPC{
class FileException:public std::exception{
public:
    FileException(const std::string& msg):msg_(msg){}
    const char* what() const noexcept override{return msg_.c_str();}
private:
    std::string msg_;
};

class RPCClientException:public std::exception{
public:
    MYRPC_DEFINE_ENUM_WITH_STRING_CONVERSIONS(ErrorType,
                                              (NO_ERROR)
                                              (HAVENT_BEEN_CALLED)
                                              (SERVICE_NOT_FOUND)
                                              (REGISTRY_SERVER_CLOSED)
                                              (CONNECT_TIME_OUT)
                                              (SERVER_EXCEPTION)
    )

    RPCClientException(ErrorType err):err_(err), msg_("Error Type: " + ToString(err)){}
    RPCClientException(ErrorType err, std::string&& server_what):err_(SERVER_EXCEPTION){
        MYRPC_ASSERT(err == SERVER_EXCEPTION);

        except_server_ = std::forward<std::string>(server_what);
        msg_ = "Error Type: SERVER_EXCEPTION, what: " + except_server_;
    }

    const char* what() const noexcept override{return msg_.c_str();}
    ErrorType GetErrorType() const{return err_;}
    std::string GetServerExceptionWhat() const{return except_server_.c_str();}

private:
    ErrorType err_;
    std::string msg_;

    std::string except_server_;
};
}

#endif //MYRPC_RPC_EXCEPTION_H
