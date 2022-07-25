#ifndef MYRPC_STRINGBUFFER_H
#define MYRPC_STRINGBUFFER_H

#include <string_view>

namespace MyRPC{
    class StringBuffer{
    private:
        struct Node{
            char* data;
            Node* next;
            Node* prev;
        };
        Node* head = nullptr;
        Node* pos = nullptr;
    public:
        StringBuffer(size_t buffer_size = 4096);
        ~StringBuffer();

        void Write(std::string_view str);
        void Write(const char* data, size_t size);

        void Rollback(size_t size);
        void Clear();

        void WriteFile(int fd);

    private:
        const size_t buf_sz;
        int offset = 0;
    };
}

#endif //MYRPC_STRINGBUFFER_H
