#ifndef MYRPC_CONCURRENT_QUEUE_H
#define MYRPC_CONCURRENT_QUEUE_H

#include <atomic>
#include "arch.h"
#include "debug.h"

namespace MyRPC{
    // Multi Producer Multi Consumer lock-free queue
    template<class T, unsigned long QSIZE>
    class MPMCLockFreeQueue{
    public:
        bool TryPush(T data){
            unsigned long current_write, current_read;

            // m_write_idx原子加一
            current_write = m_write_idx;
            do{
                current_read = m_read_idx;

                if(((current_write + 1) % QSIZE) == (current_read % QSIZE)){ // FULL
                    return false;
                }
            }while(!m_write_idx.compare_exchange_weak(current_write, current_write + 1));

            // 写入data数据
            m_array[current_write % QSIZE] = data;

            // commit操作: 更新m_commit_write_idx
            while(!m_commit_write_idx.compare_exchange_weak(current_write, current_write + 1)){
                MYRPC_PAUSE;
            }

            ++m_count;
            return true;
        }

        bool TryPop(T& data){
            unsigned long current_commit_write, current_read;

            current_read = m_read_idx;
            do{
                current_commit_write = m_commit_write_idx;

                if((current_read % QSIZE) == (current_commit_write % QSIZE)){ // EMPTY
                    return false;
                }

                data = m_array[current_read % QSIZE];

                if(m_read_idx.compare_exchange_weak(current_read, current_read + 1)){
                    --m_count;
                    return true;
                }
            }while(true);

            MYRPC_ASSERT(false);
            return false;
        }

        unsigned long Size(){
            return m_count;
        }

        bool Empty() {
            return m_count == 0;
        }
    private:
        std::atomic<T> m_array[QSIZE];
        std::atomic<unsigned long> m_commit_write_idx = {0};
        std::atomic<unsigned long> m_write_idx = {0};
        std::atomic<unsigned long> m_read_idx = {0};

        std::atomic<unsigned long> m_count;
    };
}

#endif //MYRPC_CONCURRENT_QUEUE_H
