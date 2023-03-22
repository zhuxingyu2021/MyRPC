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

class RPCClientException:public std::exception{
public:
    enum ErrorType{
        NO_ERROR,
        HAVENT_BEEN_CALLED,
        SERVICE_NOT_FOUND,
        REGISTRY_SERVER_CLOSED,
        CONNECT_TIME_OUT,
        SERVER_EXCEPTION,
        SERVER_CLOSED
    };

    inline const char* ToString(ErrorType e){
        switch(e){
            case NO_ERROR: return "NO_ERROR";
            case HAVENT_BEEN_CALLED: return "HAVENT_BEEN_CALLED";
            case SERVICE_NOT_FOUND: return "SERVICE_NOT_FOUND";
            case REGISTRY_SERVER_CLOSED: return "REGISTRY_SERVER_CLOSED";
            case CONNECT_TIME_OUT: return "CONNECT_TIME_OUT";
            case SERVER_EXCEPTION: return "SERVER_EXCEPTION";
            case SERVER_CLOSED: return "SERVER_CLOSED";
        }
    }

    RPCClientException(ErrorType err):err_(err), msg_(std::string("Error Type: ") + ToString(err)){}
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
