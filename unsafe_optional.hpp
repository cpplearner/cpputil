#pragma once

/*
    synopsis

    template<class T, class = void>
    union unsafe_optional_impl; // for internal use

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
        using value_type = T;

        // Constructors
        constexpr unsafe_optional() noexcept;
        constexpr unsafe_optional(std::nullopt_t) noexcept;
        unsafe_optional(const unsafe_optional&);
        unsafe_optional(unsafe_optional&&);
        unsafe_optional(const T&) = default;
        unsafe_optional(T&&) = default;
        template<class... Args>
        constexpr explicit unsafe_optional(std::in_place_t, Args&&...);
        template<class U, class... Args>
        constexpr explicit unsafe_optional(
            std::in_place_t, std::initializer_list<U>, Args&&...
        );

        // Destructor
        ~unsafe_optional();

        // Assignment
        unsafe_optional& operator=(const unsafe_optional&) = default;
        unsafe_optional& operator=(unsafe_optional&&) = default;
        template<class... Args> T& emplace(Args&&...);
        template<class U, class... Args>
        T& emplace(std::initializer_list<U>, Args&&...);

        // Observers
        constexpr T const& value() const &;
        constexpr T& value() &;
        constexpr T&& value() &&;
        constexpr const T&& value() const &&;

        // Modifiers
        void reset() noexcept;
    };
*/
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

template<class T>
class unsafe_optional;

template<class T, class = void>
union unsafe_optional_impl {
    friend unsafe_optional<T>;
private:
    unsigned char uninitialized;
    std::remove_const_t<T> val;
private:
    constexpr unsafe_optional_impl() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_impl(std::in_place_t, Args&&... args)
        : val(static_cast<Args&&>(args)...) {}
    ~unsafe_optional_impl() = default;
};

// The partial specialization is the same as the primary template except
// that the destructor is explicitly defined.
template<class T>
union unsafe_optional_impl<T,
    std::enable_if_t<!std::is_trivially_destructible_v<T>>
> {
    friend unsafe_optional<T>;
private:
    unsigned char uninitialized;
    std::remove_const_t<T> val;
private:
    constexpr unsafe_optional_impl() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_impl(std::in_place_t, Args&&... args)
        : val(static_cast<Args&&>(args)...) {}
    ~unsafe_optional_impl() {}
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
// unsafe_optional<T> is not copy/move constructible unless T is trivially
// copy/move constructible.
// unsafe_optional<T> is not copy/move assignable unless T is trivially
// copy/move assignable.
// Overloaded operators such as operator*, operator-> and operator=
// are not provided; instead, all functions are named, to avoid accidental use.
// Use a unsafe_optional only when the initialization state is known outside
// (or when the use only involves constant expressions).
template<class T>
class unsafe_optional {
    static_assert(sizeof(unsafe_optional_impl<T>) == sizeof(T),
        "The wrapper has unintended space overhead.");
    static_assert(alignof(unsafe_optional_impl<T>) == alignof(T),
        "The wrapper has a different alignment than the wrapped type.");
    static_assert(!std::is_reference_v<T>, "T cannot be a reference type.");
     // unsafe_optional<unsafe_optional> is quite useless.
    static_assert(!is_unsafe_optional<std::remove_cv_t<T>>::value,
        "T cannot be a unsafe_optional.");
    // unsafe_optional<std::in_place_t> and unsafe_optional<std::nullopt_t>
    // are allowed, but they may be a bit tricky to use.

    unsafe_optional_impl<T> data;
public:
    using value_type = T;

    constexpr unsafe_optional() noexcept = default;
    constexpr unsafe_optional(std::nullopt_t) noexcept
        : unsafe_optional() {}
    constexpr unsafe_optional(const T& v) : data(std::in_place, v) {}
    constexpr unsafe_optional(T&& v) : data(std::in_place, std::move(v)) {}
    template<class... Args>
    constexpr explicit unsafe_optional(std::in_place_t, Args&&... args)
        : data(std::in_place, static_cast<Args&&>(args)...) {}
    template <class U, class... Args,
        class=std::enable_if_t<
            std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>
    >
    constexpr explicit unsafe_optional(
        std::in_place_t, std::initializer_list<U> il, Args&&... args
    )
        : data(std::in_place, il, static_cast<Args&&>(args)...) {}

    template<class... Args>
    T& emplace(Args&&... args) {
        return *::new(static_cast<void*>(std::addressof(data.val)))
            std::remove_const_t<T>(static_cast<Args&&>(args)...);
    }
    template<class U, class... Args>
    T& emplace(std::initializer_list<U> il, Args&&... args) {
        return *::new(static_cast<void*>(std::addressof(data.val)))
            std::remove_const_t<T>(il, static_cast<Args&&>(args)...);
    }

    constexpr T const& value() const& noexcept { return data.val; }
    constexpr T& value() & noexcept { return data.val; }
    constexpr T&& value() && noexcept { return std::move(*this).data.val; }
    constexpr T const&& value() const&& noexcept {
        return std::move(*this).data.val;
    }

    void reset() noexcept { std::destroy_at(std::addressof(data.val)); }
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
