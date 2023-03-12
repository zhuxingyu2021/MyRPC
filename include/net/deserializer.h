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
#include "debug.h"
#include "stringbuffer.h"

#include "net/exception.h"

namespace MyRPC{
class JsonDeserializer{
public:
    JsonDeserializer(StringBuffer& sb): buffer(sb){}

    template<class T>
    using arithmetic_type =  typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>,void>;

    template<class T>
    arithmetic_type<T> Load(T& t){
        if constexpr(std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>) {
            // 浮点类型
            t = std::stod(buffer.ReadUntil<',', '}', ']'>());
        }
        if constexpr(std::is_same_v<std::decay_t<T>, bool>) {
            // 布尔类型
            t = (buffer.ReadUntil<',', '}', ']'>() == "true");
        }
        else if constexpr(std::is_unsigned_v<std::decay_t<T>>) {
            // 无符号类型
            t = std::stoll(buffer.ReadUntil<',', '}', ']'>());
        }
        else{
            // 有符号类型
            t = std::stoull(buffer.ReadUntil<',', '}', ']'>());
        }
    }

    void Load(std::string& s){
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '\"', throw JsonDeserializerException(buffer.GetPos()));
        s = std::move(buffer.ReadUntil<'\"'>());
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '\"', throw JsonDeserializerException(buffer.GetPos()));
    }

private:
    /**
     * @brief 读取json数组，用于反序列化vector, list等类型
     */
    template<class Tval, class T>
    inline void deserialize_like_vector_impl_(T& t){
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '[', throw JsonDeserializerException(buffer.GetPos()));
        while(buffer.PeekChar() != ']'){
            Tval elem;
            Load(elem);
            t.emplace_back(std::move(elem));
            if(buffer.PeekChar() == ','){
                buffer.GetChar();
            }
        }
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == ']', throw JsonDeserializerException(buffer.GetPos()));
    }

    /**
     * @brief 读取json数组，用于反序列化set, unordered_set等类型
     */
    template<class Tval, class T>
    inline void deserialize_like_set_impl_(T& t){
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '[', throw JsonDeserializerException(buffer.GetPos()));
        while(buffer.PeekChar() != ']'){
            Tval elem;
            Load(elem);
            t.insert(std::move(elem));
            if(buffer.PeekChar() == ','){
                buffer.GetChar();
            }
        }
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == ']', throw JsonDeserializerException(buffer.GetPos()));
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
        MYRPC_ASSERT_EXCEPTION(buffer.PeekString(7) == "{\"key\":", throw JsonDeserializerException(buffer.GetPos()));
        buffer.Forward(7);
        Load(key);
        MYRPC_ASSERT_EXCEPTION(buffer.PeekString(9) == ",\"value\":", throw JsonDeserializerException(buffer.GetPos()));
        buffer.Forward(9);
        Load(value);
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '}', throw JsonDeserializerException(buffer.GetPos()));
    }

    /**
     * @brief 读取json对象的key-value对（当key为string）
     */
    template<class Tval>
    inline void deserialize_key_val_impl_(std::string& key, Tval& value){
        Load(key);
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == ':', throw JsonDeserializerException(buffer.GetPos()));
        Load(value);
    }

    /**
     * @brief 读取json对象，用于反序列化map, unordered_map等类型
     */
    template<class Tkey, class Tval, class T>
    inline void deserialize_like_map_impl_(T& t){
        auto c1 = buffer.GetChar();
        MYRPC_ASSERT_EXCEPTION(c1 == '{' || c1 == '[', throw JsonDeserializerException(buffer.GetPos()));

        char c2 = buffer.PeekChar();
        while(c2 != '}' && c2 != ']'){
            Tkey key;
            Tval val;
            deserialize_key_val_impl_(key, val);
            t.emplace(std::move(key), std::move(val));
            if(buffer.PeekChar() == ','){
                buffer.GetChar();
            }
            c2 = buffer.PeekChar();
        }

        auto c3 = buffer.GetChar();
        MYRPC_ASSERT_EXCEPTION(c3 == '}' || c3 == ']', throw JsonDeserializerException(buffer.GetPos()));
    }

    /**
     * @brief 构造tuple的各个元素
     */
    template<class T>
    inline T make_tuple_internal_(){
        T t;
        Load(t);
        buffer.GetChar();
        return t;
    }

    /**
     * @brief 读取json对象，用于反序列化tuple类型
     */
    template<class Tuple, size_t... Is>
    inline void deserialize_tuple_impl_(Tuple& t,std::index_sequence<Is...>){
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '[', throw JsonDeserializerException(buffer.GetPos()));
        ((std::get<Is>(t) = std::move(make_tuple_internal_<std::tuple_element_t<Is, Tuple>>())), ...);
    }

