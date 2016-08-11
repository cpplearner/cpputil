#pragma once

#include <functional>
#include <type_traits>
#include <experimental/tuple>

template<class T, class UnBoundArgTpl>
constexpr T& get_final_args(std::reference_wrapper<T> tid, UnBoundArgTpl&& unbound_args)
{
    return tid.get();
}

template<class cvTiD, class UnBoundArgTpl,
         int idx = std::is_placeholder<std::remove_cv_t<cvTiD>>{},
         std::enable_if_t<(idx > 0)>* = nullptr>
constexpr decltype(auto) get_final_args(cvTiD& tid, UnBoundArgTpl&& unbound_args)
{
    return std::get<idx - 1>(unbound_args);
}

template<class cvTiD, class UnBoundArgTpl,
         std::enable_if_t<std::is_placeholder<std::remove_cv_t<cvTiD>>{} <= 0>* = nullptr>
constexpr cvTiD& get_final_args(cvTiD& tid, UnBoundArgTpl&& unbound_args)
{
    return tid;
}

template<class F, class BoundArgTpl, std::size_t... idx, class UnBoundArgTpl>
decltype(auto) bind_apply_unwrap_bound_args(F& f, BoundArgTpl&& bound_args, std::index_sequence<idx...>, UnBoundArgTpl&& unbound_args)
{
    return f(get_final_args(std::get<idx>(bound_args), static_cast<UnBoundArgTpl&&>(unbound_args))...);
}

template<class FD, class ArgTpl, class R = void(void)>
struct Bind {
    FD fd;
    ArgTpl bound_args;
private:
    static constexpr std::size_t arity = std::tuple_size<ArgTpl>{};
public:
    template<class... UnBoundArgs>
    constexpr auto operator()(UnBoundArgs&&... unbound_args)
    {
        return bind_apply_unwrap_bound_args(fd, bound_args, std::make_index_sequence<arity>{}, std::forward_as_tuple(std::forward<UnBoundArgs>(unbound_args)...));
    }
    
    template<class... UnBoundArgs>
    constexpr auto operator()(UnBoundArgs&&... unbound_args) const
    {
        return bind_apply_unwrap_bound_args(fd, bound_args, std::make_index_sequence<arity>{}, std::forward_as_tuple(std::forward<UnBoundArgs>(unbound_args)...));
    }
};

template<class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, std::tuple<std::decay_t<BoundArgs>...>>
  bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}

template<class R, class F, class... BoundArgs>
constexpr Bind<std::decay_t<F>, std::tuple<std::decay_t<BoundArgs>...>, R>
  bind(F&& f, BoundArgs&&... bound_args)
{
    return { std::forward<F>(f), { std::forward<BoundArgs>(bound_args)... } };
}
