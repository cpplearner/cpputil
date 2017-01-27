#pragma once

/*
    synopsis

    struct user_provided_tag {
        explicit user_provided_tag() = default;
    };
    constexpr user_provided_tag user_provided_t{};

    template<bool cond, class T> struct default_copy_ctor_if;
    template<bool cond, class T> struct default_move_ctor_if;
    template<bool cond, class T> struct default_copy_assign_if;
    template<bool cond, class T> struct default_move_assign_if;
    template<bool cond, class T> struct delete_copy_ctor_if;
    template<bool cond, class T> struct delete_move_ctor_if;
    template<bool cond, class T> struct delete_copy_assign_if;
    template<bool cond, class T> struct delete_move_assign_if;
*/
struct user_provided_tag {
    explicit user_provided_tag() = default;
};
constexpr user_provided_tag user_provided_t{};

// All the following class templates inherit from the template parameter T and
// introduce T's constructors and assignment operators into their own scope
// via using declaration.
// All their special member functions are defaulted except for the one
// corresponding to the template name.
// For each of default_*_if, the corresponding special member function is
// defaulted if the first template argument is not false, otherwise it is
// user-provided and calls T's constructor (if the special member function is
// a constructor) or T's member function named assign (if the special member
// function is operator=), with user_provided_t being the first function
// argument and the argument to the special member function being the second.
// For each of delete_*_if, the corresponding special member function is
// deleted if the first template argument is true, otherwise it is defaulted.
// Note that a defaulted function might still be implicitly deleted.
template<bool cond, class T>
struct default_copy_ctor_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct default_copy_ctor_if<false, T> : T {
    using T::T;
    using T::operator=;
    default_copy_ctor_if() = default;
    constexpr default_copy_ctor_if(const default_copy_ctor_if& that)
        : T(user_provided_t, that) {}
    default_copy_ctor_if(default_copy_ctor_if&&) = default;
    default_copy_ctor_if& operator=(const default_copy_ctor_if&) = default;
    default_copy_ctor_if& operator=(default_copy_ctor_if&&) = default;
};

template<bool cond, class T>
struct default_move_ctor_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct default_move_ctor_if<false, T> : T {
    using T::T;
    using T::operator=;
    default_move_ctor_if() = default;
    default_move_ctor_if(const default_move_ctor_if&) = default;
    constexpr default_move_ctor_if(default_move_ctor_if&& that)
        : T(user_provided_t, static_cast<decltype(that)>(that)) {}
    default_move_ctor_if& operator=(const default_move_ctor_if&) = default;
    default_move_ctor_if& operator=(default_move_ctor_if&&) = default;
};

template<bool cond, class T>
struct default_copy_assign_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct default_copy_assign_if<false, T> : T {
    using T::T;
    using T::operator=;
    default_copy_assign_if() = default;
    default_copy_assign_if(const default_copy_assign_if&) = default;
    default_copy_assign_if(default_copy_assign_if&&) = default;
    constexpr auto& operator=(const default_copy_assign_if& that) {
        T::assign(user_provided_t, that);
        return *this;
    }
    default_copy_assign_if& operator=(default_copy_assign_if&&) = default;
};

template<bool cond, class T>
struct default_move_assign_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct default_move_assign_if<false, T> : T {
    using T::T;
    using T::operator=;
    default_move_assign_if() = default;
    default_move_assign_if(const default_move_assign_if&) = default;
    default_move_assign_if(default_move_assign_if&&) = default;
    default_move_assign_if& operator=(const default_move_assign_if&) = default;
    constexpr auto& operator=(default_move_assign_if&& that) {
        T::assign(user_provided_t, static_cast<decltype(that)>(that));
        return *this;
    }
};

template<bool cond, class T>
struct delete_copy_ctor_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct delete_copy_ctor_if<true, T> : T {
    using T::T;
    using T::operator=;
    delete_copy_ctor_if() = default;
    delete_copy_ctor_if(const delete_copy_ctor_if&) = delete;
    delete_copy_ctor_if(delete_copy_ctor_if&&) = default;
    delete_copy_ctor_if& operator=(const delete_copy_ctor_if&) = default;
    delete_copy_ctor_if& operator=(delete_copy_ctor_if&&) = default;
};

template<bool cond, class T>
struct delete_move_ctor_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct delete_move_ctor_if<true, T> : T {
    using T::T;
    using T::operator=;
    delete_move_ctor_if() = default;
    delete_move_ctor_if(const delete_move_ctor_if&) = default;
    delete_move_ctor_if(delete_move_ctor_if&&) = delete;
    delete_move_ctor_if& operator=(const delete_move_ctor_if&) = default;
    delete_move_ctor_if& operator=(delete_move_ctor_if&&) = default;
};

template<bool cond, class T>
struct delete_copy_assign_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct delete_copy_assign_if<true, T> : T {
    using T::T;
    using T::operator=;
    delete_copy_assign_if() = default;
    delete_copy_assign_if(const delete_copy_assign_if&) = default;
    delete_copy_assign_if(delete_copy_assign_if&&) = default;
    delete_copy_assign_if& operator=(const delete_copy_assign_if&) = delete;
    delete_copy_assign_if& operator=(delete_copy_assign_if&&) = default;
};

template<bool cond, class T>
struct delete_move_assign_if : T {
    using T::T;
    using T::operator=;
};
template<class T>
struct delete_move_assign_if<true, T> : T {
    using T::T;
    using T::operator=;
    delete_move_assign_if() = default;
    delete_move_assign_if(const delete_move_assign_if&) = default;
    delete_move_assign_if(delete_move_assign_if&&) = default;
    delete_move_assign_if& operator=(const delete_move_assign_if&) = default;
    delete_move_assign_if& operator=(delete_move_assign_if&&) = delete;
};
