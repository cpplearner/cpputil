#pragma once

/*
    synopsis

    template<class T, class = void>
    union unsafe_optional_imp; // for internal use

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
        unsafe_optional(const unsafe_optional&) = delete;
        unsafe_optional(unsafe_optional&&); // always throws
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
        unsafe_optional& operator=(const unsafe_optional&) = delete;
        unsafe_optional& operator=(unsafe_optional&&) = delete;
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

template<class T>
class unsafe_optional;

template<class T, class = void>
union unsafe_optional_imp {
    friend unsafe_optional<T>;
private:
    unsigned char uninitialized;
    T val;
private:
    constexpr unsafe_optional_imp() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_imp(std::in_place_t, Args&&... args)
        : val(std::forward<Args>(args)...) {}
    ~unsafe_optional_imp() = default;
};

// The partial specialization is the same as the primary template except
// that the destructor is explicitly defined.
template<class T>
union unsafe_optional_imp<T,
    std::enable_if_t<!std::is_trivially_destructible<T>{}>
> {
    friend unsafe_optional<T>;
private:
    unsigned char uninitialized;
    T val;
private:
    constexpr unsafe_optional_imp() noexcept : uninitialized() {}
    template<class... Args>
    constexpr explicit unsafe_optional_imp(std::in_place_t, Args&&... args)
        : val(std::forward<Args>(args)...) {}
    ~unsafe_optional_imp() {}
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
// unsafe_optional<T> is not copy constructible.
// unsafe_optional<T> is move constructible but is not MoveConstructible.
// This is because its move constructor always throws.
// Move a unsafe_optional<T> only when copy elision is guaranteed to happen.
// unsafe_optional<T> is not copy/move assignable.
// Overloaded operators such as operator*, operator-> and operator=
// are not provided; instead, all functions are named, to avoid accidental use.
// Use a unsafe_optional only when the initialization state is known outside
// or the use only involves constant expressions.
template<class T>
class unsafe_optional {
    static_assert(sizeof(unsafe_optional_imp<T>) == sizeof(T),
        "The wrapper has unintended space overhead.");
    static_assert(alignof(unsafe_optional_imp<T>) == alignof(T),
        "The wrapper has a different alignment from the wrapped type.");
    static_assert(!std::is_reference<T>{}, "T cannot be a reference type.");
     // unsafe_optional<unsafe_optional> is quite useless.
    static_assert(!is_unsafe_optional<std::remove_cv_t<T>>{},
        "T cannot be a unsafe_optional.");
    // unsafe_optional<std::in_place_t> and unsafe_optional<std::in_place_t>
    // may be tricky to use. They might be usable though....
private:
    unsafe_optional_imp<T> data;
public:
    using value_type = T;

    constexpr unsafe_optional() noexcept = default;
    constexpr unsafe_optional(std::nullopt_t) noexcept : unsafe_optional() {}
    unsafe_optional(const unsafe_optional&) = delete;
    [[noreturn]] unsafe_optional(unsafe_optional&&) {
        throw std::runtime_error("unsafe_optional move constructor called");
    }
    constexpr unsafe_optional(const T& v) : data(std::in_place, v) {}
    constexpr unsafe_optional(T&& v) : data(std::in_place, std::move(v)) {}
    template<class... Args>
    constexpr explicit unsafe_optional(std::in_place_t, Args&&... args)
        : data(std::in_place, std::forward<Args>(args)...) {}
    template <class U, class... Args,
        std::enable_if_t<
            std::is_constructible<T,std::initializer_list<U>&,Args&&...>{},
            int> = 0
    >
    constexpr explicit unsafe_optional(
        std::in_place_t, std::initializer_list<U> il, Args&&... args
    )
        : data(std::in_place, il, std::forward<Args>(args)...) {}

    ~unsafe_optional() = default;

    unsafe_optional& operator=(unsafe_optional const&) = delete;
    unsafe_optional& operator=(unsafe_optional&&) = delete;

    template<class... Args>
    void emplace(Args&&... args) {
        ::new(static_cast<void*>(std::addressof(data.val)))
            T(std::forward<Args>(args)...);
    }
    template<class U, class... Args>
    void emplace(std::initializer_list<U> il, Args&&... args) {
        ::new(static_cast<void*>(std::addressof(data.val)))
            T(il, std::forward<Args>(args)...);
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
constexpr unsafe_optional<std::decay_t<T>> make_unsafe_optional(T&& v) {
    return unsafe_optional<std::decay_t<T>>(std::forward<T>(v));
}

template<class T, class...Args>
constexpr unsafe_optional<T> make_unsafe_optional(Args&&... args) {
    return unsafe_optional<T>(std::in_place, std::forward<Args>(args)...);
}

template<class T, class U, class... Args>
constexpr unsafe_optional<T> make_unsafe_optional(
    std::initializer_list<U> il, Args&&... args
) {
    return unsafe_optional<T>(std::in_place, il, std::forward<Args>(args)...);
}
