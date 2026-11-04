// cpp-embedlib: Embed files into C++ binaries with zero dependencies.
// SPDX-License-Identifier: MIT
#pragma once

// ---------------------------------------------------------------------------
// Language standard detection.
//
// MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is given, so use
// _MSVC_LANG (which always reflects /std:c++NN) when available.
// ---------------------------------------------------------------------------
#if defined(_MSVC_LANG)
#  define CPP_EMBEDLIB_CPLUSPLUS _MSVC_LANG
#else
#  define CPP_EMBEDLIB_CPLUSPLUS __cplusplus
#endif

#if CPP_EMBEDLIB_CPLUSPLUS < 201703L
#  error "cpp-embedlib requires C++17 or later."
#endif

// std::span is C++20. On C++17 we fall back to a minimal equivalent below.
// __has_include() also covers C++20 toolchains that ship an incomplete
// standard library (e.g. older AppleClang with -std=c++2a).
#if CPP_EMBEDLIB_CPLUSPLUS >= 202002L
#  if defined(__has_include)
#    if __has_include(<span>)
#      define CPP_EMBEDLIB_HAS_STD_SPAN 1
#    endif
#  else
#    define CPP_EMBEDLIB_HAS_STD_SPAN 1
#  endif
#endif

#ifndef CPP_EMBEDLIB_HAS_STD_SPAN
#  define CPP_EMBEDLIB_HAS_STD_SPAN 0
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if CPP_EMBEDLIB_HAS_STD_SPAN
#  include <span>
#endif

namespace cpp_embedlib {

// ---------------------------------------------------------------------------
// span<T>: alias for std::span on C++20, minimal read-only replacement on C++17.
// ---------------------------------------------------------------------------
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

