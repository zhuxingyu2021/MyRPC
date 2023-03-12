#ifndef MYRPC_RINGBUFFER_H
#define MYRPC_RINGBUFFER_H

#include <memory>
#include <string>
#include <unistd.h>

#include "debug.h"
#include "net/socket.h"

#include "buffer/buffer.h"

/**
 * @note RingBuffer对象必须在栈上创建
 */

#define MYRPC_RINGBUFFER_SIZE 1024

namespace MyRPC {
    class ReadRingBuffer : public ReadBuffer {
    public:
        using ptr = std::shared_ptr<ReadRingBuffer>;

        ReadRingBuffer(Socket::ptr& sock, ms_t timeout = 0):m_sock(sock), m_timeout(timeout){}

        bool SetPos(int pos){
            if(pos >= m_read_commit_idx && pos <= m_tail_idx){
                m_read_idx = pos;
                return true;
            }
            return false;
        }

        virtual unsigned char GetChar() override;

        virtual void Forward(size_t sz) override;

        virtual void Backward(size_t sz) override;

        virtual unsigned char PeekChar() override;

        virtual void PeekString(std::string& s, size_t N) override;

        virtual void Commit() override;

    protected:

        virtual void _read_to_str(std::string &s, unsigned long begin, unsigned long end) override ;

    private:
        Socket::weak_ptr m_sock;
        ms_t m_timeout = 0;

        char m_array[MYRPC_RINGBUFFER_SIZE];

        unsigned long m_read_idx = 0;
        unsigned long m_read_commit_idx = 0;
        unsigned long m_tail_idx = 0;

        void _read_socket();
    };

    class WriteRingBuffer: public WriteBuffer{
    public:
        using ptr = std::shared_ptr<WriteRingBuffer>;

        WriteRingBuffer(Socket::ptr& sock, ms_t timeout = 0):m_sock(sock), m_timeout(timeout){}

        virtual void Append(const std::string &str) override;

        virtual void Append(unsigned char c) override;

        virtual void Backward(size_t size) override;

        virtual void Commit() override;

        void Flush(){
            Commit();
            _write_socket();
        }

    private:
        Socket::weak_ptr m_sock;
        ms_t m_timeout = 0;

        char m_array[MYRPC_RINGBUFFER_SIZE];

        unsigned long m_write_idx = 0;
        unsigned long m_write_commit_idx = 0;
        unsigned long m_front_idx = 0;

        void _write_socket();
    };

}

#endif //MYRPC_RINGBUFFER_H
