#pragma once

/*
    synopsis

    template<class T, class = void>
    union unsafe_optional_data; // for internal use

    template<class T>
    class unsafe_optional_impl; // for internal use

    template<class T>
    class unsafe_optional;

    template<class T>
    constexpr unsafe_optional<std::decay_t<T>> make_unsafe_optional(T&&);

    template<class T, class...Args>
    constexpr unsafe_optional<T> make_unsafe_optional(Args&&...);

    template<class T, class U, class... Args>
    constexpr unsafe_optional<T> make_unsafe_optional(
        std::initializer_list<U>, Args&&...
    );

    template <class T>
    class unsafe_optional {
    public:
        typedef T value_type;

        // Constructors
        constexpr unsafe_optional() noexcept;
        constexpr unsafe_optional(std::nullopt_t) noexcept;
        unsafe_optional(const unsafe_optional&);
        unsafe_optional(unsafe_optional&&);
        constexpr unsafe_optional(const T&);
        constexpr unsafe_optional(T&&);
        template<class... Args>
        constexpr explicit unsafe_optional(std::in_place_t, Args&&...);
        template<class U, class... Args>
        constexpr explicit unsafe_optional(
            std::in_place_t, std::initializer_list<U>, Args&&...
        );

        // Destructor
        ~unsafe_optional();

        // Assignment
        unsafe_optional& operator=(const unsafe_optional&);
        unsafe_optional& operator=(unsafe_optional&&);
        template<class... Args> void emplace(Args&&...);
        template<class U, class... Args>
        void emplace(std::initializer_list<U>, Args&&...);

        // Observers
        constexpr T const& value() const &;
        constexpr T& value() &;
        constexpr T&& value() &&;
        constexpr const T&& value() const &&;
    };
*/
#include <new>
#include <memory>
#include <cstddef>
#include <utility>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include "smf_control.hpp"

template<class T>
class unsafe_optional;

template<class T>
class unsafe_optional_impl;

template<class T, class = void>
union unsafe_optional_data {
    friend unsafe_optional_impl<T>;
private:
    unsigned char uninitialized;
    T val;
private:
    constexpr unsafe_optional_data() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_data(std::in_place_t, Args&&... args)
        : val(static_cast<Args&&>(args)...) {}
    ~unsafe_optional_data() = default;
};

// The partial specialization is the same as the primary template except
// that the destructor is explicitly defined.
template<class T>
union unsafe_optional_data<T,
    std::enable_if_t<!std::is_trivially_destructible_v<T>>
> {
    friend unsafe_optional_impl<T>;
private:
    unsigned char uninitialized;
    T val;
private:
    constexpr unsafe_optional_data() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_data(std::in_place_t, Args&&... args)
        : val(static_cast<Args&&>(args)...) {}
    ~unsafe_optional_data() {}
};

template<class T>
struct is_unsafe_optional : std::false_type {};
template<class T>
struct is_unsafe_optional<unsafe_optional<T>> : std::true_type {};

// A unsafe_optional is similar to a std::optional, except that
// the initialization state of the contained object is not tracked.
// Compared to std::optional, unsafe_optional lacks observer functions
// such like has_value/operator bool, and it lacks value_or,
// comparison operators, and specialization of std::hash.
// unsafe_optional<T> is not copy constructible unless T is trivially
// copy constructible.
// unsafe_optional<T> is move constructible, but its move constructor always
// throws unless T is trivially move constructble.
// Move constructing unsafe_optional<T> will throw an exception, unless
// T is trivially move constructble or the move construction is elided.
// unsafe_optional<T> is not copy/move assignable unless T is trivially
// copy/move assignable.
// Overloaded operators such as operator*, operator-> and operator=
// are not provided; instead, all functions are named, to avoid accidental use.
// Use a unsafe_optional only when the initialization state is known outside
// or when the use only involves constant expressions.
template<class T>
class unsafe_optional_impl {
    unsafe_optional_data<T> data;
public:
    using value_type = T;

