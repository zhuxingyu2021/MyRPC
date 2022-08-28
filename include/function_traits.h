#ifndef MYRPC_FUNCTION_TRAITS_H
#define MYRPC_FUNCTION_TRAITS_H

#include <tuple>
#include <functional>

namespace MyRPC{
    template< class T >
    class function_traits
    {
        static_assert( sizeof( T ) == 0,
                       "function_traits<T>: T is not a function type" );
    };

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
    
    template< class R, class... Ts>
    struct function_traits<std::function<R(Ts...)>> : function_traits< R( Ts... ) > {};
}

#endif //MYRPC_FUNCTION_TRAITS_H
