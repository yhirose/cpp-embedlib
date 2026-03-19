#include "WebAssets.h"
#include <iostream>

int main() {
  std::cout << "Embedded entries (" << Web::FS.size() << "):\n";
  for (const auto &entry : Web::FS) {
    if (entry.is_dir()) {
      std::cout << "  " << entry.path() << "/\n";
    } else {
      std::cout << "  " << entry.path() << " (" << entry.size() << " bytes, "
                << entry.mime_type() << ")\n";
    }
  }

  if (auto it = Web::FS.find("index.html"); it != Web::FS.end()) {
    std::cout << "\n--- index.html ---\n" << (*it).text().value();
  }
}
