# cpp-embedlib

[![test](https://github.com/yhirose/cpp-embedlib/actions/workflows/ci.yml/badge.svg)](https://github.com/yhirose/cpp-embedlib/actions/workflows/ci.yml)

A C++20 CMake library for embedding files and directories into binaries. Access embedded files at runtime through a simple, type-safe API with zero-copy reads.

Pairs seamlessly with [cpp-httplib](https://github.com/yhirose/cpp-httplib) to serve embedded files over HTTP — build a self-contained web server with no external assets.

This library is used in ["Building a Desktop LLM App with cpp-httplib"](https://yhirose.github.io/cpp-httplib/en/llm-app/).

## Features

- **Single-header runtime API** with `std::span` and `std::string_view` (zero-copy access)
- **MIME type detection** from file extensions (55+ built-in types, user-customizable)
- **cpp-httplib integration** — serve embedded files with one line of code

## Requirements

- C++20
- CMake 3.20+

## Quick Start

### Project Structure

```text
my_project/
├── CMakeLists.txt
├── main.cpp
└── www/
    ├── index.html
    ├── css/
    │   └── style.css
    └── images/
        └── logo.png
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_project CXX)

# Fetch cpp-embedlib
include(FetchContent)
FetchContent_Declare(cpp-embedlib
    GIT_REPOSITORY https://github.com/yhirose/cpp-embedlib
    GIT_TAG        main
)
FetchContent_MakeAvailable(cpp-embedlib)

# Embed www/ directory into a static library "WebAssets"
cpp_embedlib_add(WebAssets
    FOLDER    ${CMAKE_CURRENT_SOURCE_DIR}/www
    NAMESPACE Web
)

add_executable(my_project main.cpp)
target_link_libraries(my_project PRIVATE WebAssets)
```

### main.cpp

```cpp
#include "WebAssets.h" // generated from cpp_embedlib_add()
#include <iostream>

int main() {
  // Find a file and read as text (zero-copy)
  if (auto it = Web::FS.find("index.html"); it != Web::FS.end()) {
    auto entry = *it;
    std::cout << entry.path() << ": " << entry.text().value().size() << " bytes\n";
    std::cout << "MIME: " << entry.mime_type() << "\n";
  }

  // Iterate over all embedded files
  for (const auto& entry : Web::FS) {
    if (entry.is_file()) {
      std::cout << entry.path() << " (" << entry.size() << " bytes)\n";
    }
  }

  // Directory listing
  for (const auto& child : Web::FS.root().children()) {
    std::cout << child.name() << (child.is_dir() ? "/" : "") << "\n";
  }
}
```

### Build and Run

```bash
cmake -B build
cmake --build build
./build/my_project
```

## CMake Macro

```cmake
cpp_embedlib_add(
    <TARGET_NAME>
    [FOLDER <dir>]            # Embed an entire directory recursively
    [FILES <file>...]         # Embed individual files
    [NAMESPACE <ns>]          # C++ namespace (defaults to target name)
    [BASE_DIR <dir>]          # Base path for relative paths (defaults to FOLDER)
    [MIME_TYPES               # Custom MIME type mappings (extension-type pairs)
        .ext1 "type/subtype"
        .ext2 "type/subtype"
    ]
)
```

The macro creates a static library target that you link against with `target_link_libraries`. The generated header `<TARGET_NAME>.h` is automatically available. The header defines a `<NAMESPACE>::FS` object (an `EmbeddedFS` instance) that you use to access the embedded files.

### MIME Type Resolution

`Entry::mime_type()` resolves MIME types from file extensions using 55+ built-in types, falling back to `application/octet-stream`.

To add custom MIME types, use the `MIME_TYPES` option. This generates a `<NAMESPACE>::mime_type()` function that checks custom types first, then falls back to built-in types:

```cmake
cpp_embedlib_add(WebAssets
    FOLDER www/
    NAMESPACE Web
    MIME_TYPES
        .vue     "text/html"
        .ts      "text/typescript"
        .svelte  "text/html"
)
```

```cpp
Web::mime_type("app.vue")    // → "text/html"                (custom)
Web::mime_type("style.css")  // → "text/css"                 (built-in fallback)
Web::mime_type("data.xyz")   // → "application/octet-stream" (default fallback)
```

## Runtime API

```cpp
namespace cpp_embedlib {

// A unified entry representing a file or directory.
// Lightweight (two pointers), valid for program lifetime.
class Entry {
public:
  std::string_view path() const;  // full path (e.g. "css/style.css", "css")
  std::string_view name() const;  // last component (e.g. "style.css", "css")
  bool is_file() const;
  bool is_dir() const;

  // File data (zero-copy). Returns std::nullopt for directories.
  std::optional<std::span<const unsigned char>> bytes() const;
  std::optional<std::string_view> text() const;

  std::size_t size() const;             // data size (0 for directories)
  std::vector<Entry> children() const;  // children (empty for files)
  std::string_view mime_type() const;   // MIME type from file extension
};

class EmbeddedFS {
public:
  using iterator = /* random-access iterator returning Entry */;

  // Iterate over all entries (files and directories, sorted by path).
  iterator begin() const;
  iterator end() const;

  // Total number of entries (files + directories).
  std::size_t size() const;

  // Root entry (use root().children() for top-level listing).
  Entry root() const;

  // Find an entry by path (file or directory). Returns end() if not found.
  iterator find(std::string_view path) const;
};

} // namespace cpp_embedlib
```

### Path Normalization

All lookup methods normalize paths before searching:

- Leading `/` and `./` are stripped
- Consecutive `//` are collapsed to `/`
- Paths containing `..` are rejected (returns `end()` / `std::nullopt`)

## cpp-httplib Integration

Serve embedded files over HTTP with [cpp-httplib](https://github.com/yhirose/cpp-httplib):

```cmake
# In your CMakeLists.txt, add cpp-httplib alongside cpp-embedlib:
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GIT_TAG        v0.38.0
)
FetchContent_MakeAvailable(httplib)

add_executable(my_server main.cpp)
target_link_libraries(my_server PRIVATE WebAssets httplib::httplib cpp-embedlib-httplib)
```

```cpp
#include <cpp-embedlib-httplib.h>
#include "WebAssets.h"

int main() {
  httplib::Server svr;

  httplib::mount(svr, Web::FS);                // serve at "/"
  // httplib::mount(svr, Web::FS, "/static");  // serve at a prefix

  svr.listen("0.0.0.0", 8080);
}
```

## Performance Characteristics

Files and directories are stored in a single sorted array at compile time. All lookups use binary search.

| Operation | Complexity | Allocation |
| --- | --- | --- |
| `find()` | O(log N) | None — returns an iterator |
| `Entry::children()` | O(P) | One `std::vector` (P = entries under the directory) |
| `mime_type()` | O(M) | None — linear scan over ~55 built-in extensions |
| `begin()` / `end()` | O(1) | None |

Where N is the total number of entries (files + directories), P is the number of entries under the directory, and M is the number of built-in MIME types.

### Zero-Copy Reads

`bytes()` and `text()` return `std::span` / `std::string_view` pointing directly into the binary's read-only data segment. No memcpy, no heap allocation — the data is served straight from memory mapped by the OS loader.

## License

MIT
