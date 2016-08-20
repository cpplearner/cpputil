#pragma once

#include <functional>

// A type can specialize this template if it wants itself to be treated like
// a std::reference_wrapper. E.g. a replacement of std::reference_wrapper that
// supports constexpr operations may want to specialize this template.
template <class T>
struct is_reference_wrapper : std::false_type {};
template <class U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};
template <class T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
