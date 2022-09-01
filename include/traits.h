#ifndef MYRPC_TRAITS_H
#define MYRPC_TRAITS_H

#include <tuple>
#include <functional>
#include <future>

namespace MyRPC{
    template< class T >
    struct function_traits:function_traits<decltype(&std::remove_reference_t<T>::operator())>{};

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) >
    {
        constexpr static const std::size_t arity = sizeof...( Ts );

        using arg_type = std::tuple<std::decay_t<Ts>...>;
        using result_type = R;

        template<class Callable>
        static R apply(Callable&& func, const arg_type& arg){
            return apply_impl_(std::forward<Callable>(func), arg, std::index_sequence_for<Ts...>{});
        }

    private:
        template<class Callable, size_t... Is>
        static R apply_impl_(Callable&& func, const arg_type& arg, std::index_sequence<Is...>){
            return func(std::get<Is>(arg)...);
        }
    };

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) const > : function_traits< R( Ts... ) > {};

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) & > : function_traits< R( Ts... ) > {};

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) const & > : function_traits< R( Ts... ) > {};

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) && > : function_traits< R( Ts... ) > {};

    template< class R, class... Ts >
    struct function_traits< R( Ts... ) const && > : function_traits< R( Ts... ) > {};

    // 函数指针类型
    template< class R, class... Ts>
    struct function_traits<R (*)(Ts...)> : function_traits< R( Ts... ) > {};

    // 成员函数
    template< class R, class ClassType, class... Ts>
    struct function_traits<R (ClassType::*)(Ts...)> : function_traits< R( Ts... ) > {
        using class_type = ClassType;
    };

    template< class R, class ClassType, class... Ts>
    struct function_traits<R (ClassType::*)(Ts...) const> : function_traits< R (ClassType::*)(Ts...) > {};


    template< class T >
    struct promise_traits{
        static_assert(std::is_void_v<T> == true , "Only work for promise type");
    };

    template< class T >
    struct promise_traits<std::promise<T>>{
        using type = T;
    };
}

#endif //MYRPC_TRAITS_H
