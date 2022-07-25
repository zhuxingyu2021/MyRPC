#ifndef MYRPC_SERIALIZER_H
#define MYRPC_SERIALIZER_H

#define Serializer JsonSerializer

#include <type_traits>
#include <rapidjson/writer.h>
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

namespace MyRPC {
/**
 * @brief 支持将STL容器类型序列化为json格式的序列化器
 *        目前支持以下STL容器类型：
 *        vector, deque, list, forward_list
 *        set, unordered_set, tuple
 *        string, map, unordered_map, optional
 *        不支持裸指针，但支持以下智能指针：
 *        shared_ptr, unique_ptr
 * @note 用法如下：
 * @code  rapidjson::StringBuffer buffer;
          JsonSerializer serializer(buffer);
          std::vector<int> vec = {32, 901, 12, 29, -323};
          serializer << vec1;
          std::cout << s.GetString() << std::endl;
 */
class JsonSerializer {
public:
    JsonSerializer(rapidjson::StringBuffer& s):writer(s){}

    template<class T, class U>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,U>;

    /**
     * @brief 模板函数的递归终点，序列化一个基本算术类型
     */
    template<class T>
    arithmetic_type<T,JsonSerializer&> operator<<(const T& t){
        if constexpr(std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>){
            // 浮点类型
            writer.Double(t);
        }else if constexpr(sizeof(std::decay_t<T>) <= 4){
            // 8, 16, 32位类型
            if constexpr(std::is_unsigned_v<std::decay_t<T>>) writer.Uint(t); // 无符号类型
            else writer.Int(t); //有符号类型
        }else{
            // 64位类型
            if constexpr(std::is_unsigned_v<std::decay_t<T>>) writer.Uint64(t); // 无符号类型
            else writer.Int64(t); //有符号类型
        }
        return (*this);
    }

    /**
     * @brief 模板函数的递归终点，序列化一个字符串
     */
    JsonSerializer& operator<<(const std::string& s){
        writer.String(s.c_str());
        return (*this);
    }

private:
    /**
     * @brief 生成json数组，用于序列化vector, list, set等类型
     */
    template<class T>
    inline void serialize_like_vector_impl_(const T& t){
        writer.StartArray();
        for(const auto& element:t)
            (*this) << element;
        writer.EndArray();
    }

    /**
     * @brief 生成json对象的key-value对
     */
    template<class Tkey, class Tval>
    inline void serialize_key_val_impl_(const Tkey& key, const Tval& value){
        if constexpr(std::is_same_v<std::decay_t<decltype(key)>, std::string>){
            // Key是字符串
            writer.Key(key.c_str());
        }
        else if constexpr(std::is_arithmetic_v<std::decay_t<decltype(key)>>){
            // Key是算术类型
            writer.Key(std::to_string(key).c_str()); // std::to_string转化为字符串后写入json
        }
        else{
            rapidjson::StringBuffer s;
            JsonSerializer m(s);
            m << key;
            writer.Key(s.GetString());
        }
        (*this) << value;
    }

    /**
     * @brief 生成json对象，用于序列化map, unordered_map等类型
     */
    template<class Tkey, class T>
    inline void serialize_like_map_impl_(const T& t){
        writer.StartObject();

        for(const auto& [key, value]:t) {
            serialize_key_val_impl_(key, value);
        }

        writer.EndObject();
    }

    /**
     * @brief 生成json数组，用于序列化tuple类型
     */
    template<class Tuple, std::size_t... Is>
    inline void serialize_tuple_impl_(const Tuple& t,std::index_sequence<Is...>){
        writer.StartArray();
        (((*this) << std::get<Is>(t)), ...);
        writer.EndArray();
    }

public:
    /**
     * 以下是序列化各种STL容器类型/智能指针的模板函数
     */

    template<class T, size_t Num>
    JsonSerializer& operator<<(const std::array<T, Num>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::vector<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::deque<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::list<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::forward_list<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::set<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::unordered_set<T>& t){
        serialize_like_vector_impl_(t);
        return (*this);
    }

    template<class ...Args>
    JsonSerializer& operator<<(const std::tuple<Args...>& t){
        serialize_tuple_impl_(t, std::index_sequence_for<Args...>{});
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonSerializer& operator<<(const std::map<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey>(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonSerializer& operator<<(const std::unordered_map<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey>(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonSerializer& operator<<(const std::pair<Tkey, Tval>& t){
        writer.StartObject();
        const auto& [key,value] = t;
        serialize_key_val_impl_(key, value);

        writer.EndObject();
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::optional<T>& t){
        if(t)
            (*this) << *t;
        else
            writer.Null();
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::shared_ptr<T>& t){
        if(t)
            (*this) << *t;
        else
            writer.Null();
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::unique_ptr<T>& t){
        if(t)
            (*this) << *t;
        else
            writer.Null();
        return (*this);
    }

private:

    rapidjson::Writer<rapidjson::StringBuffer> writer;
};
}

#endif //MYRPC_SERIALIZER_H
