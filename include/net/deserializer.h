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
    JsonDeserializer(StringBuffer& s): buffer_reader(s.GetStringBufferReader()){}

    /**
     * @brief 从字符串缓冲区的起始位置开始重新反序列化
     */
    void Reset(){
        buffer_reader.Reset();
    }

    template<class T, class U>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,U>;

    template<class T>
    arithmetic_type<T,JsonDeserializer&> operator>>(T& t){
        if constexpr(std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>) {
            // 浮点类型
            t = std::stod(buffer_reader.ReadUntil<',', '}', ']'>());
        }
        else if constexpr(std::is_unsigned_v<std::decay_t<T>>) {
            // 无符号类型
            t = std::stoll(buffer_reader.ReadUntil<',', '}', ']'>());
        }
        else{
            // 有符号类型
            t = std::stoull(buffer_reader.ReadUntil<',', '}', ']'>());
        }
        return (*this);
    }

    JsonDeserializer& operator>>(std::string& s){
        MYRPC_ASSERT(buffer_reader.GetChar() == '\"');
        s = std::move(buffer_reader.ReadUntil<'\"'>());
        MYRPC_ASSERT(buffer_reader.GetChar() == '\"');
        return *this;
    }

private:
    /**
     * @brief 读取json数组，用于反序列化vector, list, set等类型
     */
    template<class Tval, class T>
    inline void deserialize_like_vector_impl_(T& t){
        MYRPC_ASSERT(buffer_reader.GetChar() == '[');
        while(buffer_reader.PeekChar()!=']'){
            Tval elem;
            (*this) >> elem;
            t.emplace_back(std::move(elem));
            if(buffer_reader.PeekChar() == ','){
                buffer_reader.GetChar();
            }
        }
        MYRPC_ASSERT(buffer_reader.GetChar() == ']');
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
        MYRPC_ASSERT(buffer_reader.PeekString(7) == "{\"key\":");
        buffer_reader.ForwardReadPointer(7);
        (*this) >> key;
        MYRPC_ASSERT(buffer_reader.PeekString(9) == ",\"value\":");
        buffer_reader.ForwardReadPointer(9);
        (*this) >> value;
        MYRPC_ASSERT(buffer_reader.GetChar() == '}');
    }

    /**
     * @brief 读取json对象的key-value对（当key为string）
     */
    template<class Tval>
    inline void deserialize_key_val_impl_(std::string& key, Tval& value){
        (*this) >> key;
        MYRPC_ASSERT(buffer_reader.GetChar() == ':');
        (*this) >> value;
    }

    /**
     * @brief 读取json对象，用于反序列化map, unordered_map等类型
     */
    template<class Tkey, class Tval, class T>
    inline void deserialize_like_map_impl_(T& t){
        auto c1 = buffer_reader.GetChar();
        MYRPC_ASSERT(c1 == '{' || c1 == '[');

        char c2 = buffer_reader.PeekChar();
        while(c2 != '}' && c2 != ']'){
            Tkey key;
            Tval val;
            deserialize_key_val_impl_(key, val);
            t.emplace(std::move(key), std::move(val));
            if(buffer_reader.PeekChar() == ','){
                buffer_reader.GetChar();
            }
            c2 = buffer_reader.PeekChar();
        }

        auto c3 = buffer_reader.GetChar();
        MYRPC_ASSERT(c3 == '}' || c3 == ']');
    }

    /**
     * @brief 构造tuple的各个元素
     */
    template<class T>
    inline T make_tuple_internal_(){
        T t;
        (*this) >> t;
        buffer_reader.GetChar();
        return t;
    }

    /**
     * @brief 读取json对象，用于反序列化tuple类型
     */
    template<class Tuple, size_t... Is>
    inline void deserialize_tuple_impl_(Tuple& t,std::index_sequence<Is...>){
        MYRPC_ASSERT(buffer_reader.GetChar() == '[');
        ((std::get<Is>(t) = std::move(make_tuple_internal_<std::tuple_element_t<Is, Tuple>>())), ...);
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

    template<class ...Args>
    JsonDeserializer& operator>>(std::tuple<Args...>& t){
        deserialize_tuple_impl_(t, std::index_sequence_for<Args...>{});
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
        char c1 = buffer_reader.GetChar();
        MYRPC_ASSERT(c1 == '{' || c1 == '[');

        Tkey key;
        Tval val;
        deserialize_key_val_impl_(key, val);
        t.first = std::move(key);
        t.second = std::move(val);

        char c3 = buffer_reader.GetChar();
        MYRPC_ASSERT(c3 == '}' || c3 == ']');
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::optional<T>& t){
        if(buffer_reader.PeekString(4) == "null"){
            buffer_reader.ForwardReadPointer(4);
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
        if(buffer_reader.PeekString(4) == "null")
            buffer_reader.ForwardReadPointer(4);
        else
            (*this) >> *t;
        return (*this);
    }

    template<class T>
    JsonDeserializer& operator>>(std::unique_ptr<T>& t){
        t = std::move(std::make_unique<T>());
        if(buffer_reader.PeekString(4) == "null")
            buffer_reader.ForwardReadPointer(4);
        else
            (*this) >> *t;
        return (*this);
    }

private:
    StringBuffer::StringBufferReader buffer_reader;
};
}


#endif //MYRPC_DESERIALIZER_H
