#pragma once

#include <type_traits>
#include <utility>

#include "is_reference_wrapper.hpp"

template<bool /* std::is_member_pointer */>
struct InvokeSelector {
    template<class Base, class T, class Derived, class... Args>
    static constexpr auto call(T Base::*pmf, Derived&& ref, Args&&... args)
        -> std::enable_if_t<std::is_function_v<T> &&
                            std::is_base_of_v<Base, std::decay_t<Derived>>,
    decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>
    { return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...); }

    template<class Base, class T, class RefWrap, class... Args>
    static constexpr auto call(T Base::*pmf, RefWrap&& ref, Args&&... args)
        -> std::enable_if_t<std::is_function_v<T> &&
                            is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype((ref.get().*pmf)(std::forward<Args>(args)...))>
    { return (ref.get().*pmf)(std::forward<Args>(args)...); }

    template<class Base, class T, class Pointer, class... Args>
    static constexpr auto call(T Base::*pmf, Pointer&& ptr, Args&&... args)
        -> std::enable_if_t<std::is_function_v<T> &&
                            !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                            !std::is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>
    { return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...); }

    template<class Base, class T, class Derived>
    static constexpr auto call(T Base::*pmd, Derived&& ref)
        -> std::enable_if_t<!std::is_function_v<T> &&
                            std::is_base_of_v<Base, std::decay_t<Derived>>,
    decltype(std::forward<Derived>(ref).*pmd)>
    { return std::forward<Derived>(ref).*pmd; }

    template<class Base, class T, class RefWrap>
    static constexpr auto call(T Base::*pmd, RefWrap&& ref)
        -> std::enable_if_t<!std::is_function_v<T> &&
                            is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype(ref.get().*pmd)>
    { return ref.get().*pmd; }

    template<class Base, class T, class Pointer>
    static constexpr auto call(T Base::*pmd, Pointer&& ptr)
        -> std::enable_if_t<!std::is_function_v<T> &&
                            !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                            !std::is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype((*std::forward<Pointer>(ptr)).*pmd)>
    { return (*std::forward<Pointer>(ptr)).*pmd; }
};

template<>
struct InvokeSelector<false> {
    template<class F, class... Args>
    static constexpr auto call(F&& f, Args&&... args)
        -> decltype(std::forward<F>(f)(std::forward<Args>(args)...))
    { return std::forward<F>(f)(std::forward<Args>(args)...); }
};

template<class F, class... ArgTypes>
constexpr auto invoke(F&& f, ArgTypes&&... args)
    -> decltype(
        InvokeSelector<std::is_member_pointer_v<std::decay_t<F>>>::call(
            std::forward<F>(f), std::forward<ArgTypes>(args)...)
        )
{
    return InvokeSelector<std::is_member_pointer_v<std::decay_t<F>>>::call(
        std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

template<class R, class F, class... ArgTypes,
    std::enable_if_t<!std::is_void_v<R>, int> = 0,
    std::enable_if_t<
        std::is_convertible_v<
            decltype(
                InvokeSelector<std::is_member_pointer_v<std::decay_t<F>>>::call(
                    std::declval<F>(), std::declval<ArgTypes>()...)),
            R
        >,
        int> = 0
>
constexpr R invoke(F&& f, ArgTypes&&... args) {
    return InvokeSelector<std::is_member_pointer_v<std::decay_t<F>>>::call(
        std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

template<class R, class F, class... ArgTypes,
    std::enable_if_t<std::is_void_v<R>, int> = 0,
    decltype((void)InvokeSelector<std::is_member_pointer_v<std::decay_t<F>>>::call(
        std::declval<F>(), std::declval<ArgTypes>()...), 0) = 0
>
constexpr void invoke(F&& f, ArgTypes&&... args) {
    InvokeSelector<std::is_member_pointer_v<F>>::call(
        std::forward<F>(f), std::forward<ArgTypes>(args)...);
}
