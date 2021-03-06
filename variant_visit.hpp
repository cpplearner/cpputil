#pragma once

#include <variant>
#include <type_traits>
#include <utility>

#include "invoke.hpp"

template<class Visitor, class Variant>
constexpr decltype(auto) variant_visit(Visitor&& vis, Variant&& var) {
    constexpr auto size = std::variant_size_v<std::decay_t<Variant>>;
    using R = std::invoke_result_t<Visitor,
        decltype(std::get<0>(std::declval<Variant>()))>;
    auto f = [&vis, &var, index = var.index()](auto&& rec, auto i) -> R {
        // [[assert: &rec == &f]];
        static_assert(0 <= i && i <= size);
        if constexpr (i == size)
            throw std::bad_variant_access();
        else {
            static_assert(std::is_same_v<R, std::invoke_result_t<Visitor,
                decltype(std::get<i>(std::declval<Variant>()))>>,
                "visitor must return the same type for all alternatives!");
            if (i == index)
                return (invoke)(static_cast<Visitor&&>(vis),
                    std::get<i>(static_cast<Variant&&>(var)));
            else
                return rec(rec, std::integral_constant<std::size_t, i + 1>{});
        }
    };
    return f(f, std::integral_constant<std::size_t, 0>{});
}

template<class T, std::size_t> struct wrapper { T elem; };
template<class... Args> struct tuple : Args... {};

template<class I, class Visitor, class... Ts, std::size_t... Is>
constexpr decltype(auto) visit_impl(I, Visitor&& vis,
    tuple<wrapper<Ts, Is>...> tpl)
{
    return static_cast<Visitor&&>(vis)(
        static_cast<wrapper<Ts, Is>&>(tpl).elem...);
}

template<class I, class Visitor, class... Ts, std::size_t... Is,
    class Variant, class... Variants>
constexpr decltype(auto) visit_impl(I, Visitor&& vis,
    tuple<wrapper<Ts, Is>...> tpl, Variant&& var, Variants&&... vars)
{
    auto f = [&vis, &tpl, &vars...](auto&& cur) -> decltype(auto) {
        using T = decltype(cur);
        return (visit_impl)(std::integral_constant<std::size_t, I::value + 1>{},
            static_cast<Visitor&&>(vis),
            tuple<wrapper<Ts, Is>..., wrapper<T, I::value>>{
                static_cast<wrapper<Ts, Is>&>(tpl)..., { static_cast<T>(cur) }
            },
            static_cast<Variants&&>(vars)...);
    };
    return (variant_visit)(f, static_cast<Variant&&>(var));
}

template<class Visitor, class Variant, class... Variants>
constexpr decltype(auto) variant_visit(Visitor&& vis, Variant&& var,
    Variants&&... vars)
{
    return (visit_impl)(std::integral_constant<std::size_t, 0>{}, 
        static_cast<Visitor&&>(vis), tuple<>{},
        static_cast<Variant&&>(var), static_cast<Variants&&>(vars)...);
}
