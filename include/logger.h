#ifndef MYRPC_LOGGER_H
#define MYRPC_LOGGER_H

#include "spdlog/spdlog.h"

namespace MyRPC{
    class Logger{
    public:
        template<class ...Args>
        static void info(Args ...args){spdlog::info(args...);}

        template<class ...Args>
        static void debug(Args ...args){spdlog::debug(args...);}

        template<class ...Args>
        static void warn(Args ...args){spdlog::warn(args...);}

        template<class ...Args>
        static void error(Args ...args){spdlog::error(args...);}

        template<class ...Args>
        static void critical(Args ...args){spdlog::critical(args...);}
    };
}

#endif //MYRPC_LOGGER_H
