#include "DupAssets.h"
#include <cstdlib>

int main() {
  // Verify the file was embedded exactly once
  if (Dup::FS.size() != 1) return EXIT_FAILURE;
  if (Dup::FS.find("dup.txt") == Dup::FS.end()) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
