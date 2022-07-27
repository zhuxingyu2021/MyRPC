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
    int size = str.size(); // 未写入的字符串大小
    const char* data = str.data(); // 未写入的字符串数据地址

    int new_offset = write_offset + size; // 新的写指针偏移量（相对于链表表头）
    total_size += size;
    while(new_offset > buf_sz){
        auto sz_to_write = buf_sz - write_offset; // 本次写入的长度
        memcpy(write_pos->data + write_offset, data, sz_to_write);
        data = data + sz_to_write;

        size -= sz_to_write;
        new_offset -= buf_sz;

        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = write_pos;
        write_pos->next = new_node;
        write_pos = new_node;
        write_offset = 0;
    }
    memcpy(write_pos->data + write_offset, data, size);
    write_offset = new_offset;
    return *this;
}

StringBuffer &StringBuffer::operator<<(char c) {
    if(write_offset == buf_sz){
        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = write_pos;
        write_pos->next = new_node;
        write_pos = new_node;
        write_offset = 0;
    }
    write_pos->data[write_offset++] = c;
    return *this;
}

void StringBuffer::Write(const char* data, size_t size) {
    int new_offset = write_offset + size; // 新的写指针偏移量（相对于链表表头）
    total_size += size;
    while(new_offset > buf_sz){
        auto sz_to_write = buf_sz - write_offset; // 本次写入的长度
        memcpy(write_pos->data + write_offset, data, sz_to_write);
        data = data + sz_to_write;

        size -= sz_to_write;
        new_offset -= buf_sz;

        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = write_pos;
        write_pos->next = new_node;
        write_pos = new_node;
        write_offset = 0;
    }
    memcpy(write_pos->data + write_offset, data, size);
    write_offset = new_offset;
}

void StringBuffer::Backward(size_t size) {
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
    Node* cur = head->next;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete[] cur->data;
        delete cur;
        cur = next;
    }
    head->next = nullptr;
    head->prev = nullptr;
    write_pos = head;
    write_offset = 0;
    total_size = 0;
}

void StringBuffer::StringBufferReader::Reset() {
    read_pos = buffer.head;
    read_offset = 0;
    total_read_size = 0;
}

void StringBuffer::StringBufferReader::Backward(size_t size) {
    // TODO: 但是不急
    Logger::error("No Implementation! In file: {}, line:{}", __FILE__, __LINE__);
}

void StringBuffer::StringBufferReader::Foward(size_t size) {
    if(total_read_size + size <= buffer.total_size){
        total_read_size += size;
        read_offset += size;
        if(read_offset >= buffer.buf_sz){
            read_pos = read_pos->next;
            read_offset -= buffer.buf_sz;
        }
    }
}

char StringBuffer::StringBufferReader::GetChar(){
    if(total_read_size < buffer.total_size) {
        char c = read_pos->data[read_offset];
        total_read_size++;
        read_offset++;
        if(read_offset == buffer.buf_sz){
            read_pos = read_pos->next;
            read_offset = 0;
        }
        return c;
    }
    return '\0';
}

char StringBuffer::StringBufferReader::PeekChar() const {
    if(total_read_size < buffer.total_size) {
        return read_pos->data[read_offset];
    }
    return '\0';
}

std::string StringBuffer::StringBufferReader::PeekString(size_t size) const {
    std::string str;
    if(total_read_size + size <= buffer.total_size){
        str.reserve(32);
        size_t offset = read_offset;
        size_t read_cnt = 0;
        auto pos = read_pos;
        while(read_cnt < size){
            str.push_back(pos->data[offset]);
            read_cnt++;
            offset++;
            if(offset == buffer.buf_sz){
                pos = pos->next;
                offset = 0;
            }
        }
    }
    return str;
}


void StringBuffer::WriteFile(int fd) {
    Node* cur = head;
    auto written = 0;
    while (cur != write_pos) {
        while(written < buf_sz) {
            written += write(fd, cur->data, buf_sz);
        }
        cur = cur->next;
    }
    written = 0;
    while(written < write_offset) {
        written += write(fd, cur->data, write_offset);
    }
    written = 0;
    while(written < 1){
        written += write(fd, "\0", 1);
    }
}
