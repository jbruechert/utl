#pragma once

#include <functional>

namespace utl {
namespace detail {

template <typename Tuple, typename Func, std::size_t... I>
auto map_tup(std::index_sequence<I...>, Tuple& tuple, Func&& func) {
  return std::make_tuple(func(std::get<I>(tuple))...);
}

template <typename Tuple, typename Func>
auto map_tup(Tuple& tuple, Func&& func) {
  using Sequence = std::make_index_sequence<std::tuple_size<Tuple>::value>;
  return map_tup(Sequence(), tuple, func);
}

template <typename Tuple, typename Func, std::size_t... I>
void each_tup(std::index_sequence<I...>, Tuple& tuple, Func&& func) {
  // see: http://stackoverflow.com/a/26902803
  using expander = int[];
  (void)expander{0, ((void)func(std::get<I>(tuple)), 0)...};
}

template <typename Tuple, typename Func>
void each_tup(Tuple& tuple, Func&& func) {
  using Sequence = std::make_index_sequence<std::tuple_size<Tuple>::value>;
  each_tup(Sequence(), tuple, func);
}

template <typename Tuple>
struct zip_iterator;

template <typename... Iterators>
struct zip_iterator<std::tuple<Iterators...>> {
  using SelfType = zip_iterator<std::tuple<Iterators...>>;
  using IteratorSeq = std::make_index_sequence<sizeof...(Iterators)>;

  using Values =
      std::tuple<typename std::iterator_traits<Iterators>::value_type...>;

  using References =
      std::tuple<typename std::iterator_traits<Iterators>::reference...>;

  explicit zip_iterator(std::tuple<Iterators...> its) : its_(its) {}

  template <std::size_t... I>
  auto deref_helper(std::index_sequence<I...>) {
    return std::forward_as_tuple(*std::get<I>(its_)...);
  }

  References operator*() {
    using Sequence = std::make_index_sequence<sizeof...(Iterators)>;
    return deref_helper(Sequence());
  }

  SelfType& operator++() {
    each_tup(its_, [](auto&& it) { ++it; });
    return *this;
  }

  bool operator==(SelfType const& rhs) const { return its_ == rhs.its_; }
  bool operator!=(SelfType const& rhs) const { return its_ != rhs.its_; }
  bool operator>(SelfType const& rhs) const { return its_ > rhs.its_; }
  bool operator<(SelfType const& rhs) const { return its_ < rhs.its_; }
  bool operator>=(SelfType const& rhs) const { return its_ >= rhs.its_; }
  bool operator<=(SelfType const& rhs) const { return its_ <= rhs.its_; }

  std::tuple<Iterators...> its_;
};

template <typename... Containers>
struct zip_range {
  using Iterator = zip_iterator<
      std::tuple<typename std::remove_reference_t<Containers>::iterator...>>;
  using ConstIterator = zip_iterator<std::tuple<
      typename std::remove_reference_t<Containers>::const_iterator...>>;

  zip_range(std::tuple<Containers...> tup) : tup_(tup) {}

  ConstIterator begin() const {
    return ConstIterator{map_tup(tup_, [](auto&& c) { return std::begin(c); })};
  }

  Iterator begin() {
    return Iterator{map_tup(tup_, [](auto&& c) { return std::begin(c); })};
  }

  ConstIterator end() const {
    return ConstIterator{map_tup(tup_, [](auto&& c) { return std::end(c); })};
  }

  Iterator end() {
    return Iterator{map_tup(tup_, [](auto&& c) { return std::end(c); })};
  }

  std::tuple<Containers...> tup_;
};

}  // namespace detail

template <typename... Containers>
auto zip(Containers&... containers) -> detail::zip_range<Containers&...> {
  static_assert(sizeof...(Containers) > 0, "cannot zip nothing ;)");
  std::array<size_t, sizeof...(Containers)> sizes{{containers.size()...}};
  for (auto const& size : sizes) {
    if (size != sizes[0]) {
      throw std::runtime_error("utl::zip container size mismatch");
    }
  }

  return detail::zip_range<Containers&...>{
      std::forward_as_tuple(containers...)};
};

}  // namespace motis
