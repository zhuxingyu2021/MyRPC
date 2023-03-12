#ifndef MYRPC_JSON_SERIALIZER_H
#define MYRPC_JSON_SERIALIZER_H

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
#include <string_view>
#include <map>
#include <unordered_map>
#include <optional>
#include <memory>

#include "utils.h"
#include "buffer/buffer.h"
#include "buffer/stringbuffer.h"

#include "serialization/placeholder.h"

namespace MyRPC {
/**
 * @brief 将STL容器类型、自定义结构体类型序列化为json格式的序列化器
 *        目前支持以下STL容器类型：
 *        vector, deque, list, forward_list
 *        set, unordered_set, tuple
 *        map, unordered_map, multimap, unordered_multimap
 *        optional, string
 *        不支持裸指针，但支持以下智能指针：
 *        shared_ptr, unique_ptr
 *        支持自定义结构体类型
 *
 * @note 用法如下：
 * @code
 */
class JsonSerializer {
public:
    JsonSerializer(WriteBuffer& s): buffer(s){}

    template<class T>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,void>;

    /**
     * @brief 模板函数的递归终点，序列化一个基本算术类型
     */
    template<class T>
    arithmetic_type<T> Save(const T& t){
        if constexpr(std::is_same_v<std::decay_t<T>, bool>){
            // 布尔类型
            buffer.Append(t ? "true" : "false");
        }else {
            buffer.Append(std::to_string(t));
        }
    }

    /**
     * @brief 模板函数的递归终点，序列化一个字符串
     */
    void Save(const std::string& s){
        buffer.Append('\"');
        buffer.Append(s);
        buffer.Append('\"');
    }

private:
    /**
     * @brief 生成json数组，用于序列化vector, list, set等类型
     */
    template<class T>
    inline void serialize_like_vector_impl_(const T& t){
        buffer.Append('[');
        for(const auto& element:t) {
            Save(element);
            buffer.Append(',');
        }
        if(!t.empty())
            buffer.Backward(1); // 删除最后一个逗号
        buffer.Append(']');
    }

    template<class T>
    using isnot_string_type =  typename std::enable_if_t<(!std::is_same_v<std::decay_t<T>, std::string>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::string_view>), void>;

    /**
     * @brief 生成json对象的key-value对
     * @note 若可以是字符串类型，对应的json对象为："key":value
     *       否则，对应的json对象为：{"key":key,"value":value}
     */
    template<class Tkey, class Tval>
    inline isnot_string_type<Tkey> serialize_key_val_impl_(const Tkey& key, const Tval& value){
        buffer.Append("{\"key\":");
        Save(key);
        buffer.Append(",\"value\":");
        Save(value);
        buffer.Append('}');
    }

    /**
     * @brief 生成json对象的key-value对（使用c++17 string_view）
     */
    template<class Tval>
    inline void serialize_key_val_impl_(const std::string& key, const Tval& value){
        buffer.Append('\"');
        buffer.Append(key);
        buffer.Append("\":");
        Save(value);
    }

    /**
     * @brief 生成json对象，用于序列化map, unordered_map等类型
     */
    template<class Tkey, class Tval, class T>
    inline void serialize_like_map_impl_(const T& t){
        constexpr bool is_string = (std::is_same_v<std::decay_t<Tkey>, std::string>)||
                                   (std::is_same_v<std::decay_t<Tkey>, std::string_view>);
        if constexpr(is_string){
            buffer.Append('{');
            for(const auto& [key, value]:t) {
                buffer.Append('\"');
                buffer.Append(key);
                buffer.Append("\":");
                Save(value);
                buffer.Append(',');
            }
            if(!t.empty())
                buffer.Backward(1); // 删除最后一个逗号
            buffer.Append('}');
        }else{
            buffer.Append('[');
            for(const auto& [key, value]:t) {
                buffer.Append("{\"key\":");
                Save(key);
                buffer.Append(",\"value\":");
                Save(value);
                buffer.Append("},");
            }
            if(!t.empty())
                buffer.Backward(1); // 删除最后一个逗号
            buffer.Append(']');
        }
    }

