#include <ranges>
#include <algorithm>
#include <iterator>

template<class F, int = (F(), 0)>
constexpr bool is_constexpr(F) { return true; }
constexpr bool is_constexpr(...) { return false; }

template<class R>
concept simple_view = std::ranges::view<R> && std::ranges::range<const R> &&
    std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
    std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

template<bool Const, class T>
using maybe_const = std::conditional_t<Const, const T, T>;

template<std::ranges::input_range V, std::ranges::forward_range Pattern>
requires std::ranges::view<V> && std::ranges::view<Pattern> &&
         std::indirectly_comparable<std::ranges::iterator_t<V>, std::ranges::iterator_t<Pattern>,
             std::ranges::equal_to> &&
         (std::ranges::forward_range<V> || (std::ranges::sized_range<V> &&
             (is_constexpr)([]{ Pattern::size(); }) && Pattern::size() <= 1))
class split_view : public std::ranges::view_interface<split_view<V, Pattern>> {
private:
    [[no_unique_address]] V base_ = V();
    [[no_unique_address]] Pattern pattern_ = Pattern();
    [[no_unique_address]]
    std::conditional_t<!std::ranges::forward_range<V>,
        std::ranges::iterator_t<V>, std::ranges::dangling> current_ = decltype(current_)();

    template<bool> struct outer_iterator;
    template<bool> struct inner_iterator;

    friend constexpr bool operator==(const inner_iterator<true>& x, std::default_sentinel_t);
    friend constexpr bool operator==(const inner_iterator<false>& x, std::default_sentinel_t);

    template<bool Const> struct outer_iterator {
    private:
        template<bool> friend struct outer_iterator;
        template<bool> friend struct inner_iterator;
        friend constexpr bool operator==(const inner_iterator<Const>& x, std::default_sentinel_t);

        using Parent = maybe_const<Const, split_view>;
        using Base = maybe_const<Const, V>;

        Parent* parent_ = nullptr;
        [[no_unique_address]]
        std::conditional_t<std::ranges::forward_range<V>,
            std::ranges::iterator_t<Base>, std::ranges::dangling> current_ = decltype(current_)();

    public:
        constexpr auto& get_current_() {
            if constexpr (std::ranges::forward_range<V>)
                return this->current_;
            else
                return parent_->current_;
        }
        constexpr auto& get_current_() const {
            if constexpr (std::ranges::forward_range<V>)
                return this->current_;
            else
                return parent_->current_;
        }
    public:
        using iterator_concept =
          std::conditional_t<std::ranges::forward_range<Base>,
            std::forward_iterator_tag, std::input_iterator_tag>;
        using iterator_category = std::input_iterator_tag;
        struct value_type : std::ranges::view_interface<value_type> {
        private:
            outer_iterator i_ = outer_iterator();
        public:
            value_type() = default;
            constexpr explicit value_type(outer_iterator i) : i_(std::move(i)) {}

            constexpr inner_iterator<Const> begin() const requires std::copyable<outer_iterator> {
                return inner_iterator<Const>{i_};
            }
            constexpr inner_iterator<Const> begin() requires (!std::copyable<outer_iterator>) {
                return inner_iterator<Const>{std::move(i_)};
            }
            constexpr std::default_sentinel_t end() const {
                return std::default_sentinel;
            }
        };
        using difference_type = std::ranges::range_difference_t<Base>;

        outer_iterator() = default;
        constexpr explicit outer_iterator(Parent& parent)
            requires (!std::ranges::forward_range<Base>)
            : parent_(std::addressof(parent)) {}
        constexpr explicit outer_iterator(Parent& parent, std::ranges::iterator_t<Base> current)
            requires std::ranges::forward_range<Base>
            : parent_(std::addressof(parent)), current_(std::move(current)) {}
        constexpr outer_iterator(outer_iterator<!Const> i)
            requires Const && std::convertible_to<std::ranges::iterator_t<V>,
                                                  std::ranges::iterator_t<Base>>
            : parent_(i.parent_), current_(std::move(i.current_)) {}

        constexpr value_type operator*() const { return value_type{*this}; }

        constexpr outer_iterator& operator++() {
            const auto end = std::ranges::end(parent_->base_);
            if (get_current_() == end)
                return *this;
            const auto [pbegin, pend] = std::ranges::subrange{parent_->pattern_};
            if (pbegin == pend)
                ++get_current_();
            else if constexpr (!std::ranges::forward_range<Base>) {
                get_current_() = std::ranges::find(std::move(get_current_()), end, *pbegin);
                if (get_current_() != end) {
                    ++get_current_();
                }
            } else {
                do {
                    auto [b, p] =
                        std::ranges::mismatch(std::move(get_current_()), end, pbegin, pend);
                    if (p == pend) {
                        get_current_() = std::move(b);
                        break;
                    }
                } while (++get_current_() != end);
            }
            return *this;
        }

        constexpr decltype(auto) operator++(int) {
            if constexpr (std::ranges::forward_range<Base>) {
                auto tmp = *this;
                ++*this;
                return tmp;
            } else {
                ++*this;
            }
        }

        friend constexpr bool operator==(const outer_iterator& x, const outer_iterator& y)
            requires std::ranges::forward_range<Base> {
            return x.current_ == y.current_;
        }
        friend constexpr bool operator==(const outer_iterator& x, std::default_sentinel_t) {
            return x.get_current_() == std::ranges::end(x.parent_->base_);
        }
    };

