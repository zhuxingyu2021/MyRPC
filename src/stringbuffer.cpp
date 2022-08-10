#include "stringbuffer.h"
#include <unistd.h>
#include <cstring>
#include "logger.h"

using namespace MyRPC;

void StringBuilder::Append(StringBuffer &&sb) {
    m_total_size += sb.size;
    build_buf.emplace_back(std::forward<StringBuffer>(sb));
}

void StringBuilder::Append(char c) {
    ++m_total_size;
    build_buf.emplace_back(c);
}

void StringBuilder::Append(std::string&& str) {
    m_total_size += str.size();
    build_buf.emplace_back(std::forward<std::string>(str));
}

void StringBuilder::Append(const std::string& str) {
    m_total_size += str.size();
    build_buf.emplace_back(str);
}

void StringBuilder::Backward(size_t size) {
    m_total_size -= size;
    _backward_type b = {size};
    build_buf.emplace_back(b);

    if(size > m_max_backward) m_max_backward = size;
}

StringBuffer StringBuilder::Concat() {
    StringBuffer buf(m_total_size + m_max_backward);
    buf.size = m_total_size;

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
    m_total_size = 0;
    build_buf.clear();
}


void StringBuffer::Backward(size_t sz) {
    if(m_read_offset - sz >= 0) m_read_offset -= sz;
}

void StringBuffer::Forward(size_t sz) {
    if(m_read_offset + sz <= size) m_read_offset += sz;
}

char StringBuffer::GetChar(){
    if(m_read_offset < size) {
        char c = data[m_read_offset++];
        return c;
    }
    return '\0';
}

char StringBuffer::PeekChar() const {
    if(m_read_offset < size) {
        return data[m_read_offset];
    }
    return '\0';
}

std::string StringBuffer::PeekString(size_t N) const {
    if(m_read_offset + N <= size){
        std::string str(data + m_read_offset, N);
        return str;
    }
    std::string str;
    return str;
}
