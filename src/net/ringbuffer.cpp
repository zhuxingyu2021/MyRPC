#include "net/ringbuffer.h"
#include "net/exception.h"

using namespace MyRPC;

void ReadRingBuffer::_read_socket() {
    // 写入tail -> read_commit之间的缓冲区
    int recv_cnt = 0;
    int actual_beg_pos = m_tail_idx % MYRPC_RINGBUFFER_SIZE;
    int actual_end_pos = (m_read_commit_idx + MYRPC_RINGBUFFER_SIZE - 1) % MYRPC_RINGBUFFER_SIZE;

    if(actual_beg_pos == actual_end_pos){
        // RingBuffer缓冲区满
        throw NetException("read socket to ringbuffer", NetException::BUFFER);
    }else if(actual_beg_pos < actual_end_pos){ // 在ringbuffer中添加新数据
        recv_cnt = m_sock->RecvTimeout(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos, 0, m_timeout);
    }else{ // 在ringbuffer中添加新数据
        iovec _iov[2];
        _iov[0].iov_base = &m_array[actual_beg_pos];
        _iov[0].iov_len = MYRPC_RINGBUFFER_SIZE - actual_beg_pos;
        _iov[1].iov_base = m_array;
        _iov[1].iov_len = actual_end_pos;
        recv_cnt = m_sock->ReadvTimeout(_iov, 2, m_timeout);
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
        send_cnt = m_sock->SendTimeout(&m_array[actual_beg_pos], actual_end_pos - actual_beg_pos, 0, m_timeout);
    }else{
        iovec _iov[2];
        _iov[0].iov_base = &m_array[actual_beg_pos];
        _iov[0].iov_len = MYRPC_RINGBUFFER_SIZE - actual_beg_pos;
        _iov[1].iov_base = m_array;
        _iov[1].iov_len = actual_end_pos;
        send_cnt = m_sock->WritevTimeout(_iov, 2, m_timeout);
    }
    m_front_idx += send_cnt;
}
