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
struct BoundArgument {
    static constexpr int position { 0 };
    template<class...>
    using type = TDi&;
    template<class...>
    using const_type = TDi const&;
};

template<class TDi>
struct BoundArgument<TDi, true, false, false> {
    static constexpr int position { 0 };
    template<class...>
    using type = typename TDi::type&;
    template<class...>
    using const_type = typename TDi::type&;
};

template<class TDi>
struct BoundArgument<TDi, false, true, false> {
    static constexpr int position { 0 };
    template<class... U>
    using type = std::invoke_result_t<TDi&, U&&...>;
    template<class... U>
    using const_type = std::invoke_result_t<TDi const&, U&&...>;
};

template<class TDi>
struct BoundArgument<TDi, false, false, true> {
    static constexpr int position { std::is_placeholder_v<TDi> };
    template<class... U>
    using type = std::tuple_element_t<position - 1, std::tuple<U...>>;
    template<class... U>
    using const_type = std::tuple_element_t<position - 1, std::tuple<U...>>;
};

template<class FD, class R, class... BoundArgs>
struct Bind {
    FD fd;
    std::tuple<BoundArgs...> bound_args;
private:
    template<class cvTDi, class... U>
    static constexpr decltype(auto) transform_args(cvTDi& tdi, U&&... u) {
        using TDi = std::remove_cvref_t<cvTDi>;
        if constexpr (is_reference_wrapper_v<TDi>)
            return tdi.get();
        else if constexpr (std::is_bind_expression_v<TDi>)
            return tdi(std::forward<U>(u)...);
        else if constexpr ((std::is_placeholder_v<TDi>) > 0) {
            constexpr std::size_t position = std::is_placeholder_v<TDi>;
            return std::get<position - 1>(std::forward_as_tuple(std::forward<U>(u)...));
        } else
            return tdi;
    }

    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx>
    static constexpr decltype(auto) call(cvFD& fd,
        /* cv std::tuple<TDi...>& */ BoundArgTpl& bound_args, std::index_sequence<idx...>,
        UnBoundArgs&&... unbound_args
    ) {
        if constexpr (std::is_same_v<R, void(void)>)
            return (invoke)(fd, transform_args(std::get<idx>(bound_args),
                std::forward<UnBoundArgs>(unbound_args)...)...);
        else
            return (invoke<R>)(fd, transform_args(std::get<idx>(bound_args),
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
        return (call)(fd, bound_args, std::index_sequence_for<BoundArgs...>{},
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
        return (call)(fd, bound_args, std::index_sequence_for<BoundArgs...>{},
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
