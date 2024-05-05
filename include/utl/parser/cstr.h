#pragma once

#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdint>

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

namespace utl {

struct size {
  constexpr explicit size(size_t s) : size_(s) {}
  size_t size_;
};

struct field {
  size_t from = 0;
  size_t size = 0;
  static constexpr auto MAX_SIZE = std::numeric_limits<int>::max();
};

struct cstr {
  constexpr cstr() : str(nullptr), len(0) {}
  cstr(std::string const& s) : str(s.data()), len(s.length()) {}
  constexpr cstr(std::string_view const& s) : str(s.data()), len(s.length()) {}
  constexpr cstr(char const* s)
      : str(s), len(s ? std::char_traits<char>::length(str) : 0) {}
  constexpr cstr(char const* s, size_t l) : str(s), len(l) {}
  cstr(unsigned char const* s, size_t l)
      : str(reinterpret_cast<char const*>(s)), len(l) {}
  constexpr cstr(char const* begin, char const* end)
      : str(begin), len(static_cast<size_t>(end - begin)) {
    assert(begin <= end);
  }
  constexpr cstr(std::nullptr_t, size_t) : str(nullptr), len(0) {}
  constexpr inline cstr& operator++() {
    ++str;
    --len;
    return *this;
  }
  constexpr cstr operator+(size_t inc) {
    cstr tmp;
    tmp.str = str + inc;
    tmp.len = len - inc;
    return tmp;
  }
  constexpr cstr& operator+=(size_t inc) {
    str = str + inc;
    len = len - inc;
    return *this;
  }
  constexpr bool operator==(cstr const& s) const {
    if (len != s.len) {
      return false;
    } else if (len == 0) {
      return true;
    } else {
      return view() == s.view();
    }
  }
  constexpr bool operator!=(cstr const& s) const { return !operator==(s); }
  bool operator<(cstr const& s) const {
    return std::lexicographical_compare(str, str + len, s.str, s.str + s.len);
  }
  constexpr char operator[](size_t i) const { return str[i]; }
  constexpr inline explicit operator bool() const { return valid(); }
  constexpr inline bool valid() const { return len != 0 && str != nullptr; }
  constexpr char const* begin() const { return str; }
  constexpr char const* end() const { return str + len; }
  constexpr friend char const* begin(cstr const& s) { return s.begin(); }
  constexpr friend char const* end(cstr const& s) { return s.end(); }
  constexpr void assign(char const* s, size_t l) {
    str = s;
    len = l;
  }
  constexpr cstr substr(size_t position, size s) const {
    auto const adjusted_position = std::min(position, len);
    auto const adjusted_size = std::min(s.size_, len - adjusted_position);
    return {str + adjusted_position, adjusted_size};
  }
  constexpr cstr substr(size_t begin, size_t end) const {
    auto const adjusted_begin = std::min(begin, len);
    auto const adjusted_end = std::min(end, len);
    return {str + adjusted_begin, adjusted_end - adjusted_begin};
  }
  constexpr cstr substr(size_t begin) const {
    return {str + begin, len - begin};
  }
  constexpr cstr substr(field const& f) const {
    return (f.size == field::MAX_SIZE) ? substr(f.from)
                                       : substr(f.from, size(f.size));
  }
  constexpr bool contains(cstr needle) const {
    return view().find(needle.view()) != std::string_view::npos;
  }
  constexpr bool starts_with(cstr prefix) const {
    if (len < prefix.len) {
      return false;
    }
    return substr(0, size(prefix.len)) == prefix;
  }
  constexpr static bool is_space(char const c) { return c == ' ' || c == '\n'; }
  constexpr cstr skip_whitespace_front() const {
    auto copy = (*this);
    while (copy.len != 0 && is_space(copy[0])) {
      ++copy;
    }
    return copy;
  }
  constexpr cstr skip_whitespace_back() const {
    auto copy = (*this);
    while (copy.len != 0 && is_space(copy.str[copy.len - 1])) {
      --copy.len;
    }
    return copy;
  }
  constexpr cstr trim() const {
    return skip_whitespace_front().skip_whitespace_back();
  }
  constexpr bool empty() const { return len == 0; }
  constexpr size_t length() const { return len; }
  constexpr char const* c_str() const { return str; }
  constexpr char const* data() const { return str; }
  constexpr size_t substr_offset(cstr needle) {
    for (size_t i = 0; i < len; ++i) {
      if (substr(i).starts_with(needle)) {
        return i;
      }
    }
    return std::numeric_limits<size_t>::max();
  }
  std::string to_str() const { return std::string(str, len); }
  constexpr std::string_view view() const { return {str, len}; }
  constexpr operator std::string_view() { return view(); }

