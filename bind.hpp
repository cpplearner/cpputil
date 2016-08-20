#pragma once

#include <functional>
#include <type_traits>
#include <tuple>

#include "is_reference_wrapper.hpp"
#include "invoke.hpp"

template<class TiD,
    bool = is_reference_wrapper_v<TiD>,
    bool = std::is_bind_expression_v<TiD>,
    bool = ((std::is_placeholder_v<TiD>) > 0)>
struct BoundArgument;

template<class TiD>
struct BoundArgument<TiD, true, false, false> {
    template<class cvTiD, class... Uj>
    static constexpr auto value(cvTiD& tid, Uj&&...)
        -> typename TiD::type&
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTiD>, TiD>);
        return tid.get();
    }
    template<class...>
    using type = typename TiD::type&;
    template<class...>
    using const_type = typename TiD::type&;
};

template<class TiD>
struct BoundArgument<TiD, false, true, false> {
    template<class cvTiD, class... Uj>
    static constexpr auto value(cvTiD& tid, Uj&&... uj)
        -> std::result_of_t<cvTiD& (Uj&&...)>
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTiD>, TiD>);
        return tid(std::forward<Uj>(uj)...);
    }
    template<class... Uj>
    using type = std::result_of_t<TiD& (Uj&&...)>;
    template<class... Uj>
    using const_type = std::result_of_t<TiD const& (Uj&&...)>;
};

template<class TiD>
struct BoundArgument<TiD, false, false, true> {
    static constexpr int position { std::is_placeholder_v<TiD> - 1 };
    template<class cvTiD, class... Uj>
    static constexpr auto value(cvTiD& tid, Uj&&... uj)
        -> std::tuple_element_t<position,std::tuple<Uj...>>
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTiD>, TiD>);
        return std::get<position>(std::forward_as_tuple(std::forward<Uj>(uj)...));
    }
    template<class... Uj>
    using type = std::tuple_element_t<position,std::tuple<Uj...>>;
    template<class... Uj>
    using const_type = std::tuple_element_t<position,std::tuple<Uj...>>;
};

template<class TiD>
const int BoundArgument<TiD, false, false, true>::position;

template<class TiD, bool, bool, bool>
struct BoundArgument {
    template<class cvTiD, class... Uj>
    static constexpr auto value(cvTiD& tid, Uj&&...)
        -> cvTiD&
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTiD>, TiD>);
        return tid;
    }
    template<class...>
    using type = TiD&;
    template<class...>
    using const_type = TiD const&;
};

template<class FD, class R, class... BoundArgs>
struct Bind {
    FD fd;
    std::tuple<BoundArgs...> bound_args;
private:
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<std::is_same_v<T,void(void)>, int> = 0>
    static constexpr decltype(auto) unpack_bound_args(cvFD& fd,
                         /* cv std::tuple<TiD...>& */ BoundArgTpl& bound_args,
                                                      std::index_sequence<idx...>,
                                                      UnBoundArgs&&... unbound_args
                                                     ) {
        return (invoke)(fd, BoundArgument<BoundArgs>::value(std::get<idx>(bound_args),
                                                            std::forward<UnBoundArgs>(unbound_args)...)...);
    }
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<!std::is_same_v<T,void(void)>, int> = 0>
    static constexpr T unpack_bound_args(cvFD& fd,
            /* cv std::tuple<TiD...>& */ BoundArgTpl& bound_args,
                                         std::index_sequence<idx...>,
                                         UnBoundArgs&&... unbound_args
                                        ) {
        return (invoke<T>)(fd,BoundArgument<BoundArgs>::value(std::get<idx>(bound_args),
                                                              std::forward<UnBoundArgs>(unbound_args)...)...);
    }
public:
    template<class... UnBoundArgs>
    constexpr decltype(auto) operator()(UnBoundArgs&&... unbound_args) {
        static_assert(
            std::is_callable_v<
                FD& (typename BoundArgument<BoundArgs>::template type<UnBoundArgs&&...>...)
            >,
            "The target object is not callable with the given arguments."
        );
        return unpack_bound_args(fd, bound_args,
                                 std::index_sequence_for<BoundArgs...>{},
                                 std::forward<UnBoundArgs>(unbound_args)...);
    }
    
    template<class... UnBoundArgs>
    constexpr decltype(auto) operator()(UnBoundArgs&&... unbound_args) const {
        static_assert(
            std::is_callable_v<
                FD const& (typename BoundArgument<BoundArgs>::template const_type<UnBoundArgs&&...>...)
            >,
            "The target object is not callable with the given arguments."
        );
        return unpack_bound_args(fd, bound_args,
                                 std::index_sequence_for<BoundArgs...>{},
                                 std::forward<UnBoundArgs>(unbound_args)...);
    }
};

namespace std {
    template<class FD, class R, class... BoundArgs>
    struct is_bind_expression<Bind<FD, R, BoundArgs...>> : true_type {};
    template<class FD, class R, class... BoundArgs>
    struct is_bind_expression<const Bind<FD, R, BoundArgs...>> : true_type {};
}

template<class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, void(void), std::decay_t<BoundArgs>...>
    bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}

template<class R, class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, R, std::decay_t<BoundArgs>...>
    bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}
