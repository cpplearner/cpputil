#pragma once

/*
    synopsis

    template<class>
    struct tag; // not defined

    template<std::size_t I, class T>
    struct type_to_index;

    template<class T, class, class...>
    struct meta_index_of_impl; // for internal use

    template<class T, class... Args>
    struct meta_index_of;
*/
#include <utility>

template<class> struct tag;

template<std::size_t I, class T>
struct type_to_index {
    std::integral_constant<std::size_t, I> operator()(T);
};

template<class, class, class...> struct meta_index_of_impl;

template<class T, std::size_t... Idx, class... Args>
struct meta_index_of_impl<T, std::index_sequence<Idx...>, Args...>
    : type_to_index<Idx, tag<Args>**>...
{
    using type_to_index<Idx, tag<Args>**>::operator()...;
    static const std::size_t value;
};
template<class T, std::size_t... Idx, class... Args>
inline constexpr std::size_t
    meta_index_of_impl<T, std::index_sequence<Idx...>, Args...>::value =
        decltype(meta_index_of_impl{}(static_cast<tag<T>**>(nullptr)))::value;

// Given a type T followed by a sequence of types, the trait meta_index_of
// computes the 0-based index of T in the sequence. It is required that
// T is in the sequence and the sequence does not contain duplicated types.
template<class T, class... Args>
struct meta_index_of
    : std::integral_constant<std::size_t,
        meta_index_of_impl<T, std::index_sequence_for<Args...>, Args...>::value
> {};
