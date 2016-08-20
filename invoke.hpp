#pragma once

#include <type_traits>
#include <utility>

#include "is_reference_wrapper.hpp"

template<class Base, class T, class Derived, class... Args>
auto INVOKE(T Base::*pmf, Derived&& ref, Args&&... args)
    -> std::enable_if_t<std::is_function_v<T> &&
                        std::is_base_of_v<Base, std::decay_t<Derived>>,
    decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>
{
    return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
}

template<class Base, class T, class RefWrap, class... Args>
auto INVOKE(T Base::*pmf, RefWrap&& ref, Args&&... args)
    -> std::enable_if_t<std::is_function_v<T> &&
                        is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype((ref.get().*pmf)(std::forward<Args>(args)...))>
{
    return (ref.get().*pmf)(std::forward<Args>(args)...);
}

template<class Base, class T, class Pointer, class... Args>
auto INVOKE(T Base::*pmf, Pointer&& ptr, Args&&... args)
    -> std::enable_if_t<std::is_function_v<T> &&
                        !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                        !std::is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>
{
    return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
}

template<class Base, class T, class Derived>
auto INVOKE(T Base::*pmd, Derived&& ref)
    -> std::enable_if_t<!std::is_function_v<T> &&
                        std::is_base_of_v<Base, std::decay_t<Derived>>,
    decltype(std::forward<Derived>(ref).*pmd)>
{
    return std::forward<Derived>(ref).*pmd;
}

template<class Base, class T, class RefWrap>
auto INVOKE(T Base::*pmd, RefWrap&& ref)
    -> std::enable_if_t<!std::is_function_v<T> &&
                        is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype(ref.get().*pmd)>
{
    return ref.get().*pmd;
}

template<class Base, class T, class Pointer>
auto INVOKE(T Base::*pmd, Pointer&& ptr)
    -> std::enable_if_t<!std::is_function_v<T> &&
                        !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                        !std::is_base_of_v<Base, std::decay_t<Pointer>>,
        decltype((*std::forward<Pointer>(ptr)).*pmd)>
{
    return (*std::forward<Pointer>(ptr)).*pmd;
}

template<class F, class... Args>
auto INVOKE(F&& f, Args&&... args)
    -> std::enable_if_t<!std::is_member_pointer_v<std::decay_t<F>>,
        decltype(std::forward<F>(f)(std::forward<Args>(args)...))>
{
    return std::forward<F>(f)(std::forward<Args>(args)...);
}

template<class F, class... ArgTypes>
auto invoke(F&& f, ArgTypes&&... args)
    -> decltype((INVOKE)(std::forward<F>(f), std::forward<ArgTypes>(args)...))
{
    return (INVOKE)(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

template<class R, class F, class... ArgTypes,
    std::enable_if_t<!std::is_void_v<R>, int> = 0,
    std::enable_if_t<
        std::is_convertible_v<
            decltype((INVOKE)(std::declval<F>(), std::declval<ArgTypes>()...)),
            R
        >,
        int> = 0
>
R invoke(F&& f, ArgTypes&&... args) {
    return (INVOKE)(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

template<class R, class F, class... ArgTypes,
    std::enable_if_t<std::is_void_v<R>, int> = 0,
    decltype((void)(INVOKE)(std::declval<F>(), std::declval<ArgTypes>()...),0) = 0
>
void invoke(F&& f, ArgTypes&&... args) {
    (INVOKE)(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}