    /**
     * @brief 生成json数组，用于序列化tuple类型
     */
    template<class Tuple, size_t... Is>
    inline void serialize_tuple_impl_(const Tuple& t,std::index_sequence<Is...>){
        buffer.Append('[');
        ((Save(std::get<Is>(t)), buffer.Append(',')), ...);
        buffer.Backward(1); // 删除最后一个逗号
        buffer.Append(']');
    }

public:
    /**
     * 以下是序列化各种STL容器类型/智能指针的模板函数
     */

    template<class T, size_t Num>
    void Save(const std::array<T, Num>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::vector<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::deque<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::list<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::forward_list<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::set<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class T>
    void Save(const std::unordered_set<T>& t){
        serialize_like_vector_impl_(t);
    }

    template<class ...Args>
    void Save(const std::tuple<Args...>& t){
        serialize_tuple_impl_(t, std::index_sequence_for<Args...>{});
    }

    template<class Tkey, class Tval>
    void Save(const std::map<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey,Tval>(t);
    }

    template<class Tkey, class Tval>
    void Save(const std::multimap<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey,Tval>(t);
    }

    template<class Tkey, class Tval>
    void Save(const std::unordered_map<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey,Tval>(t);
    }

    template<class Tkey, class Tval>
    void Save(const std::unordered_multimap<Tkey, Tval>& t){
        serialize_like_map_impl_<Tkey,Tval>(t);
    }

    template<class Tkey, class Tval>
    void Save(const std::pair<Tkey, Tval>& t){
        constexpr bool is_string = (std::is_same_v<std::decay_t<Tkey>, std::string>)||
                                   (std::is_same_v<std::decay_t<Tkey>, std::string_view>);
        if constexpr(is_string){
            //key是字符串类型
            buffer.Append('{');
        }else{
            buffer.Append('[');
        }
        const auto& [key,value] = t;
        serialize_key_val_impl_(key, value);

        if constexpr(is_string){
            //key是字符串类型
            buffer.Append('}');
        }else{
            buffer.Append(']');
        }
    }

    template<class T>
    void Save(const std::optional<T>& t){
        if(t)
            Save(*t);
        else
            buffer.Append("null");
    }

    template<class T>
    void Save(const std::shared_ptr<T>& t){
        if(t)
            Save(*t);
        else
            buffer.Append("null");
    }

    template<class T>
    void Save(const std::unique_ptr<T>& t){
        if(t)
            Save(*t);
        else
            buffer.Append("null");
    }

private:

    WriteBuffer& buffer;

public:
    /**
     * @brief 以下是针对struct的序列化
     */

    inline void serialize_struct_begin_impl_(){
        buffer.Append('{');
    }
    inline void serialize_struct_end_impl_(){
        buffer.Backward(1);
        buffer.Append('}');
    }

    template<class T>
    inline void serialize_item_impl_(const std::string& key, const T& val){
        serialize_key_val_impl_(key, val);
        buffer.Append(',');
    }

    template<class T>
    using struct_class_type =  typename std::enable_if_t<(std::is_class_v<std::decay_t<T>>)&&
                                                        (!std::is_same_v<std::decay_t<T>, std::string>)&&
                                                        (!std::is_same_v<std::decay_t<T>, std::string_view>),void>;

    template<class T>
    struct_class_type<T> Save(const T& t) {
        t.Save(*this);
    }

};

};

#define SAVE_BEGIN_DEF void Save(MyRPC::JsonSerializer& serializer) const{
#define SAVE_BEGIN_WRITE serializer.serialize_struct_begin_impl_();
#define SAVE_BEGIN SAVE_BEGIN_DEF SAVE_BEGIN_WRITE

#define SAVE_ITEM(x) serializer.serialize_item_impl_(#x, x);
#define SAVE_ALIAS_ITEM(alias, x) serializer.serialize_item_impl_(#alias, x);

#define SAVE_END_WRITE serializer.serialize_struct_end_impl_();
#define SAVE_END_DEF }
#define SAVE_END SAVE_END_WRITE SAVE_END_DEF

#endif //MYRPC_JSON_SERIALIZER_H
