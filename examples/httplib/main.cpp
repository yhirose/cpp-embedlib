#include "WebAssets.h"
#include <cpp-embedlib-httplib.h>
#include <iostream>

int main() {
  httplib::Server svr;
  httplib::mount(svr, Web::FS);

  std::cout << "Serving on http://localhost:8080\n";
  svr.listen("0.0.0.0", 8080);
}
