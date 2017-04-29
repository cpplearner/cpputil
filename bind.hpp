#pragma once

#include <functional>
#include <type_traits>
#include <tuple>

#include "is_reference_wrapper.hpp"
#include "invoke.hpp"

template<class TDi,
    bool = is_reference_wrapper_v<TDi>,
    bool = std::is_bind_expression_v<TDi>,
    bool = ((std::is_placeholder_v<TDi>) > 0)>
struct BoundArgument;

template<class TDi>
struct BoundArgument<TDi, true, false, false> {
    template<class cvTDi, class... U>
    static constexpr auto value(cvTDi& tdi, U&&...)
        -> typename TDi::type&
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTDi>, TDi>);
        return tdi.get();
    }
    template<class...>
    using type = typename TDi::type&;
    template<class...>
    using const_type = typename TDi::type&;
};

template<class TDi>
struct BoundArgument<TDi, false, true, false> {
    template<class cvTDi, class... U>
    static constexpr auto value(cvTDi& tdi, U&&... u)
        -> std::invoke_result_t<cvTDi&, U&&...>
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTDi>, TDi>);
        return tdi(std::forward<U>(u)...);
    }
    template<class... U>
    using type = std::invoke_result_t<TDi&, U&&...>;
    template<class... U>
    using const_type = std::invoke_result_t<TDi const&, U&&...>;
};

template<class TDi>
struct BoundArgument<TDi, false, false, true> {
    static constexpr int position { std::is_placeholder_v<TDi> - 1 };
    template<class cvTDi, class... U>
    static constexpr auto value(cvTDi& tdi, U&&... u)
        -> std::tuple_element_t<position,std::tuple<U...>>
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTDi>, TDi>);
        return std::get<position>(std::forward_as_tuple(std::forward<U>(u)...));
    }
    template<class... U>
    using type = std::tuple_element_t<position, std::tuple<U...>>;
    template<class... U>
    using const_type = std::tuple_element_t<position, std::tuple<U...>>;
};

template<class TDi, bool, bool, bool>
struct BoundArgument {
    template<class cvTDi, class... U>
    static constexpr auto value(cvTDi& tdi, U&&...)
        -> cvTDi&
    {
        static_assert(std::is_same_v<std::remove_cv_t<cvTDi>, TDi>);
        return tdi;
    }
    template<class...>
    using type = TDi&;
    template<class...>
    using const_type = TDi const&;
};

template<class FD, class R, class... BoundArgs>
struct Bind {
    FD fd;
    std::tuple<BoundArgs...> bound_args;
private:
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<std::is_same_v<T,void(void)>, int> = 0>
    static constexpr decltype(auto) unpack_bound_args(cvFD& fd,
                         /* cv std::tuple<TDi...>& */ BoundArgTpl& bound_args,
                                                      std::index_sequence<idx...>,
                                                      UnBoundArgs&&... unbound_args
                                                     ) {
        return (invoke)(fd, BoundArgument<BoundArgs>::value(std::get<idx>(bound_args),
                                                            std::forward<UnBoundArgs>(unbound_args)...)...);
    }
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<!std::is_same_v<T,void(void)>, int> = 0>
    static constexpr T unpack_bound_args(cvFD& fd,
            /* cv std::tuple<TDi...>& */ BoundArgTpl& bound_args,
                                         std::index_sequence<idx...>,
                                         UnBoundArgs&&... unbound_args
                                        ) {
        return (invoke<T>)(fd, BoundArgument<BoundArgs>::value(std::get<idx>(bound_args),
                                                               std::forward<UnBoundArgs>(unbound_args)...)...);
    }
public:
    template<class... UnBoundArgs>
    constexpr decltype(auto) operator()(UnBoundArgs&&... unbound_args) {
        static_assert(
            std::is_invocable_v<
                FD&, typename BoundArgument<BoundArgs>::template type<UnBoundArgs&&...>...
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
            std::is_invocable_v<
                FD const&, typename BoundArgument<BoundArgs>::template const_type<UnBoundArgs&&...>...
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
