#include "buffer/ringbuffer.h"
#include "net/exception.h"

using namespace MyRPC;

unsigned char ReadRingBuffer::GetChar() {
    while((m_read_idx % MYRPC_RINGBUFFER_SIZE) == (m_tail_idx % MYRPC_RINGBUFFER_SIZE)){ // EMPTY
        _read_socket();
    }
    return m_array[(m_read_idx++) % MYRPC_RINGBUFFER_SIZE];
}

void ReadRingBuffer::Forward(size_t sz) {
    while(m_read_idx + sz > m_tail_idx){ // 缓冲区中的元素不够读
        _read_socket();
    }
    m_read_idx += sz;
}

void ReadRingBuffer::Backward(size_t sz) {
    m_read_idx -= sz;
    MYRPC_ASSERT(m_read_idx >= m_read_commit_idx);
}

unsigned char ReadRingBuffer::PeekChar() {
    while((m_read_idx % MYRPC_RINGBUFFER_SIZE) == (m_tail_idx % MYRPC_RINGBUFFER_SIZE)){ // EMPTY
        _read_socket();
    }
    return m_array[m_read_idx % MYRPC_RINGBUFFER_SIZE];
}

void ReadRingBuffer::PeekString(std::string &s, size_t N) {
    while(m_read_idx + N > m_tail_idx){ // 缓冲区中的元素不够读
        _read_socket();
    }
    int actual_beg_pos = m_read_idx % MYRPC_RINGBUFFER_SIZE;
    int actual_end_pos = (m_read_idx + N) % MYRPC_RINGBUFFER_SIZE;
    if(actual_beg_pos < actual_end_pos){
        s = std::string(&m_array[actual_beg_pos], N);
    }else{
        s = std::string(&m_array[actual_beg_pos], MYRPC_RINGBUFFER_SIZE - actual_beg_pos) +
            std::string(m_array, actual_end_pos);
    }
}

void ReadRingBuffer::Commit() {
    m_read_commit_idx = m_read_idx;
}

void ReadRingBuffer::_read_to_str(std::string &s, unsigned long begin, unsigned long end) {
    int actual_beg_pos = begin % MYRPC_RINGBUFFER_SIZE;
    int actual_end_pos = end % MYRPC_RINGBUFFER_SIZE;

    if(actual_beg_pos < actual_end_pos){
        s = std::string(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos);
    }else{
        s = std::string(&m_array[actual_beg_pos], MYRPC_RINGBUFFER_SIZE - actual_beg_pos) +
            std::string(m_array, actual_end_pos);
    }
}

void WriteRingBuffer::Append(const std::string &str) {
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

void WriteRingBuffer::Append(unsigned char c) {
    unsigned long limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
    while(m_write_idx == limit){// 缓冲区满
        _write_socket();
        limit = m_front_idx + MYRPC_RINGBUFFER_SIZE - 1;
    }
    m_array[(m_write_idx++) % MYRPC_RINGBUFFER_SIZE] = c;
}

void WriteRingBuffer::Backward(size_t size) {
    m_write_idx -= size;
    MYRPC_ASSERT(m_write_idx >= m_write_commit_idx);
}

void WriteRingBuffer::Commit() {
    m_write_commit_idx = m_write_idx;
}

void ReadRingBuffer::_read_socket() {
    // 写入tail -> read_commit之间的缓冲区
    int recv_cnt = 0;
    int actual_beg_pos = m_tail_idx % MYRPC_RINGBUFFER_SIZE;
    int actual_end_pos = (m_read_commit_idx + MYRPC_RINGBUFFER_SIZE - 1) % MYRPC_RINGBUFFER_SIZE;

    if(actual_beg_pos == actual_end_pos){
        // RingBuffer缓冲区满
        throw NetException("read socket to ringbuffer", NetException::BUFFER);
    }else if(actual_beg_pos < actual_end_pos){ // 在ringbuffer中添加新数据
        recv_cnt = m_sock.lock()->RecvTimeout(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos, 0, m_timeout);
    }else{ // 在ringbuffer中添加新数据
        iovec _iov[2];
        _iov[0].iov_base = &m_array[actual_beg_pos];
        _iov[0].iov_len = MYRPC_RINGBUFFER_SIZE - actual_beg_pos;
        _iov[1].iov_base = m_array;
        _iov[1].iov_len = actual_end_pos;
        recv_cnt = m_sock.lock()->ReadvTimeout(_iov, 2, m_timeout);
    }
    m_tail_idx += recv_cnt;
}

void WriteRingBuffer::_write_socket() {
    // 发送front -> write_commit之间的缓冲区
    int send_cnt = 0;
    int actual_beg_pos = m_front_idx % MYRPC_RINGBUFFER_SIZE;
    int actual_end_pos = m_write_commit_idx % MYRPC_RINGBUFFER_SIZE;

    if(actual_beg_pos == actual_end_pos){
        // RingBuffer缓冲区为空
        return;
    }else if(actual_beg_pos < actual_end_pos) {
        send_cnt = m_sock.lock()->SendTimeout(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos, 0, m_timeout);
    }else{
        iovec _iov[2];
        _iov[0].iov_base = &m_array[actual_beg_pos];
        _iov[0].iov_len = MYRPC_RINGBUFFER_SIZE - actual_beg_pos;
        _iov[1].iov_base = m_array;
        _iov[1].iov_len = actual_end_pos;
        send_cnt = m_sock.lock()->WritevTimeout(_iov, 2, m_timeout);
    }
    m_front_idx += send_cnt;
}
