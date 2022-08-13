#ifndef MYRPC_LOGGER_H
#define MYRPC_LOGGER_H

#include "spdlog/spdlog.h"
#include "fiber/hook_sleep.h"

#include "spinlock.h"

namespace MyRPC{
    extern SpinLock _global_logger_spinlock;

    class Logger{
    public:
        template<class ...Args>
        static void info(Args ...args){
            auto tmp = enable_hook;
            enable_hook = false;
            spdlog::info(std::forward<Args>(args)...);
            enable_hook = tmp;
        }

        template<class ...Args>
        static void debug(Args ...args){
            _global_logger_spinlock.lock();
            auto tmp = enable_hook;
            enable_hook = false;
            spdlog::debug(std::forward<Args>(args)...);
            enable_hook = tmp;
            _global_logger_spinlock.unlock();
        }

        template<class ...Args>
        static void warn(Args ...args){
            auto tmp = enable_hook;
            enable_hook = false;
            spdlog::warn(std::forward<Args>(args)...);
            enable_hook = tmp;
        }

        template<class ...Args>
        static void error(Args ...args){
            auto tmp = enable_hook;
            enable_hook = false;
            spdlog::error(std::forward<Args>(args)...);
            enable_hook = tmp;
        }

        template<class ...Args>
        static void critical(Args ...args){
            auto tmp = enable_hook;
            enable_hook = false;
            spdlog::critical(std::forward<Args>(args)...);
            enable_hook = tmp;
        }
    };
}

#endif //MYRPC_LOGGER_H
