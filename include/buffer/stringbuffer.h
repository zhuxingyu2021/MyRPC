#ifndef MYRPC_STRINGBUFFER_H
#define MYRPC_STRINGBUFFER_H

#include <string_view>
#include <string>
#include <memory>
#include <deque>
#include <variant>

#include "buffer/buffer.h"

namespace MyRPC{
    // 字符串缓冲区，由StringBuilder生成
    class StringBuffer: public ReadBuffer{
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
            m_read_idx = sb.m_read_idx;
            sb.data = nullptr; sb.size = 0;
        }
        StringBuffer& operator=(StringBuffer&& sb) noexcept{
            capacity = sb.capacity;
            data = sb.data;
            size = sb.size;
            m_read_idx = sb.m_read_idx;
            sb.data = nullptr; sb.size = 0;
            return *this;
        }

        ~StringBuffer(){delete data;}


        virtual unsigned char GetChar() override;

        virtual void Forward(size_t sz) override;

        virtual void Backward(size_t sz) override;

        virtual unsigned char PeekChar() override;

        virtual void PeekString(std::string& s, size_t N) override;

        std::string ToString() const{
            return std::string((char*)data, size);
        }

        virtual void Commit() override{}

    protected:

        virtual void _read_to_str(std::string &s, unsigned long begin, unsigned long end) override ;

    };

    class StringBuilder : public WriteBuffer{
    public:
        /**
         * @brief 向StringBuilder中添加字符串缓冲区
         * @param sb 要添加的字符串缓冲区
         */
        void Append(StringBuffer &&sb);

        virtual void Append(const std::string &str) override;

        virtual void Append(unsigned char c) override;

        virtual void Backward(size_t size) override;

        /**
         * @brief 合并StringBuilder
         */
        StringBuffer Concat();

        /**
         * @brief 清除字符缓冲区
         */
        void Clear();

        virtual void Commit() override{}

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
