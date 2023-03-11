#ifndef MYRPC_RINGBUFFER_H
#define MYRPC_RINGBUFFER_H

#include <memory>
#include <string>
#include <unistd.h>

#include "noncopyable.h"
#include "macro.h"
#include "net/socket.h"

/**
 * @note RingBuffer对象必须在栈上创建
 */

#define MYRPC_RINGBUFFER_SIZE 1024

namespace MyRPC {
    class ReadRingBuffer : public NonCopyable {
    public:
        using ptr = std::shared_ptr<ReadRingBuffer>;

        ReadRingBuffer(Socket::ptr sock, useconds_t timeout = 0):m_sock(sock), m_timeout(timeout){}

        /**
             * @brief 获取读指针的当前位置
             */
        int GetPos() const { return m_read_idx; }

        /**
         * @brief 获得下一个字符，并将读指针向后移动一个字符
         */
        char GetChar() {
            while((m_read_idx % MYRPC_RINGBUFFER_SIZE) == (m_tail_idx % MYRPC_RINGBUFFER_SIZE)){ // EMPTY
                _read_socket();
            }
            return m_array[(m_read_idx++) % MYRPC_RINGBUFFER_SIZE];
        }

        /**
         * @brief 读指针前进sz个字符
         */
        void Forward(size_t sz){
            while(m_read_idx + sz > m_tail_idx){ // 缓冲区中的元素不够读
                _read_socket();
            }
            m_read_idx += sz;
        }

        /**
         * @brief 读指针回退sz个字符
         */
        void Backward(size_t sz){
            m_read_idx -= sz;
            MYRPC_ASSERT(m_read_idx >= m_read_commit_idx);
        }

        /**
         * @brief 获得下一个字符，但不移动读指针
         */
        unsigned char PeekChar(){
            while((m_read_idx % MYRPC_RINGBUFFER_SIZE) == (m_tail_idx % MYRPC_RINGBUFFER_SIZE)){ // EMPTY
                _read_socket();
            }
            return m_array[m_read_idx % MYRPC_RINGBUFFER_SIZE];
        }

        /**
        * @brief 获得之后的N个字符，但不移动读指针
        */
        std::string PeekString(size_t N){
            while(m_read_idx + N > m_tail_idx){ // 缓冲区中的元素不够读
                _read_socket();
            }
            int actual_beg_pos = m_read_idx % MYRPC_RINGBUFFER_SIZE;
            int actual_end_pos = (m_read_idx + N) % MYRPC_RINGBUFFER_SIZE;
            if(actual_beg_pos < actual_end_pos){
                return std::string(&m_array[actual_beg_pos], N);
            }else{
                return std::string(&m_array[actual_beg_pos], MYRPC_RINGBUFFER_SIZE - actual_beg_pos) +
                    std::string(m_array, actual_end_pos);
            }
        }

    private:
        Socket::ptr m_sock;

        char m_array[MYRPC_RINGBUFFER_SIZE];

        unsigned long m_read_idx = 0;
        unsigned long m_read_commit_idx = 0;
        unsigned long m_tail_idx = 0;

        useconds_t m_timeout = 0;

        void _read_socket();
    };

    class WriteRingBuffer {
    public:
    };

}

#endif //MYRPC_RINGBUFFER_H
