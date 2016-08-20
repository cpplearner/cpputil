#pragma once

#include <functional>
#include <type_traits>
#include <tuple>

#include "invoke.hpp"

template<class T, class UnBoundArgTpl>
constexpr T& get_final_args(std::reference_wrapper<T> tid, UnBoundArgTpl&& unbound_args) {
    return tid.get();
}

template<class cvTiD, class UnBoundArgTpl,
         std::enable_if_t<std::is_bind_expression_v<std::remove_cv_t<cvTiD>>>* = nullptr>
constexpr decltype(auto) get_final_args(cvTiD& tid, UnBoundArgTpl&& unbound_args) {
     return std::apply(tid, std::move(unbound_args));
}
             
template<class cvTiD, class UnBoundArgTpl,
         int idx = std::is_placeholder_v<std::remove_cv_t<cvTiD>>,
         std::enable_if_t<(idx > 0)>* = nullptr>
constexpr decltype(auto) get_final_args(cvTiD& tid, UnBoundArgTpl&& unbound_args)
{
    return std::get<idx - 1>(unbound_args);
}

template<class cvTiD, class UnBoundArgTpl,
         std::enable_if_t<!std::is_bind_expression_v<std::remove_cv_t<cvTiD>> &&
                          std::is_placeholder_v<std::remove_cv_t<cvTiD>> <= 0>* = nullptr>
constexpr cvTiD& get_final_args(cvTiD& tid, UnBoundArgTpl&& unbound_args)
{
    return tid;
}

template<class FD, class R, class... BoundArgs>
struct Bind {
    FD fd;
    std::tuple<std::decay_t<BoundArgs>...> bound_args;
private:
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<std::is_same_v<T,void(void)>>* = nullptr>
    static constexpr decltype(auto) unwrap_bound_args(cvFD& fd,
                         /* cv std::tuple<TiD...>& */ BoundArgTpl& bound_args,
                                                      std::index_sequence<idx...>,
                          /* std::tuple<Vi&&...>&& */ std::tuple<UnBoundArgs...>&& unbound_args
                                              ) {
        return (invoke)(fd,(get_final_args)(std::get<idx>(bound_args), std::move(unbound_args))...);
    }
    template<class cvFD, class BoundArgTpl, class... UnBoundArgs, std::size_t... idx,
             class T = R, std::enable_if_t<!std::is_same_v<T,void(void)>>* = nullptr>
    static constexpr T unwrap_bound_args(cvFD& fd,
            /* cv std::tuple<TiD...>& */ BoundArgTpl& bound_args,
                                         std::index_sequence<idx...>,
             /* std::tuple<Vi&&...>&& */ std::tuple<UnBoundArgs...>&& unbound_args
                                        ) {
        return (invoke<T>)(fd,(get_final_args)(std::get<idx>(bound_args), std::move(unbound_args))...);
    }
public:
    template<class... UnBoundArgs>
    constexpr decltype(auto) operator()(UnBoundArgs&&... unbound_args) {
        return unwrap_bound_args(fd, bound_args,
                                 std::index_sequence_for<BoundArgs...>{},
                                 std::forward_as_tuple(std::forward<UnBoundArgs>(unbound_args)...));
    }
    
    template<class... UnBoundArgs>
    constexpr decltype(auto) operator()(UnBoundArgs&&... unbound_args) const {
        return unwrap_bound_args(fd, bound_args,
                                 std::index_sequence_for<BoundArgs...>{},
                                 std::forward_as_tuple(std::forward<UnBoundArgs>(unbound_args)...));
    }
};

namespace std {
    template<class FD, class R, class... BoundArgs>
    struct is_bind_expression<Bind<FD, R, BoundArgs...>> : true_type {};
}

template<class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, void(void), BoundArgs...>
  bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}

template<class R, class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, R, BoundArgs...>
  bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}