public:
    /**
     * 以下是反序列化各种STL容器类型/智能指针的模板函数
     */

    template<class T, size_t Num>
    void Load(std::array<T, Num>& t){
        deserialize_like_vector_impl_<T>(t);
    }

    template<class T>
    void Load(std::vector<T>& t){
        deserialize_like_vector_impl_<T>(t);
    }

    template<class T>
    void Load(std::deque<T>& t){
        deserialize_like_vector_impl_<T>(t);
    }

    template<class T>
    void Load(std::list<T>& t){
        deserialize_like_vector_impl_<T>(t);
    }

    template<class T>
    void Load(std::forward_list<T>& t){
        deserialize_like_vector_impl_<T>(t);
    }

    template<class T>
    void Load(std::set<T>& t){
        deserialize_like_set_impl_<T>(t);
    }

    template<class T>
    void Load(std::unordered_set<T>& t){
        deserialize_like_set_impl_<T>(t);
    }

    template<class ...Args>
    void Load(std::tuple<Args...>& t){
        deserialize_tuple_impl_(t, std::index_sequence_for<Args...>{});
    }

    template<class Tkey, class Tval>
    void Load(std::map<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
    }

    template<class Tkey, class Tval>
    void Load(std::multimap<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
    }

    template<class Tkey, class Tval>
    void Load(std::unordered_map<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
    }

    template<class Tkey, class Tval>
    void Load(std::unordered_multimap<Tkey, Tval>& t){
        deserialize_like_map_impl_<Tkey, Tval>(t);
    }

    template<class Tkey, class Tval>
    void Load(std::pair<Tkey, Tval>& t){
        char c1 = buffer.GetChar();
        MYRPC_ASSERT_EXCEPTION(c1 == '{' || c1 == '[', throw JsonDeserializerException(buffer.GetPos()));

        Tkey key;
        Tval val;
        deserialize_key_val_impl_(key, val);
        t.first = std::move(key);
        t.second = std::move(val);

        char c3 = buffer.GetChar();
        MYRPC_ASSERT_EXCEPTION(c3 == '}' || c3 == ']', throw JsonDeserializerException(buffer.GetPos()));
    }

    template<class T>
    void Load(std::optional<T>& t){
        if(buffer.PeekString(4) == "null"){
            buffer.Forward(4);
            t = std::nullopt;
        }
        else{
            T val;
            Load(val);
            t.emplace(std::move(val));
        }
    }

    template<class T>
    void Load(std::shared_ptr<T>& t){
        t = std::move(std::make_shared<T>());
        if(buffer.PeekString(4) == "null")
            buffer.Forward(4);
        else
            Load(*t);
    }

    template<class T>
    void Load(std::unique_ptr<T>& t){
        t = std::move(std::make_unique<T>());
        if(buffer.PeekString(4) == "null")
            buffer.Forward(4);
        else
            Load(*t);
    }

private:
    StringBuffer& buffer;

public:
    /**
     * @brief 以下是针对struct的反序列化
     */

    inline void deserialize_struct_begin_impl_(){
        MYRPC_ASSERT_EXCEPTION(buffer.GetChar() == '{', throw JsonDeserializerException(buffer.GetPos()));
    }
    inline void deserialize_struct_end_impl_(){
    }

    template<class T>
    inline void deserialize_item_impl_(const std::string_view key, T& val){
        std::string key_str;

        deserialize_key_val_impl_(key_str, val);
        MYRPC_ASSERT_EXCEPTION(key == key_str, throw JsonDeserializerException(buffer.GetPos()));

        char c = buffer.GetChar();
        MYRPC_ASSERT_EXCEPTION(c == ',' || c == '}', throw JsonDeserializerException(buffer.GetPos()));
    }

    template<class T>
    using struct_class_type =  typename std::enable_if_t<(std::is_class_v<std::decay_t<T>>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::string>)&&
                                                         (!std::is_same_v<std::decay_t<T>, std::string_view>),void>;

    template<class T>
    struct_class_type<T> Load(T& t) {
        t.Load(*this);
    }
};
}

#define LOAD_BEGIN void Load(MyRPC::JsonDeserializer& deserializer){ \
                   deserializer.deserialize_struct_begin_impl_();

#define LOAD_ITEM(x) deserializer.deserialize_item_impl_(#x, x);
#define LOAD_ALIAS_ITEM(alias, x) deserializer.deserialize_item_impl_(#alias, x);
#define LOAD_END deserializer.deserialize_struct_end_impl_();}

#endif //MYRPC_DESERIALIZER_H