  constexpr span subspan(size_type offset) const noexcept {
    return (offset > size_) ? span{}
                            : span{data_ + offset, size_ - offset};
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

namespace detail {

struct StorageEntry {
  std::string_view path;
  span<const unsigned char> data;
  bool is_dir;
};

struct PathLess {
  using is_transparent = void;
  bool operator()(const StorageEntry &entry, std::string_view p) const {
    return entry.path < p;
  }
};

// Result of normalize_path: holds either a borrowed string_view (fast path,
// no allocation) or an owned string (slow path, normalization was needed).
struct NormalizedPath {
  std::string owned;
  std::string_view view;

  // Fast path: borrow the original input
  explicit NormalizedPath(std::string_view sv) : view(sv) {}
  // Slow path: take ownership
  explicit NormalizedPath(std::string &&s) : owned(std::move(s)), view(owned) {}

  NormalizedPath(NormalizedPath &&o) noexcept
      : owned(std::move(o.owned)),
        view(owned.empty() ? o.view : std::string_view{owned}) {}

  NormalizedPath(const NormalizedPath &) = delete;
  NormalizedPath &operator=(const NormalizedPath &) = delete;
  NormalizedPath &operator=(NormalizedPath &&) = delete;

  operator std::string_view() const { return view; }
};

// Normalize a path for lookup:
// - Strip leading '/' and './'
// - Collapse consecutive '/' into one
// - Reject paths containing '..'
inline std::optional<NormalizedPath> normalize_path(std::string_view path) {
  // Single scan: reject '..' and detect if normalization is needed.
  bool needs_normalize = false;

  for (std::size_t i = 0; i < path.size(); ++i) {
    char c = path[i];
    if (c == '.') {
      // Check for '..' traversal
      if (i + 1 < path.size() && path[i + 1] == '.') {
        bool at_start = (i == 0) || (path[i - 1] == '/');
        bool at_end = (i + 2 >= path.size()) || (path[i + 2] == '/');
        if (at_start && at_end) { return std::nullopt; }
      }
      // Leading './' needs normalization
      if (i == 0 && i + 1 < path.size() && path[i + 1] == '/') {
        needs_normalize = true;
      }
    } else if (c == '/') {
      if (i == 0 ||                                      // leading '/'
          (i + 1 < path.size() && path[i + 1] == '/') || // consecutive '//'
          (i + 1 == path.size())) {                      // trailing '/'
        needs_normalize = true;
      }
    }
  }

  // Fast path: path is already clean - no allocation, no copy
  if (!needs_normalize) { return NormalizedPath{path}; }

  // Slow path: strip leading '/' and './', collapse '//', remove trailing '/'
  while (!path.empty()) {
    if (path[0] == '/') {
      path.remove_prefix(1);
    } else if (path.size() >= 2 && path[0] == '.' && path[1] == '/') {
      path.remove_prefix(2);
    } else {
      break;
    }
  }

  std::string result;
  result.reserve(path.size());

  bool prev_slash = false;
  for (char c : path) {
    if (c == '/') {
      if (!prev_slash) { result.push_back(c); }
      prev_slash = true;
    } else {
      result.push_back(c);
      prev_slash = false;
    }
  }

  if (!result.empty() && result.back() == '/') { result.pop_back(); }

  return NormalizedPath{std::move(result)};
}

// MIME type from file extension.
// Returns "application/octet-stream" for unknown extensions.
inline std::string_view mime_type(std::string_view path) {
  auto dot = path.rfind('.');
  if (dot == std::string_view::npos) { return "application/octet-stream"; }
  auto ext = path.substr(dot);

  // Text / Web
  if (ext == ".html" || ext == ".htm") return "text/html";
  if (ext == ".xhtml" || ext == ".xht") return "application/xhtml+xml";
  if (ext == ".css") return "text/css";
  if (ext == ".js" || ext == ".mjs") return "text/javascript";
  if (ext == ".json") return "application/json";
  if (ext == ".xml") return "application/xml";
  if (ext == ".xslt") return "application/xslt+xml";
  if (ext == ".txt") return "text/plain";
  if (ext == ".csv") return "text/csv";
  if (ext == ".md") return "text/markdown";
  if (ext == ".vtt") return "text/vtt";
  if (ext == ".atom") return "application/atom+xml";
  if (ext == ".rss") return "application/rss+xml";
  if (ext == ".map") return "application/json";

  // Images
  if (ext == ".png") return "image/png";
  if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
  if (ext == ".gif") return "image/gif";
  if (ext == ".svg") return "image/svg+xml";
  if (ext == ".ico") return "image/x-icon";
  if (ext == ".webp") return "image/webp";
  if (ext == ".avif") return "image/avif";
  if (ext == ".apng") return "image/apng";
  if (ext == ".bmp") return "image/bmp";
  if (ext == ".tif" || ext == ".tiff") return "image/tiff";

  // Audio
  if (ext == ".mp3") return "audio/mp3";
  if (ext == ".mpga") return "audio/mpeg";
  if (ext == ".wav") return "audio/wave";
  if (ext == ".weba") return "audio/webm";
  if (ext == ".ogg") return "audio/ogg";
  if (ext == ".flac") return "audio/flac";
  if (ext == ".aac") return "audio/aac";
  if (ext == ".m4a") return "audio/mp4";
  if (ext == ".opus") return "audio/opus";

  // Video
  if (ext == ".mp4") return "video/mp4";
  if (ext == ".mpeg") return "video/mpeg";
  if (ext == ".webm") return "video/webm";
  if (ext == ".ogv") return "video/ogg";
  if (ext == ".avi") return "video/x-msvideo";
  if (ext == ".mov") return "video/quicktime";
  if (ext == ".mkv") return "video/x-matroska";

  // Fonts
  if (ext == ".woff") return "font/woff";
  if (ext == ".woff2") return "font/woff2";
  if (ext == ".ttf") return "font/ttf";
  if (ext == ".otf") return "font/otf";
  if (ext == ".eot") return "application/vnd.ms-fontobject";

  // Archives
  if (ext == ".zip") return "application/zip";
  if (ext == ".gz") return "application/gzip";
  if (ext == ".tar") return "application/x-tar";
  if (ext == ".7z") return "application/x-7z-compressed";
  if (ext == ".br") return "application/x-brotli";
  if (ext == ".zst") return "application/zstd";

  // Documents
  if (ext == ".pdf") return "application/pdf";

  // Binary / Other
  if (ext == ".wasm") return "application/wasm";

  return "application/octet-stream";
}

} // namespace detail

// Forward declaration
class EmbeddedFS;

// A unified entry representing either a file or a directory in the embedded
// filesystem. Entry objects are lightweight (two pointers) and remain valid
// for the program lifetime.
class Entry {
  const detail::StorageEntry *entry_;
  const EmbeddedFS *fs_;

  friend class EmbeddedFS;
  Entry(const detail::StorageEntry *e, const EmbeddedFS *fs)
      : entry_(e), fs_(fs) {}

public:
  // Full path (e.g. "css/style.css", "css")
  std::string_view path() const { return entry_->path; }

  // Filename or directory name (last component of path, e.g. "style.css",
  // "css")
  std::string_view name() const {
    auto p = entry_->path;
    auto slash = p.rfind('/');
    return (slash == std::string_view::npos) ? p : p.substr(slash + 1);
  }

  bool is_file() const { return !entry_->is_dir; }
  bool is_dir() const { return entry_->is_dir; }

  // File data as bytes. Returns std::nullopt for directories.
  std::optional<span<const unsigned char>> bytes() const {
    if (entry_->is_dir) { return std::nullopt; }
    return entry_->data;
  }

  // File data as text. Returns std::nullopt for directories.
  std::optional<std::string_view> text() const {
    if (entry_->is_dir) { return std::nullopt; }
    return std::string_view{reinterpret_cast<const char *>(entry_->data.data()),
                            entry_->data.size()};
  }

  // MIME type from file extension.
  // Returns "application/octet-stream" for unknown extensions or directories.
  std::string_view mime_type() const;

  // Data size in bytes. Returns 0 for directories.
  std::size_t size() const { return entry_->data.size(); }

  // Immediate children (only meaningful for directories).
  // Returns empty vector for files.
  std::vector<Entry> children() const;
};

class EmbeddedFS {
  friend class Entry;

  span<const detail::StorageEntry> entries_;
  static inline const detail::StorageEntry root_entry_{"", {}, true};

  const detail::StorageEntry *find_storage(std::string_view path) const {
    auto normalized = detail::normalize_path(path);
    if (!normalized) { return nullptr; }

    std::string_view key = *normalized;
    auto it = std::lower_bound(entries_.begin(), entries_.end(), key,
                               detail::PathLess{});

    if (it != entries_.end() && it->path == key) { return &(*it); }
    return nullptr;
  }

  // Find the range of entries whose path starts with prefix + "/".
  // Returns an empty span if the directory doesn't exist.
  span<const detail::StorageEntry>
  find_prefix_range(std::string_view dir) const {
    auto normalized = detail::normalize_path(dir);
    if (!normalized) { return {}; }

    std::string prefix{std::string_view(*normalized)};
    prefix += '/';

    auto first = std::lower_bound(entries_.begin(), entries_.end(),
                                  std::string_view{prefix},
                                  detail::PathLess{});

    // Upper bound: increment last char of prefix to find end of range.
    // '/' is 0x2F, '0' is 0x30 - so "sub/" becomes "sub0".
    prefix.back() += 1;
    auto last = std::lower_bound(first, entries_.end(),
                                 std::string_view{prefix},
                                 detail::PathLess{});

    return {first, last};
  }

public:
  EmbeddedFS(span<const detail::StorageEntry> entries)
      : entries_(entries) {}

  // --- Iterator ---
  class Iterator {
    friend class EmbeddedFS;

    const detail::StorageEntry *ptr_;
    const EmbeddedFS *fs_;

    Iterator(const detail::StorageEntry *p, const EmbeddedFS *fs)
        : ptr_(p), fs_(fs) {}

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Entry;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = Entry;

    Iterator() : ptr_(nullptr), fs_(nullptr) {}

    Entry operator*() const { return Entry{ptr_, fs_}; }
    Entry operator[](difference_type n) const { return Entry{ptr_ + n, fs_}; }

    Iterator &operator++() { ++ptr_; return *this; }
    Iterator operator++(int) { auto tmp = *this; ++ptr_; return tmp; }
    Iterator &operator--() { --ptr_; return *this; }
    Iterator operator--(int) { auto tmp = *this; --ptr_; return tmp; }

    Iterator &operator+=(difference_type n) { ptr_ += n; return *this; }
    Iterator &operator-=(difference_type n) { ptr_ -= n; return *this; }

    friend Iterator operator+(Iterator it, difference_type n) {
      return {it.ptr_ + n, it.fs_};
    }
    friend Iterator operator+(difference_type n, Iterator it) {
      return {it.ptr_ + n, it.fs_};
    }
    friend Iterator operator-(Iterator it, difference_type n) {
      return {it.ptr_ - n, it.fs_};
    }
    friend difference_type operator-(Iterator a, Iterator b) {
      return a.ptr_ - b.ptr_;
    }

    bool operator==(const Iterator &o) const { return ptr_ == o.ptr_; }
    bool operator!=(const Iterator &o) const { return ptr_ != o.ptr_; }
    bool operator<(const Iterator &o) const { return ptr_ < o.ptr_; }
    bool operator<=(const Iterator &o) const { return ptr_ <= o.ptr_; }
    bool operator>(const Iterator &o) const { return ptr_ > o.ptr_; }
    bool operator>=(const Iterator &o) const { return ptr_ >= o.ptr_; }
  };

  using iterator = Iterator;
  using const_iterator = Iterator;

  Iterator begin() const {
    return Iterator{entries_.data(), this};
  }
  Iterator end() const {
    return Iterator{entries_.data() + entries_.size(), this};
  }

  // --- find ---
  // Returns an iterator to the entry with the given path (file or directory).
  // Returns end() if not found.
  Iterator find(std::string_view path) const {
    auto *p = find_storage(path);
    if (!p) { return end(); }
    return Iterator{p, this};
  }

  // Root entry of the embedded filesystem.
  Entry root() const { return Entry{&root_entry_, this}; }

  // Total number of entries (files + directories).
  std::size_t size() const { return entries_.size(); }
};

// --- Entry method implementations that depend on EmbeddedFS ---

inline std::string_view Entry::mime_type() const {
  return detail::mime_type(entry_->path);
}

inline std::vector<Entry> Entry::children() const {
  if (!entry_->is_dir) { return {}; }

  // entry_->path is a compile-time literal - already normalized.
  // Inline the prefix-range logic to avoid redundant normalize_path().
  auto dir = entry_->path;
  span<const detail::StorageEntry> entries;
  std::size_t prefix_len;

  if (dir.empty()) {
    entries = fs_->entries_;
    prefix_len = 0;
  } else {
    std::string prefix{dir};
    prefix += '/';
    auto first = std::lower_bound(fs_->entries_.begin(), fs_->entries_.end(),
                                   std::string_view{prefix},
                                   detail::PathLess{});
    prefix.back() += 1;
    auto last = std::lower_bound(first, fs_->entries_.end(),
                                  std::string_view{prefix},
                                  detail::PathLess{});
    entries = {first, last};
    prefix_len = dir.size() + 1;
  }

  // An immediate child is any entry whose relative path contains no '/'.
  std::vector<Entry> result;
  result.reserve(entries.size());
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    auto rel = it->path.substr(prefix_len);
    if (rel.find('/') == std::string_view::npos) {
      result.push_back(Entry{&(*it), fs_});
    }
  }
  return result;
}

} // namespace cpp_embedlib
