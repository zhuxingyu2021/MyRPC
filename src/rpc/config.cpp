#include "rpc/config.h"
#include "rpc/exception.h"

#include "macro.h"

#include <unistd.h>
#include <fcntl.h>

using namespace MyRPC;

Config::ptr Config::LoadFromJson(const std::string& json_file) {
    int fd = open(json_file.c_str(), O_RDONLY);
    MYRPC_ASSERT_EXCEPTION(fd >= 0, throw FileException(std::string("open file ") + json_file + " failed"));

    StringBuilder s;
    while(true){
        StringBuffer buf(4096);
        int sz = read(fd, buf.data, 4096);
        if(sz == 0){
            break;
        }else if(sz < 0){
            throw FileException(std::string("read file ") + json_file + " failed");
        }
        auto new_sz = strtrim<' ', '\t', '\n', '\r'>(buf.data, sz);
        buf.size = new_sz;
        s.Append(std::move(buf));
    }
    close(fd);

    StringBuffer sb(s.Concat());
    Deserializer d(sb);
    Config::ptr config = std::make_shared<Config>();

    d.Load(*config);
    return config;
}

void Config::SaveToJson(const std::string& json_file) const {
    StringBuilder s;
    JsonSerializer ser(s);

    ser.Save(*this);
    StringBuffer sb = s.Concat();

    int fd = open(json_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    MYRPC_ASSERT_EXCEPTION(fd >= 0, throw FileException(std::string("open file ") + json_file + " failed"));

    auto sz = write(fd, sb.data, sb.size);
    MYRPC_ASSERT_EXCEPTION(sz == sb.size, throw FileException(std::string("write file ") + json_file + " failed"));

    close(fd);
}
