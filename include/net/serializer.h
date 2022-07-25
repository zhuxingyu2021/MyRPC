#ifndef MYRPC_SERIALIZER_H
#define MYRPC_SERIALIZER_H

#define Serializer JsonSerializer

#include <type_traits>
#include "stringbuffer.h"

#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <tuple>
#include <string>
#include <string_view>
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
 *        map, unordered_map, optional
 *        string
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
    JsonSerializer(StringBuffer& s):buffer(s){}

    template<class T, class U>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,U>;

    /**
     * @brief 模板函数的递归终点，序列化一个基本算术类型
     */
    template<class T>
    arithmetic_type<T,JsonSerializer&> operator<<(const T& t){
        buffer << std::to_string(t);
        return (*this);
    }

    /**
     * @brief 模板函数的递归终点，序列化一个字符串
     */
    JsonSerializer& operator<<(const std::string_view s){
        buffer << "\"" << s << "\"";
        return (*this);
    }

private:
    /**
     * @brief 生成json数组，用于序列化vector, list, set等类型
     */
    template<class T>
    inline void serialize_like_vector_impl_(const T& t){
        buffer << "[";
        for(const auto& element:t) {
            (*this) << element;
            buffer << ",";
        }
        if(!t.empty())
            buffer.RollbackWritePointer(1); // 删除最后一个逗号
        buffer << "]";
    }

    template<class T>
    using isnot_string_type =  typename std::enable_if_t<(!std::is_same_v<std::decay_t<T>, std::string>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::string_view>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::wstring>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::wstring_view>),void>;

    /**
     * @brief 生成json对象的key-value对
     */
    template<class Tkey, class Tval>
    inline isnot_string_type<Tkey> serialize_key_val_impl_(const Tkey& key, const Tval& value){
        if constexpr(std::is_arithmetic_v<std::decay_t<decltype(key)>>){
            // Key是算术类型
            buffer << "\"" << std::to_string(key) << "\":";
        }
        else{
            buffer << "\"";
            (*this) << key;
            buffer << "\":";
        }
        (*this) << value;
    }

    /**
     * @brief 生成json对象的key-value对（使用c++17 string_view）
     */
    template<class Tval>
    inline void serialize_key_val_impl_(const std::string_view key, const Tval& value){
        buffer << "\"" << key << "\":";
        (*this) << value;
    }

    /**
     * @brief 生成json对象，用于序列化map, unordered_map等类型
     */
    template<class T>
    inline void serialize_like_map_impl_(const T& t){
        buffer << "{";

        for(const auto& [key, value]:t) {
            serialize_key_val_impl_(key, value);
            buffer << ",";
        }
        if(!t.empty())
            buffer.RollbackWritePointer(1); // 删除最后一个逗号
        buffer << "}";
    }

    /**
     * @brief 生成json数组，用于序列化tuple类型
     */
    template<class Tuple, std::size_t... Is>
    inline void serialize_tuple_impl_(const Tuple& t,std::index_sequence<Is...>){
        buffer << "[";
        (((*this) << std::get<Is>(t), buffer<<","), ...);
        buffer.RollbackWritePointer(1); // 删除最后一个逗号
        buffer << "]";
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
        serialize_like_map_impl_(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonSerializer& operator<<(const std::unordered_map<Tkey, Tval>& t){
        serialize_like_map_impl_(t);
        return (*this);
    }

    template<class Tkey, class Tval>
    JsonSerializer& operator<<(const std::pair<Tkey, Tval>& t){
        buffer << "{";
        const auto& [key,value] = t;
        serialize_key_val_impl_(key, value);

        buffer << "}";
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::optional<T>& t){
        if(t)
            (*this) << *t;
        else
            buffer << "null";
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::shared_ptr<T>& t){
        if(t)
            (*this) << *t;
        else
            buffer << "null";
        return (*this);
    }

    template<class T>
    JsonSerializer& operator<<(const std::unique_ptr<T>& t){
        if(t)
            (*this) << *t;
        else
            buffer << "null";
        return (*this);
    }

private:

    StringBuffer& buffer;
};
}

#endif //MYRPC_SERIALIZER_H
