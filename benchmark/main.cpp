#include "BenchAssets.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Timing helpers
// ---------------------------------------------------------------------------
using Clock = std::chrono::high_resolution_clock;

struct BenchResult {
  const char *name;
  double total_ns;
  std::size_t iterations;

  double ns_per_op() const { return total_ns / iterations; }
};

static void print_result(const BenchResult &r) {
  std::printf("  %-40s %10.1f ns/op  (%zu iters, %.3f ms total)\n", r.name,
              r.ns_per_op(), r.iterations, r.total_ns / 1e6);
}

static void print_header(const char *section) {
  std::printf("\n=== %s ===\n", section);
}

// Prevent the compiler from optimizing away the result.
template <typename T> static void do_not_optimize(const T &val) {
  asm volatile("" : : "r,m"(val) : "memory");
}

// ---------------------------------------------------------------------------
// File paths used in benchmarks
// ---------------------------------------------------------------------------
static const std::vector<std::string> kPaths = {
    "index.html",     "about.html",   "contact.html",     "css/style.css",
    "css/reset.css",  "js/app.js",    "js/vendor.js",     "api/config.json",
    "api/users.json", "api/data.xml", "img/icon.png",     "img/photo.jpg",
    "img/bg.webp",    "img/logo.svg", "fonts/main.woff2", "fonts/fallback.ttf",
    "README.md",      "robots.txt",   "favicon.ico",
};

// Paths that require normalization work
static const std::vector<std::string> kNormPaths = {
    "/index.html",        "./css/style.css", "js//app.js",
    "/./api/config.json", "img///photo.jpg", "///fonts/main.woff2",
};

// Paths that don't exist (worst case for binary search)
static const std::vector<std::string> kMissPaths = {
    "nonexistent.html",
    "css/missing.css",
    "zzz/deep/path/file.txt",
};

static constexpr std::size_t kWarmup = 1000;
static constexpr std::size_t kIters = 100000;

// ---------------------------------------------------------------------------
// Benchmarks
// ---------------------------------------------------------------------------

// 1. find() + Entry::bytes() — hit
static BenchResult bench_embed_get_hit() {
  for (std::size_t i = 0; i < kWarmup; ++i) {
    for (const auto &p : kPaths)
      do_not_optimize((*Bench::FS.find(p)).bytes());
  }

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kPaths) {
      auto it = Bench::FS.find(p);
      auto r = (*it).bytes();
      do_not_optimize(r);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find + Entry::bytes (hit)", ns, kIters * kPaths.size()};
}

// 2. find() — miss
static BenchResult bench_embed_get_miss() {
  for (std::size_t i = 0; i < kWarmup; ++i) {
    for (const auto &p : kMissPaths)
      do_not_optimize(Bench::FS.find(p));
  }

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kMissPaths) {
      auto r = Bench::FS.find(p);
      do_not_optimize(r);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find (miss)", ns, kIters * kMissPaths.size()};
}

// 3. find() with paths that need normalization
static BenchResult bench_embed_get_normalize() {
  for (std::size_t i = 0; i < kWarmup; ++i) {
    for (const auto &p : kNormPaths)
      do_not_optimize(Bench::FS.find(p));
  }

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kNormPaths) {
      auto r = Bench::FS.find(p);
      do_not_optimize(r);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find (normalize)", ns, kIters * kNormPaths.size()};
}

// 4. Filesystem read — equivalent of get()
static BenchResult bench_fs_read() {
  const std::string base = BENCH_ASSETS_DIR;

  // Warmup + verify files exist
  for (std::size_t i = 0; i < kWarmup; ++i) {
    for (const auto &p : kPaths) {
      std::ifstream f(base + "/" + p, std::ios::binary);
      do_not_optimize(f.good());
    }
  }

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kPaths) {
      std::ifstream f(base + "/" + p, std::ios::binary);
      std::string content((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
      do_not_optimize(content);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"Filesystem read (ifstream)", ns, kIters * kPaths.size()};
}

// 5. mime_type()
static BenchResult bench_mime_type() {
  for (std::size_t i = 0; i < kWarmup; ++i) {
    for (const auto &p : kPaths)
      do_not_optimize(cpp_embedlib::detail::mime_type(p));
  }

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kPaths) {
      auto m = cpp_embedlib::detail::mime_type(p);
      do_not_optimize(m);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"mime_type", ns, kIters * kPaths.size()};
}

