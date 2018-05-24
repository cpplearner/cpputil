#pragma once

#include <type_traits>
#include <utility>

#include "is_reference_wrapper.hpp"

template<class>
struct invoke_impl {
    template<class F, class... Args>
    static constexpr auto call(F&& f, Args&&... args)
        noexcept(noexcept(static_cast<F&&>(f)(static_cast<Args&&>(args)...)))
        -> decltype(static_cast<F&&>(f)(static_cast<Args&&>(args)...))
    { return static_cast<F&&>(f)(static_cast<Args&&>(args)...); }
};

template<class A, class B>
struct invoke_impl<A B::*> {
    template<class T,
        class = std::enable_if_t<std::is_base_of_v<B, std::decay_t<T>>>
    >
    static constexpr auto get(T&& t)
            noexcept(noexcept(static_cast<T&&>(t)))
            -> decltype(static_cast<T&&>(t))
    { return static_cast<T&&>(t); }

    template<class T,
        class = std::enable_if_t<is_reference_wrapper_v<std::decay_t<T>>>
    >
    static constexpr auto get(T&& t)
            noexcept(noexcept(t.get()))
            -> decltype(t.get())
    { return t.get(); }

    template<class T,
        class = std::enable_if_t<!std::is_base_of_v<B, std::decay_t<T>>>,
        class = std::enable_if_t<!is_reference_wrapper_v<std::decay_t<T>>>
    >
    static constexpr auto get(T&& t)
        noexcept(noexcept(*static_cast<T&&>(t)))
        -> decltype(*static_cast<T&&>(t))
    { return *static_cast<T&&>(t); }

    template<class AA, class T, class... Args,
        class = std::enable_if_t<std::is_function_v<AA>>
    >
    static constexpr auto call(AA B::*pmf, T&& t, Args&&... args)
        noexcept(noexcept(((get)(static_cast<T&&>(t)).*pmf)(static_cast<Args&&>(args)...)))
        -> decltype(((get)(static_cast<T&&>(t)).*pmf)(static_cast<Args&&>(args)...))
    { return ((get)(static_cast<T&&>(t)).*pmf)(static_cast<Args&&>(args)...); }

    template<class T>
    static constexpr auto call(A B::*pmd, T&& t)
        noexcept(noexcept((get)(static_cast<T&&>(t)).*pmd))
        -> decltype((get)(static_cast<T&&>(t)).*pmd)
    { return (get)(static_cast<T&&>(t)).*pmd; }
};

template<class F, class... Args>
constexpr auto invoke(F&& f, Args&&... args)
    noexcept(noexcept(invoke_impl<std::decay_t<F>>::call(
            static_cast<F&&>(f), static_cast<Args&&>(args)...)))
    -> decltype(invoke_impl<std::decay_t<F>>::call(
            static_cast<F&&>(f), static_cast<Args&&>(args)...))
{
    return invoke_impl<std::decay_t<F>>::call(
        static_cast<F&&>(f), static_cast<Args&&>(args)...);
}

template<class T>
void implicit_cast(T) noexcept;

template<class R, class F, class... Args,
    class = std::enable_if_t<!std::is_void_v<R>>,
    class = decltype((implicit_cast<R>)(
        invoke_impl<std::decay_t<F>>::call(
            std::declval<F>(), std::declval<Args>()...)))
>
constexpr R invoke(F&& f, Args&&... args)
    noexcept(noexcept((implicit_cast<R>)(
        invoke_impl<std::decay_t<F>>::call(
            static_cast<F&&>(f), static_cast<Args&&>(args)...))))
{
    return invoke_impl<std::decay_t<F>>::call(
        static_cast<F&&>(f), static_cast<Args&&>(args)...);
}

template<class R, class F, class... Args,
    class = std::enable_if_t<std::is_void_v<R>>,
    class = decltype(invoke_impl<std::decay_t<F>>::call(
        std::declval<F>(), std::declval<Args>()...))
>
constexpr void invoke(F&& f, Args&&... args)
    noexcept(noexcept(invoke_impl<std::decay_t<F>>::call(
        static_cast<F&&>(f), static_cast<Args&&>(args)...)))
{
    invoke_impl<std::decay_t<F>>::call(
        static_cast<F&&>(f), static_cast<Args&&>(args)...);
}
