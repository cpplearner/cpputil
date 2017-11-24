#pragma once

#include <variant>
#include <type_traits>
#include <utility>

#include "invoke.hpp"

template<class Visitor, class Variant, class IndexSeq>
struct visit_return_type;

template<class Visitor, class Variant, std::size_t... I>
struct visit_return_type<Visitor, Variant, std::index_sequence<I...>> {
    using type = std::common_type_t<std::result_of_t<
        Visitor(decltype(std::get<I>(std::declval<Variant>())))>...>;
};

template<class Visitor, class Variant>
constexpr decltype(auto) variant_visit(Visitor&& vis, Variant&& var) {
#define size std::variant_size_v<std::decay_t<Variant>>
    using R = typename visit_return_type<Visitor&&, Variant&&,
        std::make_index_sequence<size>>::type;
    auto f = [&vis, &var, index = var.index()](auto&& self, auto i) -> R {
        // [[assert: &self == &f]]
        // [[assert: 0 <= i && i <= size]]
        if constexpr (i == size)
            throw std::bad_variant_access();
        else if (i == index)
            return (invoke)(static_cast<Visitor&&>(vis),
                std::get<i>(static_cast<Variant&&>(var)));
        else
            return self(self, std::integral_constant<std::size_t, i + 1>{});
        };
    return f(f, std::integral_constant<std::size_t, 0>{});
#undef size
}

template<class T> struct wrapper { T elem; };
template<class... Args> struct tuple : Args... {};

template<class Visitor, class... Ts>
constexpr decltype(auto) visit_impl(Visitor&& vis, tuple<wrapper<Ts>...> tpl) {
    return static_cast<Visitor&&>(vis)(static_cast<wrapper<Ts>&>(tpl).elem...);
}

template<class Visitor, class... Ts, class Variant, class... Variants>
constexpr decltype(auto) visit_impl(Visitor&& vis, tuple<wrapper<Ts>...> tpl,
    Variant&& var, Variants&&... vars)
{
    auto f = [&vis, &tpl, &vars...](auto&& cur) -> decltype(auto) {
        using T = decltype(cur);
        return (visit_impl)(static_cast<Visitor&&>(vis),
            tuple<wrapper<Ts>..., wrapper<T>>{
                static_cast<wrapper<Ts>&>(tpl)..., { static_cast<T>(cur) }
            },
            static_cast<Variants&&>(vars)...);
    };
    return (variant_visit)(f, static_cast<Variant&&>(var));
}

template<class Visitor, class Variant, class... Variants>
constexpr decltype(auto) variant_visit(Visitor&& vis, Variant&& var,
    Variants&&... vars)
{
    return (visit_impl)(static_cast<Visitor&&>(vis), tuple<>{},
        static_cast<Variant&&>(var), static_cast<Variants&&>(vars)...);
}