// 6. find() as existence check
static BenchResult bench_exists() {
  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kPaths) {
      auto r = (Bench::FS.find(p) != Bench::FS.end());
      do_not_optimize(r);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find != end (exists)", ns, kIters * kPaths.size()};
}

// 7. children()
static BenchResult bench_children() {
  auto css = Bench::FS.find("css");
  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    auto kids = (*css).children();
    do_not_optimize(kids);
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"Entry::children(\"css\")", ns, kIters};
}

// 8. find()
static BenchResult bench_find() {
  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kPaths) {
      auto it = Bench::FS.find(p);
      do_not_optimize(it);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"EmbeddedFS::find", ns, kIters * kPaths.size()};
}

// 9. find + is_dir()
static BenchResult bench_is_dir() {
  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    auto it = Bench::FS.find("js");
    do_not_optimize(it != Bench::FS.end() && (*it).is_dir());
    auto it2 = Bench::FS.find("nonexistent");
    do_not_optimize(it2 != Bench::FS.end());
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find + Entry::is_dir", ns, kIters * 2};
}

// 10. find + Entry::text()
static BenchResult bench_get_str() {
  // Only text files
  static const std::vector<std::string> kTextPaths = {
      "index.html",      "css/style.css", "js/app.js",
      "api/config.json", "README.md",
  };

  auto t0 = Clock::now();
  for (std::size_t i = 0; i < kIters; ++i) {
    for (const auto &p : kTextPaths) {
      auto r = (*Bench::FS.find(p)).text();
      do_not_optimize(r);
    }
  }
  auto t1 = Clock::now();
  double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
  return {"find + Entry::text", ns, kIters * kTextPaths.size()};
}

// ---------------------------------------------------------------------------
// Profile: breakdown of EmbeddedFS::bytes() into components
// ---------------------------------------------------------------------------

static void profile_get_breakdown() {
  print_header("PROFILE: find() breakdown");

  const std::string path = "js/vendor.js";
  constexpr std::size_t N = 500000;

  // (a) normalize_path alone
  {
    auto t0 = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
      auto r = cpp_embedlib::detail::normalize_path(path);
      do_not_optimize(r);
    }
    auto t1 = Clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    BenchResult r{"normalize_path", ns, N};
    print_result(r);
  }

  // (b) normalize_path + std::lower_bound (the full find)
  {
    auto t0 = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
      auto r = Bench::FS.find(path);
      do_not_optimize(r);
    }
    auto t1 = Clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    BenchResult r{"find (full: normalize + binary search)", ns, N};
    print_result(r);
  }

  // (c) mime_type alone
  {
    auto t0 = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
      auto r = cpp_embedlib::detail::mime_type(path);
      do_not_optimize(r);
    }
    auto t1 = Clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    BenchResult r{"mime_type", ns, N};
    print_result(r);
  }

  // (d) Compute overhead: binary search = get - normalize_path
  std::printf("\n  (binary search overhead ≈ get - normalize_path)\n");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
  std::printf("cpp-embedlib benchmark\n");
  std::printf("Files embedded: %zu\n", Bench::FS.size());
  std::printf("Iterations per benchmark: %zu\n\n", kIters);

  // --- Comparison: EmbeddedFS vs Filesystem ---
  print_header("EmbeddedFS vs Filesystem");
  auto embed_hit = bench_embed_get_hit();
  auto fs_read = bench_fs_read();
  print_result(embed_hit);
  print_result(fs_read);
  std::printf("\n  Speedup: %.1fx\n",
              fs_read.ns_per_op() / embed_hit.ns_per_op());

  // --- EmbeddedFS operations ---
  print_header("EmbeddedFS operations");
  print_result(embed_hit);
  print_result(bench_embed_get_miss());
  print_result(bench_embed_get_normalize());
  print_result(bench_get_str());
  print_result(bench_exists());
  print_result(bench_children());
  print_result(bench_is_dir());
  print_result(bench_mime_type());
  print_result(bench_find());

  // --- Profile ---
  profile_get_breakdown();

  return 0;
}