    constexpr unsafe_optional_impl() noexcept = default;
    constexpr unsafe_optional_impl(std::nullopt_t) noexcept
        : unsafe_optional_impl() {}
    unsafe_optional_impl(const unsafe_optional_impl&) = default;
    unsafe_optional_impl(unsafe_optional_impl&&) = default;
    [[noreturn]]
    unsafe_optional_impl(user_provided_t, unsafe_optional_impl&&) {
        throw std::runtime_error("unsafe_optional move constructor called");
    }
    constexpr unsafe_optional_impl(const T& v) : data(std::in_place, v) {}
    constexpr unsafe_optional_impl(T&& v) : data(std::in_place, std::move(v)) {}
    template<class... Args>
    constexpr explicit unsafe_optional_impl(std::in_place_t, Args&&... args)
        : data(std::in_place, static_cast<Args&&>(args)...) {}
    template <class U, class... Args,
        std::enable_if_t<
            std::is_constructible_v<T,std::initializer_list<U>&,Args&&...>,
            int> = 0
    >
    constexpr explicit unsafe_optional_impl(
        std::in_place_t, std::initializer_list<U> il, Args&&... args
    )
        : data(std::in_place, il, static_cast<Args&&>(args)...) {}

    ~unsafe_optional_impl() = default;

    unsafe_optional_impl& operator=(unsafe_optional_impl const&) = default;
    unsafe_optional_impl& operator=(unsafe_optional_impl&&) = default;

    template<class... Args>
    T& emplace(Args&&... args) {
        return *::new(static_cast<void*>(std::addressof(data.val)))
            T(static_cast<Args&&>(args)...);
    }
    template<class U, class... Args>
    T& emplace(std::initializer_list<U> il, Args&&... args) {
        return *::new(static_cast<void*>(std::addressof(data.val)))
            T(il, static_cast<Args&&>(args)...);
    }

    constexpr T const& value() const& noexcept { return data.val; }
    constexpr T& value() & noexcept { return data.val; }
    constexpr T&& value() && noexcept { return std::move(*this).data.val; }
    constexpr T const&& value() const&& noexcept {
        return std::move(*this).data.val;
    }

    void reset() noexcept { data.val.~T(); }
};

template<class T>
class unsafe_optional : public
    delete_copy_ctor_if<!std::is_trivially_copy_constructible_v<T>,
    default_move_ctor_if<std::is_trivially_copy_constructible_v<T>,
    delete_copy_assign_if<!std::is_trivially_copy_assignable_v<T>,
    delete_move_assign_if<!std::is_trivially_move_assignable_v<T>,
    unsafe_optional_impl<T>
>>>> {
    static_assert(sizeof(unsafe_optional_data<T>) == sizeof(T),
        "The wrapper has unintended space overhead.");
    static_assert(alignof(unsafe_optional_data<T>) == alignof(T),
        "The wrapper has a different alignment from the wrapped type.");
    static_assert(!std::is_reference_v<T>, "T cannot be a reference type.");
     // unsafe_optional_impl<unsafe_optional_impl> is quite useless.
    static_assert(!is_unsafe_optional_v<std::remove_cv_t<T>>,
        "T cannot be a unsafe_optional_impl.");
    // unsafe_optional<std::in_place_t> and unsafe_optional<std::nullopt_t>
    // are not disabled, but they may be a bit tricky to use.

    using base = typename unsafe_optional::delete_copy_ctor_if;
public:
    using base::base;
};

template<class T>
constexpr unsafe_optional<std::decay_t<T>> make_unsafe_optional(T&& v) {
    return unsafe_optional<std::decay_t<T>>(static_cast<T&&>(v));
}

template<class T, class...Args>
constexpr unsafe_optional<T> make_unsafe_optional(Args&&... args) {
    return unsafe_optional<T>(std::in_place, static_cast<Args&&>(args)...);
}

template<class T, class U, class... Args>
constexpr unsafe_optional<T> make_unsafe_optional(
    std::initializer_list<U> il, Args&&... args
) {
    return unsafe_optional<T>(std::in_place, il, static_cast<Args&&>(args)...);
}
