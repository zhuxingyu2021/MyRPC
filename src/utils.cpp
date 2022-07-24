#include "utils.h"

namespace MyRPC{
    // 判断本地字节序是否是小端字节序
    bool IS_SMALL_ENDIAN = [](){
        int i=1;
        return (*((char*)(&i)))==i;
    }();
}

