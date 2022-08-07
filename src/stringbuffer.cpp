#include "stringbuffer.h"
#include <unistd.h>
#include <cstring>
#include "logger.h"

using namespace MyRPC;

void StringBuilder::Append(StringBuffer &&sb) {
    total_size += sb.size;
    build_buf.emplace_back(std::forward<StringBuffer>(sb));
}

void StringBuilder::Append(char c) {
    ++total_size;
    build_buf.emplace_back(c);
}

void StringBuilder::Append(std::string&& str) {
    total_size += str.size();
    build_buf.emplace_back(std::forward<std::string>(str));
}

void StringBuilder::Append(const std::string& str) {
    total_size += str.size();
    build_buf.emplace_back(str);
}

void StringBuilder::Backward(size_t size) {
    total_size -= size;
    _backward_type b = {size};
    build_buf.emplace_back(b);

    if(size>max_backward) max_backward = size;
}

StringBuffer StringBuilder::Concat() {
    StringBuffer buf(total_size+max_backward);
    buf.size = total_size;

    struct _elem_visitor{
        StringBuffer* sb;
        size_t offset;

        void operator()(std::string_view sv){memcpy(sb->data+offset, sv.data(), sv.size());offset+=sv.size();}
        void operator()(const StringBuffer& sb_src){memcpy(sb->data+offset, sb_src.data, sb_src.size); offset+=sb_src.size;}
        void operator()(char c){sb->data[offset++]=c;}
        void operator()(_backward_type bk){offset-=bk.size;}
    };
    _elem_visitor v = {&buf, 0};

    for(auto& elem:build_buf){
        std::visit(v, elem);
    }
    return buf;
}

void StringBuilder::Clear() {
    total_size = 0;
    build_buf.clear();
}


void StringBuffer::Backward(size_t sz) {
    if(read_offset - sz >= 0) read_offset -= sz;
}

void StringBuffer::Forward(size_t sz) {
    if(read_offset + sz <= size) read_offset += sz;
}

char StringBuffer::GetChar(){
    if(read_offset < size) {
        char c = data[read_offset++];
        return c;
    }
    return '\0';
}

char StringBuffer::PeekChar() const {
    if(read_offset < size) {
        return data[read_offset];
    }
    return '\0';
}

std::string StringBuffer::PeekString(size_t N) const {
    if(read_offset + N <= size){
        std::string str(data+read_offset, N);
        return str;
    }
    std::string str;
    return str;
}