  char const* str;
  size_t len;
};

constexpr inline cstr get_until(cstr s, char delimiter) {
  if (s.len == 0) {
    return s;
  }
  auto end = static_cast<char const*>(std::memchr(s.str, delimiter, s.len));
  if (end == nullptr) {
    return s;
  }
  return {s.str, static_cast<size_t>(end - s.str)};
}

constexpr inline cstr strip_cr(cstr s) {
  return (s && s[s.len - 1] == '\r') ? s.substr(0, size(s.len - 1)) : s;
}

constexpr inline cstr get_line(cstr s) { return strip_cr(get_until(s, '\n')); }

enum class continue_t : std::uint8_t { kContinue, kBreak };

template <typename Function>
constexpr auto for_each_token(cstr s, char separator, Function&& f)
    -> std::enable_if_t<std::is_same_v<decltype(f(cstr{})), void>> {
  while (s.len > 0) {
    cstr token = get_until(s, separator);
    f(token);
    s += token.len;
    // skip separator
    if (s.len != 0) {
      ++s;
    }
  }
}

template <typename Function>
constexpr auto for_each_token(cstr s, char separator, Function&& f)
    -> std::enable_if_t<std::is_same_v<decltype(f(cstr{})), continue_t>> {
  while (s.len > 0) {
    cstr token = get_until(s, separator);
    if (f(token) == continue_t::kBreak) {
      break;
    }
    s += token.len;
    // skip separator
    if (s.len != 0) {
      ++s;
    }
  }
}

struct line_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = cstr;
  using difference_type = std::ptrdiff_t;
  using pointer = cstr*;
  using reference = cstr&;
  using const_reference = cstr const&;

  line_iterator() = default;
  explicit line_iterator(cstr s) : s_{s} { ++*this; }

  explicit operator bool() const { return s_.valid(); }

  bool operator==(line_iterator const& i) const {
    return line_.data() == i.line_.data() && s_ == i.s_;
  }
  bool operator!=(line_iterator const& i) const { return !(*this == i); }

  line_iterator& operator+=(difference_type n) {
    for (int i = 0; i != n; ++i) {
      ++*this;
    }
    return *this;
  }
  line_iterator& operator++() {
    line_ = get_until(s_, '\n');
    s_ += line_.len;
    if (s_.len != 0) {
      ++s_;  // skip separator
    }
    line_ = strip_cr(line_);
    if (line_.begin() == s_.end()) {
      line_ = cstr{};
    }
    return *this;
  }
  line_iterator operator++(int) {
    line_iterator curr = *this;
    ++*this;
    return curr;
  }
  line_iterator operator+(difference_type const n) {
    line_iterator curr = *this;
    *this += n;
    return curr;
  }

  reference operator*() { return line_; }
  const_reference operator*() const { return line_; }
  pointer operator->() { return &line_; }

  cstr s_;
  cstr line_;
};

struct lines {
  using iterator = line_iterator;
  using const_iterator = line_iterator;
  explicit lines(cstr s) : s_{s} {}
  line_iterator begin() const { return line_iterator{s_}; }
  line_iterator end() const { return line_iterator{cstr{}}; }
  friend line_iterator begin(lines const& l) { return l.begin(); }
  friend line_iterator end(lines const& l) { return l.end(); }
  cstr s_;
};

template <typename Function>
void for_each_token_numbered(cstr s, char separator, Function&& f) {
  int token_number = 0;
  for_each_token(s, separator, [&](cstr token) { f(token, ++token_number); });
}

template <typename Function>
void for_each_line(cstr s, Function&& f) {
  for_each_token(s, '\n', [&f](cstr token) { f(strip_cr(token)); });
}

template <typename Function>
void for_each_line_numbered(cstr s, Function&& f) {
  auto line_number = 0U;
  for_each_line(s, [&](cstr line) { f(line, ++line_number); });
}

template <typename Predicate>
cstr skip_lines(cstr file_content, Predicate skip) {
  while (file_content.len > 0) {
    cstr line = get_line(file_content);
    if (!skip(line)) {
      break;
    }
    file_content += line.len;
    if (file_content.len != 0) {
      ++file_content;
    }
  }
  return file_content;
}

}  // namespace utl
