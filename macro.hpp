#define FWDEXPR(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
#define RETURNS(...) noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) { return __VA_ARGS__; }
#define OVERLOAD_SET(f) [](auto&&... args) RETURNS(f(FWDEXPR(args)...))
