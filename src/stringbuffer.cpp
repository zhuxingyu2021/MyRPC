#include "stringbuffer.h"
#include <unistd.h>
#include <cstring>
#include "logger.h"

using namespace MyRPC;

StringBuffer::StringBuffer(size_t buffer_size):buf_sz(buffer_size){
    head = new Node;
    head->data = new char[buffer_size];
    head->next = nullptr;
    head->prev = nullptr;
    write_pos = head;

    read_pos = head;
}

StringBuffer::~StringBuffer() {
    Node* cur = head;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete[] cur->data;
        delete cur;
        cur = next;
    }
}

StringBuffer &StringBuffer::operator<<(std::string_view str) {
    int append_sz = str.size();
    int new_offset = write_offset + append_sz;
    total_size += append_sz;
    if(new_offset <= buf_sz){
        memcpy(write_pos->data + write_offset, str.data(), append_sz);
        write_offset = new_offset;
    }else{
        memcpy(write_pos->data, str.data(), buf_sz - write_offset);
        const char* str_data_offset = str.data() + buf_sz - write_offset; // 剩余字节的起始地址
        append_sz = append_sz - buf_sz + write_offset; // 剩余字节的长度
        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = write_pos;
        write_pos->next = new_node;
        write_pos = new_node;
        memcpy(write_pos->data, str_data_offset, append_sz);
        write_offset = append_sz;
    }
    return *this;
}

void StringBuffer::Write(const char* data, size_t size) {
    int new_offset = write_offset + size;
    if(new_offset <= buf_sz){
        memcpy(write_pos->data + write_offset, data, size);
        write_offset = new_offset;
    }else{
        memcpy(write_pos->data, data, buf_sz - write_offset);
        const char* str_data_offset = data + buf_sz - write_offset; // 剩余字节的起始地址
        size = size - buf_sz + write_offset; // 剩余字节的长度
        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = write_pos;
        write_pos->next = new_node;
        write_pos = new_node;
        memcpy(write_pos->data, str_data_offset, size);
        write_offset = size;
    }
    total_size += size;
}

void StringBuffer::RollbackWritePointer(size_t size) {
    write_offset -= size;
    total_size -= size;
    if(write_offset < 0){
        delete[] write_pos->data;
        write_pos = write_pos->prev;
        delete write_pos;
        if(write_pos == nullptr){
            write_offset = 0;
        }else{
            write_offset += buf_sz;
        }
    }
}

void StringBuffer::Clear() {
    Node* cur = head;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete[] cur->data;
        delete cur;
        cur = next;
    }
    head = new Node;
    head->data = new char[buf_sz];
    head->next = nullptr;
    head->prev = nullptr;
    write_pos = head;
    write_offset = 0;
    total_size = 0;
}

void StringBuffer::RollbackReadPointer(size_t size) {
    Logger::error("No Implementation! In file: {}, line:{}", __FILE__, __LINE__);
}

void StringBuffer::ForwardReadPointer(size_t size) {
    if(total_read_size + size <= total_size){
        total_read_size += size;
        read_offset += size;
        if(read_offset >= buf_sz){
            read_pos = read_pos->next;
            read_offset -= buf_sz;
        }
    }
}

char StringBuffer::GetChar(){
    if(total_read_size < total_size) {
        char c = read_pos->data[read_offset];
        total_read_size++;
        read_offset++;
        if(read_offset == buf_sz){
            read_pos = read_pos->next;
            read_offset = 0;
        }
        return c;
    }
    return '\0';
}

char StringBuffer::PeekChar() const {
    if(total_read_size < total_size) {
        return read_pos->data[read_offset];
    }
    return '\0';
}

std::string StringBuffer::PeekString(size_t size) const {
    std::string str;
    if(total_read_size + size <= total_size){
        str.reserve(20);
        size_t offset = read_offset;
        size_t read_cnt = 0;
        auto pos = read_pos;
        while(read_cnt < size){
            str.push_back(pos->data[offset]);
            read_cnt++;
            offset++;
            if(offset == buf_sz){
                pos = pos->next;
                offset = 0;
            }
        }
    }
    return str;
}


void StringBuffer::WriteFile(int fd) {
    Node* cur = head;
    while (cur != write_pos) {
        write(fd, cur->data, buf_sz);
        cur = cur->next;
    }
    write(fd, cur->data, write_offset);
    write(fd, "\n", 1);
}
