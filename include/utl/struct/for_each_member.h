#include <type_traits>
#include <utility>

#include "utl/struct/arity.h"
#include "utl/struct/to_tuple.h"

namespace utl {

template <typename T, typename Fn>
inline void for_each_field(T& t, Fn&& fn) {
  if constexpr (std::is_pointer_v<T>) {
    if (t != nullptr) {
      for_each_field(*t, std::forward<Fn>(fn));
    }
  } else if constexpr (std::is_scalar_v<T>) {
    f(t);
  } else {
    std::apply([&](auto&&... args) { (fn(args), ...); }, to_tuple(t));
  }
}

}  // namespace utl