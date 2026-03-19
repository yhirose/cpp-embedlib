// Directory traversal example for cpp-embedlib.
// Demonstrates depth-first and breadth-first traversal
// using EmbeddedFS::children().

#include "SiteAssets.h"

#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Depth-first traversal (pre-order)
// ---------------------------------------------------------------------------

void dfs(const cpp_embedlib::Entry &dir_entry, int depth) {
  for (const auto &child : dir_entry.children()) {
    std::string indent(depth * 2, ' ');
    if (child.is_dir()) {
      std::cout << indent << child.name() << "/\n";
      dfs(child, depth + 1);
    } else {
      std::cout << indent << child.name() << " (" << child.size()
                << " bytes)\n";
    }
  }
}

// ---------------------------------------------------------------------------
// Breadth-first traversal
// ---------------------------------------------------------------------------

void bfs(const cpp_embedlib::EmbeddedFS &fs) {
  auto current = fs.root().children();

  int depth = 0;
  while (!current.empty()) {
    std::string indent(depth * 2, ' ');
    std::vector<cpp_embedlib::Entry> next;

    for (const auto &child : current) {
      if (child.is_dir()) {
        std::cout << indent << child.name() << "/\n";
        for (const auto &c : child.children()) {
          next.push_back(c);
        }
      } else {
        std::cout << indent << child.name() << " ("
                  << child.size() << " bytes)\n";
      }
    }

    current = std::move(next);
    ++depth;
  }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
  std::cout << "Embedded files: " << Site::FS.size() << "\n\n";

  std::cout << "=== Depth-first (pre-order) ===\n";
  dfs(Site::FS.root(), 0);

  std::cout << "\n=== Breadth-first ===\n";
  bfs(Site::FS);
}
