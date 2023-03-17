#ifndef MYRPC_SYNC_QUEUE_H
#define MYRPC_SYNC_QUEUE_H

#include "fiber_sync.h"
#include <queue>

namespace MyRPC {
    template<class T>
    class SyncQueue {
    public:
        template<class Arg>
        void Push(Arg&& val){
            m_queue_mutex.lock();
            m_queue.push(std::forward<Arg>(val));
            m_empty.notify_one();
            m_queue_mutex.unlock();
        }

        template<class ...Args>
        void Emplace(Args&& ...args){
            m_queue_mutex.lock();
            m_queue.emplace(std::forward<Args>(args)...);
            m_empty.notify_one();
            m_queue_mutex.unlock();
        }

        T Pop(){
            m_queue_mutex.lock();
            while(m_queue.empty()){
                m_empty.wait(m_queue_mutex);
            }
            T ret = m_queue.front();
            m_queue.pop();
            m_queue_mutex.unlock();
            return ret;
        }

        void Close(){
            m_empty.notify_all();
        }

        void Clear(){
            m_queue_mutex.Clear();
            m_empty.Clear();
        }

    private:
        std::queue<T> m_queue;
        FiberSync::Mutex m_queue_mutex;
        FiberSync::ConditionVariable m_empty;
    };
}

#endif //MYRPC_SYNC_QUEUE_H
