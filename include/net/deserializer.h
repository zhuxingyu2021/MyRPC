#ifndef MYRPC_DESERIALIZER_H
#define MYRPC_DESERIALIZER_H

#define Deserializer JsonDeserializer

#include <type_traits>

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
#include "macro.h"
#include "stringbuffer.h"

namespace MyRPC{
class JsonDeserializer{
public:
    JsonDeserializer(StringBuffer& s):buffer(s){}

    template<class T, class U>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,U>;

    template<class T>
    arithmetic_type<T,JsonDeserializer&> operator>>(T& t){
        if constexpr(std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>) {
            // 浮点类型
            t = std::stod(buffer.ReadUntil<',', '}', ']'>());
        }
        else if constexpr(std::is_unsigned_v<std::decay_t<T>>) {
            // 无符号类型
            t = std::stoll(buffer.ReadUntil<',', '}', ']'>());
        }
        else{
            // 有符号类型
            t = std::stoull(buffer.ReadUntil<',', '}', ']'>());
        }
        return (*this);
    }

    JsonDeserializer& operator>>(std::string& s){
        MYRPC_ASSERT(buffer.GetChar() == '\"');
        s = std::move(buffer.ReadUntil<'\"', '\"', '\"'>());
        MYRPC_ASSERT(buffer.GetChar() == '\"');
        return *this;
    }

private:
    /**
     * @brief 读取json数组，用于反序列化vector, list, set等类型
     */
    template<class Tval, class T>
    inline void deserialize_like_vector_impl_(T& t){
        MYRPC_ASSERT(buffer.GetChar() == '[');
        while(buffer.PeekChar()!=']'){
            Tval elem;
            (*this) >> elem;
            t.emplace_back(std::move(elem));
            if(buffer.PeekChar() == ','){
                buffer.GetChar();
            }
        }
        MYRPC_ASSERT(buffer.GetChar() == ']');
    }

    template<class T>
    using isnot_string_type =  typename std::enable_if_t<(!std::is_same_v<std::decay_t<T>, std::string>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::string_view>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::wstring>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::wstring_view>),void>;

    /**
     * @brief 读取json对象的key-value对
     */
    template<class Tkey, class Tval>
    inline isnot_string_type<Tkey> deserialize_key_val_impl_(Tkey& key, Tval& value){
        if constexpr(std::is_arithmetic_v<std::decay_t<decltype(key)>>){
            // Key是算术类型
            MYRPC_ASSERT(buffer.GetChar() == '\"');
            if constexpr(std::is_same_v<std::decay_t<decltype(key)>, float> || std::is_same_v<std::decay_t<decltype(key)>, double>) {
                // 浮点类型
                key = std::stod(buffer.ReadUntil<'\"', '\"', '\"'>());
            }
            else if constexpr(std::is_unsigned_v<std::decay_t<decltype(key)>>) {
                // 无符号类型
                key = std::stoll(buffer.ReadUntil<'\"', '\"', '\"'>());
            }
            else{
                // 有符号类型
                key = std::stoull(buffer.ReadUntil<'\"', '\"', '\"'>());
            }
            MYRPC_ASSERT(buffer.GetChar() == '\"');
            MYRPC_ASSERT(buffer.GetChar() == ':');
        }
        else{
            MYRPC_ASSERT(buffer.GetChar() == '\"');
            (*this) >> key;
            MYRPC_ASSERT(buffer.GetChar() == '\"');
            MYRPC_ASSERT(buffer.GetChar() == ':');
        }
        (*this) >> value;
    }

    /**
     * @brief 读取json对象的key-value对（使用c++17 string_view）
     */
    template<class Tval>
    inline void deserialize_key_val_impl_(std::string& key, Tval& value){
        (*this) >> key;
        MYRPC_ASSERT(buffer.GetChar() == ':');
        (*this) >> value;
    }

    /**
     * @brief 读取json对象，用于反序列化map, unordered_map等类型
     */
    template<class Tkey, class Tval, class T>
    inline void deserialize_like_map_impl_(T& t){
        MYRPC_ASSERT(buffer.GetChar() == '{');

        while(buffer.PeekChar()!='}'){
            Tkey key;
            Tval val;
            deserialize_key_val_impl_(key, val);
            t.emplace(std::move(key), std::move(val));
            if(buffer.PeekChar() == ','){
                buffer.GetChar();
            }
        }

        MYRPC_ASSERT(buffer.GetChar() == '}');
    }

public:
    /**
     * 以下是反序列化各种STL容器类型/智能指针的模板函数
     */

    template<class T, size_t Num>
    JsonDeserializer& operator>>(std::array<T, Num>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::vector<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::deque<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::list<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::forward_list<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::set<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::unordered_set<T>& t){
        deserialize_like_vector_impl_<T>(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonDeserializer& operator>>(std::map<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonDeserializer& operator>>(std::unordered_map<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonDeserializer& operator>>(std::pair<Tkey, Tval>& t){
        MYRPC_ASSERT(buffer.GetChar() == '{');

        Tkey key;
        Tval val;
        deserialize_key_val_impl_(key, val);
        t.first = std::move(key);
        t.second = std::move(val);

        MYRPC_ASSERT(buffer.GetChar() == '}');
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::optional<T>& t){
        if(buffer.PeekString(4) == "null"){
            buffer.ForwardReadPointer(4);
            t = std::nullopt;
        }
        else{
            T val;
            (*this) >> val;
            t.emplace(std::move(val));
        }
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::shared_ptr<T>& t){
        t = std::move(std::make_shared<T>());
        if(buffer.PeekString(4) == "null")
            buffer.ForwardReadPointer(4);
        else
            (*this) >> *t;
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::unique_ptr<T>& t){
        t = std::move(std::make_unique<T>());
        if(buffer.PeekString(4) == "null")
            buffer.ForwardReadPointer(4);
        else
            (*this) >> *t;
        return (*this);
    }

private:
    StringBuffer& buffer;
};
}


#endif //MYRPC_DESERIALIZER_H
