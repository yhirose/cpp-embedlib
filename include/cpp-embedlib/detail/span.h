// cpp-embedlib: Embed files into C++ binaries with zero dependencies.
// SPDX-License-Identifier: MIT
//
// span<T>: alias for std::span on C++20, minimal read-only replacement on C++17.
//
// Implementation detail. Include <cpp-embedlib/cpp-embedlib.h> instead.
#pragma once

#include <cpp-embedlib/detail/config.h>

#include <cstddef>
#include <type_traits>

#if CPP_EMBEDLIB_HAS_STD_SPAN
#  include <span>
#endif

namespace cpp_embedlib {

#if CPP_EMBEDLIB_HAS_STD_SPAN

template <class T> using span = std::span<T>;

#else // C++17

template <class T> class span {
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = element_type *;
  using const_pointer = const element_type *;
  using reference = element_type &;
  using iterator = pointer;

  constexpr span() noexcept : data_(nullptr), size_(0) {}

  constexpr span(pointer data, size_type size) noexcept
      : data_(data), size_(size) {}

  // Iterator-pair constructor: span{a, b} where b - a is the size.
  constexpr span(pointer first, pointer last) noexcept
      : data_(first), size_(static_cast<size_type>(last - first)) {}

  template <size_type N>
  constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N) {}

  constexpr pointer data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type size_bytes() const noexcept {
    return size_ * sizeof(element_type);
  }
  constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size_; }
  constexpr reference operator[](size_type i) const noexcept { return data_[i]; }
  constexpr reference front() const noexcept { return data_[0]; }
  constexpr reference back() const noexcept { return data_[size_ - 1]; }

  // Unlike std::span, out-of-range requests are clamped instead of being
  // undefined behavior.
  constexpr span subspan(size_type offset) const noexcept {
    return (offset > size_) ? span{} : span{data_ + offset, size_ - offset};
  }

  constexpr span subspan(size_type offset, size_type count) const noexcept {
    if (offset > size_) { return span{}; }
    const size_type avail = size_ - offset;
    return span{data_ + offset, (count < avail) ? count : avail};
  }

private:
  pointer data_;
  size_type size_;
};

#endif // CPP_EMBEDLIB_HAS_STD_SPAN

// Array helper so generated code stays identical for C++17 and C++20
// (C++20's std::span CTAD is not available on C++17).
template <class T, std::size_t N>
constexpr span<T> make_span(T (&arr)[N]) noexcept {
  return span<T>(arr, N);
}

} // namespace cpp_embedlib