    template<bool Const>
    struct inner_iterator {
    private:
        using Base = maybe_const<Const, V>;
        outer_iterator<Const> i_ = decltype(i_)();
        bool incremented_ = false;
    public:
        using iterator_concept = typename outer_iterator<Const>::iterator_concept;
        using iterator_category = std::conditional_t<
            std::derived_from<
                typename std::iterator_traits<std::ranges::iterator_t<Base>>::iterator_category,
                std::forward_iterator_tag>,
            std::forward_iterator_tag,
            typename std::iterator_traits<std::ranges::iterator_t<Base>>::iterator_category>;
        using value_type = std::ranges::range_value_t<Base>;
        using difference_type = std::ranges::range_difference_t<Base>;

        inner_iterator() = default;
        constexpr explicit inner_iterator(outer_iterator<Const> i) : i_(std::move(i)) {}

        constexpr decltype(auto) operator*() const { return *i_.get_current_(); }
        constexpr inner_iterator& operator++() {
            incremented_ = true;
            if constexpr (!std::ranges::forward_range<Base>) {
                if constexpr (Pattern::size() == 0) {
                    return *this;
                }
            }
            ++i_.get_current_();
            return *this;
        }
        constexpr decltype(auto) operator++(int) {
            if constexpr (std::ranges::forward_range<V>) {
                auto tmp = *this;
                ++*this;
                return tmp;
            } else {
                ++*this;
            }
        }

        friend constexpr bool operator==(const inner_iterator& x, const inner_iterator& y)
            requires std::ranges::forward_range<Base> {
            return x.i_.get_current_() == y.i_.get_current_();
        }
        friend constexpr bool operator==(const inner_iterator& x, std::default_sentinel_t) {
            auto [pcur, pend] = std::ranges::subrange{x.i_.parent_->pattern_};
            auto end = std::ranges::end(x.i_.parent_->base_);
            if constexpr (!std::ranges::forward_range<Base>) {
                const auto& cur = x.i_.get_current_();
                if (cur == end) return true;
                if (pcur == pend) return x.incremented_;
                return *cur == *pcur;
            } else {
                auto cur = x.i_.get_current_();
                if (cur == end) return true;
                if (pcur == pend) return x.incremented_;
                do {
                    if (*cur != *pcur) return false;
                    if (++pcur == pend) return true;
                } while (++cur != end);
                return false;
            }
        }

        friend constexpr decltype(auto) iter_move(const inner_iterator& i)
            noexcept(noexcept(std::ranges::iter_move(i.i_.get_current_()))) {
            return std::ranges::iter_move(i.i_.get_current_());
        }

        friend constexpr void iter_swap(const inner_iterator& x, const inner_iterator& y)
            noexcept(noexcept(std::ranges::iter_swap(x.i_.get_current_(), y.i_.get_current_())))
            requires std::indirectly_swappable<std::ranges::iterator_t<Base>> {
            std::ranges::iter_swap(x.i_.get_current_(), y.i_.get_current_());
        }
    };

public:
    split_view() = default;
    constexpr split_view(V base, Pattern pattern)
        : base_(std::move(base)), pattern_(std::move(pattern)) {}

    template<std::ranges::input_range R>
    requires std::constructible_from<V, std::views::all_t<R>> &&
             std::constructible_from<Pattern,
                 std::ranges::single_view<std::ranges::range_value_t<R>>>
    constexpr split_view(R&& r, std::ranges::range_value_t<R> e)
        : base_(std::views::all(std::forward<R>(r))),
          pattern_(std::ranges::single_view{std::move(e)}) {}

    constexpr V base() const& requires std::copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() {
        if constexpr (std::ranges::forward_range<V>)
            return outer_iterator<simple_view<V>>{*this, std::ranges::begin(base_)};
        else {
            current_ = std::ranges::begin(base_);
            return outer_iterator<false>{*this};
        }
    }

    constexpr auto begin() const requires std::ranges::forward_range<V> &&
                                          std::ranges::forward_range<const V> {
        return outer_iterator<true>{*this, std::ranges::begin(base_)};
    }

    constexpr auto end() requires std::ranges::forward_range<V> &&
                                  std::ranges::common_range<V> {
        return outer_iterator<simple_view<V>>{*this, std::ranges::end(base_)};
    }

    constexpr auto end() const {
        if constexpr (std::ranges::forward_range<V> &&
            std::ranges::forward_range<const V> && std::ranges::common_range<const V>)
            return outer_iterator<true>{*this, std::ranges::end(base_)};
        else
            return std::default_sentinel;
    }
};

template<class R, class P>
split_view(R&&, P&&) -> split_view<std::views::all_t<R>, std::views::all_t<P>>;

template<std::ranges::input_range R>
split_view(R&&, std::ranges::range_value_t<R>)
    -> split_view<std::views::all_t<R>, std::ranges::single_view<std::ranges::range_value_t<R>>>;

