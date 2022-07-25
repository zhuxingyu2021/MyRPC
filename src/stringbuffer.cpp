#include "stringbuffer.h"
#include <unistd.h>
#include <cstring>

using namespace MyRPC;

StringBuffer::StringBuffer(size_t buffer_size):buf_sz(buffer_size){
    head = new Node;
    head->data = new char[buffer_size];
    head->next = nullptr;
    head->prev = nullptr;
    pos = head;
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

void StringBuffer::Write(std::string_view str) {
    int append_sz = str.size();
    int new_offset = offset + append_sz + 1;
    if(new_offset <= buf_sz){
        memcpy(pos->data + offset, str.data(), append_sz);
        offset = new_offset;
    }else{
        memcpy(pos->data, str.data(), buf_sz - offset);
        const char* str_data_offset = str.data() + buf_sz - offset; // 剩余字节的起始地址
        append_sz = append_sz - buf_sz + offset; // 剩余字节的长度
        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = pos;
        pos->next = new_node;
        pos = new_node;
        memcpy(pos->data, str_data_offset, append_sz);
        offset = append_sz;
    }
}

void StringBuffer::Write(const char* data, size_t size) {
    int new_offset = offset + size + 1;
    if(new_offset <= buf_sz){
        memcpy(pos->data + offset, data, size);
        offset = new_offset;
    }else{
        memcpy(pos->data, data, buf_sz - offset);
        const char* str_data_offset = data + buf_sz - offset; // 剩余字节的起始地址
        size = size - buf_sz + offset; // 剩余字节的长度
        // 分配新的缓冲区
        Node* new_node = new Node;
        new_node->data = new char[buf_sz];
        new_node->next = nullptr;
        new_node->prev = pos;
        pos->next = new_node;
        pos = new_node;
        memcpy(pos->data, str_data_offset, size);
        offset = size;
    }
}

void StringBuffer::Rollback(size_t size) {
    offset -= size;
    if(offset < 0){
        delete[] pos->data;
        pos = pos->prev;
        delete pos;
        if(pos==nullptr){
            offset = 0;
        }else{
            offset += buf_sz;
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
    pos = head;
    offset = 0;
}

void StringBuffer::WriteFile(int fd) {
    Node* cur = head;
    while (cur != nullptr) {
        write(fd, cur->data, buf_sz);
        cur = cur->next;
    }
}
