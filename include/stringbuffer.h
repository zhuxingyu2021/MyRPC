#ifndef MYRPC_STRINGBUFFER_H
#define MYRPC_STRINGBUFFER_H

#include <string_view>
#include <string>
#include <memory>

namespace MyRPC{

    class StringBuffer{
    private:
        // 用双向链表存储字符数据
        struct Node{
            char* data;
            Node* next;
            Node* prev;
        };
        Node* head = nullptr;
    public:
        using ptr = std::shared_ptr<StringBuffer>;

        /**
         * @param buffer_size[in] 字符缓冲区链表每个节点的大小
         */
        StringBuffer(size_t buffer_size = 4096);
        StringBuffer(const StringBuffer&) = delete;
        StringBuffer(StringBuffer&&) = delete;
        ~StringBuffer();

        /**
         * @brief 向字符缓冲区中添加字符数组
         * @param data 字符数组区域
         * @param size 字符数组区域的大小
         */
        void Write(const char* data, size_t size);

        /**
         * @brief 向字符缓冲区中添加字符串数据
         * @param str 要添加的字符串
         */
        StringBuffer& operator<<(std::string_view str);

        /**
         * @brief 向字符缓冲区中添加字符数据
         * @param str 要添加的字符串
         */
        StringBuffer& operator<<(char c);

        /**
         * @brief 写入指针回退size个字符
         */
        void Backward(size_t size);

        /**
         * @brief 清除字符缓冲区
         */
        void Clear();

        /**
         * @brief 将字符缓冲区写到文件
         * @param fd 文件描述符
         */
        void WriteFile(int fd);

    private:
        int total_size = 0; // 字符缓冲区总大小

        const size_t buf_sz; // 字符缓冲区链表每个节点的大小
        Node* write_pos; // 当前链表节点（写入）
        int write_offset = 0; // 当前字符缓冲区链表节点的偏移量（写入）


    public:
        class StringBufferReader {
        public:
            StringBufferReader(const StringBuffer& buffer) : buffer(buffer),read_pos(buffer.head) {}

            /**
             * @brief 获得下一个字符，并将读指针向后移动一个字符
             */
            char GetChar();

            /**
             * @brief 读指针前进size个字符
             */
            void Foward(size_t size);

            /**
             * @brief 读指针回退size个字符
             */
            void Backward(size_t size);

            /**
             * @brief 获得下一个字符，但不移动读指针
             */
            char PeekChar() const;

            /**
            * @brief 获得之后的N个字符，但不移动读指针
            */
            std::string PeekString(size_t N) const;

            /**
             * @brief 获得从当前读指针到字符c之前的所有字符，并移动读指针
             */
            template<char ...c>
            std::string ReadUntil() {
                std::string str;
                str.reserve(32);
                while (total_read_size < buffer.total_size) {
                    char t = read_pos->data[read_offset];
                    if (((c == t) || ...)) {
                        break;
                    }
                    total_read_size++;
                    read_offset++;
                    if (read_offset == buffer.buf_sz) {
                        read_pos = read_pos->next;
                        read_offset = 0;
                    }
                    str.push_back(t);
                }
                return str;
            }

            /**
             * @brief 重置读指针
             */
            void Reset();

        private:
            int total_read_size = 0; // 已读取的字符数量

            Node* read_pos; // 当前链表节点（读取）
            int read_offset = 0; // 当前字符缓冲区链表节点的偏移量（读取）

            const StringBuffer& buffer;
        };

        StringBufferReader GetStringBufferReader() const{return StringBufferReader(*this);}
    };
}

#endif //MYRPC_STRINGBUFFER_H
