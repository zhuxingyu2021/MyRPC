#ifndef MYRPC_RINGBUFFER_H
#define MYRPC_RINGBUFFER_H

#include <memory>
#include <string>
#include <unistd.h>

#include "noncopyable.h"
#include "debug.h"
#include "net/socket.h"

/**
 * @note RingBuffer对象必须在栈上创建
 */

#define MYRPC_RINGBUFFER_SIZE 1024

namespace MyRPC {
    class ReadRingBuffer : public NonCopyable {
    public:
        using ptr = std::shared_ptr<ReadRingBuffer>;

        ReadRingBuffer(Socket::ptr& sock, useconds_t timeout = 0):m_sock(sock), m_timeout(timeout){}

        /**
             * @brief 获取读指针的当前位置
             */
        int GetPos() const { return m_read_idx; }

        bool SetPos(int pos){
            if(pos >= m_read_commit_idx && pos <= m_tail_idx){
                m_read_idx = pos;
                return true;
            }
            return false;
        }

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

        /**
         * @brief 获得从当前读指针到字符c之前的所有字符，并移动读指针
         */
        template<char ...c>
        std::string ReadUntil() {
            int prev_read_idx = m_read_idx;
            while (true) {
                try {
                    char t = GetChar();
                    if (((c == t) || ...)) {
                        break;
                    }
                }catch(const NetException& e){
                    // 发生异常，则恢复read指针
                    m_read_idx = prev_read_idx;
                    throw e;
                }
            }
            int actual_beg_pos = prev_read_idx % MYRPC_RINGBUFFER_SIZE;
            int actual_end_pos = m_read_idx % MYRPC_RINGBUFFER_SIZE;

            if(actual_beg_pos < actual_end_pos){
                return std::string(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos);
            }else{
                return std::string(&m_array[actual_beg_pos], MYRPC_RINGBUFFER_SIZE - actual_beg_pos) +
                       std::string(m_array, actual_end_pos);
            }
        }

        void Commit(){
            m_read_commit_idx = m_read_idx;
        }

    private:
        Socket::weak_ptr m_sock;
        useconds_t m_timeout = 0;

        char m_array[MYRPC_RINGBUFFER_SIZE];

        unsigned long m_read_idx = 0;
        unsigned long m_read_commit_idx = 0;
        unsigned long m_tail_idx = 0;

        void _read_socket();
    };

    class WriteRingBuffer {
    public:
        using ptr = std::shared_ptr<WriteRingBuffer>;

        WriteRingBuffer(Socket::ptr& sock, useconds_t timeout = 0):m_sock(sock), m_timeout(timeout){}

        /**
         * @brief 向字符缓冲区中添加字符串数据
         * @param str 要添加的字符串
         */
        void Append(const std::string &str){
            int len = str.length();
            unsigned long limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
            unsigned long write_end_idx = m_write_idx + len;
            while(write_end_idx > limit){ // 缓冲区满
                _write_socket();
                limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
            }

            int actual_beg_pos = m_write_idx % MYRPC_RINGBUFFER_SIZE;
            int actual_end_pos = write_end_idx % MYRPC_RINGBUFFER_SIZE;
            if(actual_beg_pos < actual_end_pos){
                memcpy(&m_array[actual_beg_pos], str.c_str(), actual_end_pos - actual_beg_pos);
            }else{
                memcpy(&m_array[actual_beg_pos], str.c_str(), MYRPC_RINGBUFFER_SIZE - actual_beg_pos);
                memcpy(m_array, str.c_str() + MYRPC_RINGBUFFER_SIZE - actual_beg_pos, actual_end_pos);
            }

            m_write_idx = write_end_idx;
        }

        /**
         * @brief 向字符缓冲区中添加字符数据
         * @param str 要添加的字符串
         */
        void Append(unsigned char c){
            unsigned long limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
            while(m_write_idx == limit){// 缓冲区满
                _write_socket();
                limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
            }
            m_array[(m_write_idx++) % MYRPC_RINGBUFFER_SIZE] = c;
        }

        /**
         * @brief 写入指针回退size个字符
         */
        void Backward(size_t size){
            m_write_idx -= size;
            MYRPC_ASSERT(m_write_idx >= m_write_commit_idx);
        }

        void Commit(){
            m_write_commit_idx = m_write_idx;
        }

        void Flush(){
            Commit();
            _write_socket();
        }

    private:
        Socket::weak_ptr m_sock;
        useconds_t m_timeout = 0;

        char m_array[MYRPC_RINGBUFFER_SIZE];

        unsigned long m_write_idx = 0;
        unsigned long m_write_commit_idx = 0;
        unsigned long m_front_idx = 0;

        void _write_socket();
    };

}

#endif //MYRPC_RINGBUFFER_H
