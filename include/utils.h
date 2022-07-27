#ifndef MYRPC_UTILS_H
#define MYRPC_UTILS_H

#include <type_traits>
#include <arpa/inet.h>
#include <logger.h>

namespace MyRPC{
    extern bool IS_SMALL_ENDIAN;

    template<class T,size_t size>
    using byteorder_convert_type =  typename std::enable_if_t<sizeof(std::decay_t<T>)==size,std::decay_t<T>>;

    /*
     * @brief 将一个8位变量从本地字节序转化为网络字节序
     */
    template <class T>
    inline byteorder_convert_type<T,1> hton(T t){
        return t;
    }

    /*
     * @brief 将一个16位变量从本地字节序转化为网络字节序
     */
    template <class T>
    inline byteorder_convert_type<T,2> hton(T t){
        uint16_t u = htons(*(reinterpret_cast<uint16_t*>(&t)));
        return *(reinterpret_cast<T*>(&u));
    }

    /*
     * @brief 将一个32位变量从本地字节序转化为网络字节序
     */
    template <class T>
    inline byteorder_convert_type<T,4> hton(T t){
        uint32_t u = htonl(*(reinterpret_cast<uint32_t*>(&t)));
        return *(reinterpret_cast<T*>(&u));
    }

    /*
     * @brief 将一个64位变量从本地字节序转化为网络字节序
     */
    template <class T>
    inline byteorder_convert_type<T,8> hton(T t){
        if(IS_SMALL_ENDIAN) {
            // 转化成网络字节序（大端）
            auto u = *(reinterpret_cast<uint64_t *>(&t));
            auto low = u & 0xFFFFFFFF; // t的低32位（本地字节序）
            uint64_t lown = htonl(low); // t的低32位（网络字节序）
            auto highn = htonl(u >> 32); // t的高32位（网络字节序）
            auto ret = highn | (lown << 32); // 交换高32位和低32位
            return *(reinterpret_cast<T *>(&ret));
        }
        return t;
    }

    /*
     * @brief 将一个8位变量从网络字节序转化为本地字节序
     */
    template <class T>
    inline byteorder_convert_type<T,1> ntoh(T t){
        return t;
    }

    /*
     * @brief 将一个16位变量从网络字节序转化为本地字节序
     */
    template <class T>
    inline byteorder_convert_type<T,2> ntoh(T t){
        uint16_t u = ntohs(*(reinterpret_cast<uint16_t*>(&t)));
        return *(reinterpret_cast<T*>(&u));
    }

    /*
     * @brief 将一个32位变量从网络字节序转化为本地字节序
     */
    template <class T>
    inline byteorder_convert_type<T,4> ntoh(T t){
        uint32_t u = ntohl(*(reinterpret_cast<uint32_t*>(&t)));
        return *(reinterpret_cast<T*>(&u));
    }

    /*
     * @brief 将一个64位变量从网络字节序转化为本地字节序
     */
    template <class T>
    inline byteorder_convert_type<T,8> ntoh(T t){
        if(IS_SMALL_ENDIAN) {
            // 转化成本地字节序（小端）
            auto u = *(reinterpret_cast<uint64_t *>(&t));
            auto low = u & 0xFFFFFFFF; // t的低32位（网络字节序）
            uint64_t lown = ntohl(low); // t的低32位（本地字节序）
            auto highn = ntohl(u >> 32); // t的高32位（本地字节序）
            auto ret = highn | (lown << 32); // 交换高32位和低32位
            return *(reinterpret_cast<T *>(&ret));
        }
        return t;
    }


    /**
     * 删除字符串的空白字符
     * @tparam write_space 空白字符 比如' ','\t','\n','\r'
     * @param str 字符数组
     * @param size 字符数组的大小
     * @return 删去字符串中的空白字符之后，字符串的字符数量
     */
    template <char... write_space>
    size_t strtrim(char* str, size_t size){
        size_t write_pos=0;
        for(size_t read_pos=0;read_pos<size;++read_pos){
            char c = str[read_pos];
            if(((c == write_space) || ...)){
                // skip
            }else{
                str[write_pos++] = str[read_pos];
            }
        }
        return write_pos;
    }

#undef MYRPC_ENABLE_IF

    static bool _my_util_initializer = IS_SMALL_ENDIAN;
}

#endif //MYRPC_UTILS_H
