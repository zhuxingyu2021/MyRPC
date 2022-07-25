#ifndef MYRPC_DESERIALIZER_H
#define MYRPC_DESERIALIZER_H

#define Deserializer JsonDeserializer

#include <type_traits>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>

#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <tuple>
#include <string>
#include <map>
#include <unordered_map>
#include <optional>
#include <memory>

#include "utils.h"

namespace MyRPC{
class JsonDeserializer{
public:

    JsonDeserializer& operator>>(std::string& s){

    }
};
}


#endif //MYRPC_DESERIALIZER_H
