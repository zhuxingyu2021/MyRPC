#ifndef MYRPC_FD_RAII_H
#define MYRPC_FD_RAII_H

#include <unistd.h>
#include "noncopyable.h"

namespace MyRPC {
    class FdRAIIWrapper : public NonCopyable{
    public:
        FdRAIIWrapper(int fd):m_fd(fd){}

        bool Closefd() {
            if((m_fd != -1) && (!m_closed)){
                close(m_fd);
                m_closed = true;
                return true;
            }
            return false;
        }

        virtual ~FdRAIIWrapper(){
            Closefd();
        }

        inline int Getfd() const{return m_fd;}
    protected:
        int m_fd;

        bool m_closed = false;
    };

}
#endif //MYRPC_FD_RAII_H
