#ifndef MYRPC_STRINGBUFFER_H
#define MYRPC_STRINGBUFFER_H

#include <string_view>
#include <string>
#include <memory>
#include <deque>
#include <variant>

#include "noncopyable.h"

namespace MyRPC{
    // 字符串缓冲区，由StringBuilder生成
    class StringBuffer: public NonCopyable{
    public:
        unsigned char* data = nullptr;
        size_t size = 0;
        unsigned long capacity;

        StringBuffer():capacity(0){}

        StringBuffer(size_t cap):capacity(cap){data = new unsigned char[cap];}
        StringBuffer(StringBuffer&& sb) noexcept:capacity(sb.capacity) {
            delete data;
            data = sb.data;
            size = sb.size;
            m_read_offset = sb.m_read_offset;
            sb.data = nullptr; sb.size = 0;
        }
        StringBuffer& operator=(StringBuffer&& sb) noexcept{
            capacity = sb.capacity;
            data = sb.data;
            size = sb.size;
            m_read_offset = sb.m_read_offset;
            sb.data = nullptr; sb.size = 0;
            return *this;
        }

        ~StringBuffer(){delete data;}

        /**
         * @brief 获取读指针的当前位置
         */
        int GetPos() const {return m_read_offset;}

        /**
         * @brief 获得下一个字符，并将读指针向后移动一个字符
         */
        char GetChar();

        /**
         * @brief 读指针前进sz个字符
         */
        void Forward(size_t sz);

        /**
         * @brief 读指针回退sz个字符
         */
        void Backward(size_t sz);

        /**
         * @brief 获得下一个字符，但不移动读指针
         */
        unsigned char PeekChar() const;

        /**
        * @brief 获得之后的N个字符，但不移动读指针
        */
        std::string PeekString(size_t N) const;

        /**
         * @brief 获得从当前读指针到字符c之前的所有字符，并移动读指针
         */
        template<char ...c>
        std::string ReadUntil() {
            auto start_pos = m_read_offset;
            while (m_read_offset < size) {
                char t = data[m_read_offset];
                if (((c == t) || ...)) {
                    break;
                }
                m_read_offset++;
            }
            std::string str((char*)(data + start_pos), m_read_offset - start_pos);
            return str;
        }

        std::string ToString() const{
            return std::string((char*)data, size);
        }

        void Commit(){ /*Do Nothing*/ }

    private:
        int m_read_offset = 0; // 已读取的字符数量
    };

    class StringBuilder : public NonCopyable{
    public:
        /**
         * @brief 向StringBuilder中添加字符串缓冲区
         * @param sb 要添加的字符串缓冲区
         */
        void Append(StringBuffer &&sb);

        /**
         * @brief 向字符缓冲区中添加字符串数据
         * @param str 要添加的字符串
         */
        void Append(std::string &&str);

        /**
         * @brief 向字符缓冲区中添加字符串数据
         * @param str 要添加的字符串
         */
        void Append(const std::string &str);

        /**
         * @brief 向字符缓冲区中添加字符数据
         * @param str 要添加的字符串
         */
        void Append(unsigned char c);

        /**
         * @brief 写入指针回退size个字符
         */
        void Backward(size_t size);

        /**
         * @brief 合并StringBuilder
         */
        StringBuffer Concat();

        /**
         * @brief 清除字符缓冲区
         */
        void Clear();

        void Commit(){ /*Do Nothing*/ }
        void Flush(){ /*Do Nothing*/ }

    private:
        int m_total_size = 0; // 字符缓冲区总大小
        int m_max_backward = 0;

        struct _backward_type {
            size_t size;
        };
        std::deque<std::variant<std::string, StringBuffer, char, _backward_type>> build_buf;
    };

};

#endif //MYRPC_STRINGBUFFER_H
