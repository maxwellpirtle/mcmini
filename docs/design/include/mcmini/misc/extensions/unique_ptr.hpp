#pragma once

#include <memory>

namespace mcmini::extensions {

template <typename T>
std::unique_ptr<const T> to_const_unique_ptr(std::unique_ptr<T> ptr) {
  // C++11 doensn't have a constructor for std::unique_ptr<const T>
  // from a std::unique_ptr<T> (this was introduced in C++14). This performs
  // the equivalent operation. Casting from `T*` to `const T` is always safe.
  return std::unique_ptr<const T>(static_cast<const T*>(ptr.release()));
}
}  // namespace mcmini::extensions
