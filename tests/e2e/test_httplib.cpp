#include <doctest/doctest.h>
#include "E2EAssets.h"
#include <cpp-embedlib-httplib.h>
#include <httplib.h>

#include <string>
#include <thread>

TEST_CASE("httplib serves embedded files") {
  httplib::Server svr;
  httplib::mount(svr, E2E::FS);

  auto t = std::thread([&] { svr.listen("127.0.0.1", 18787); });
  svr.wait_until_ready();

  httplib::Client cli("http://127.0.0.1:18787");

  SUBCASE("GET existing text file") {
    auto res = cli.Get("/hello.txt");
    REQUIRE(res);
    CHECK(res->status == 200);
    CHECK(res->body == "Hello, World!\n");
    CHECK(res->get_header_value("Content-Type") == "text/plain");
  }

  SUBCASE("GET existing binary file") {
    auto res = cli.Get("/binary.bin");
    REQUIRE(res);
    CHECK(res->status == 200);
    CHECK(res->body.size() == 8);
    CHECK(res->get_header_value("Content-Type") == "application/octet-stream");
  }

  SUBCASE("GET nested file") {
    auto res = cli.Get("/sub/nested.txt");
    REQUIRE(res);
    CHECK(res->status == 200);
    CHECK(res->body == "nested\n");
  }

  SUBCASE("GET nonexistent file returns 404") {
    auto res = cli.Get("/nonexistent.txt");
    REQUIRE(res);
    CHECK(res->status == 404);
  }

  SUBCASE("GET empty path serves index.html fallback") {
    auto res = cli.Get("/");
    REQUIRE(res);
    // No index.html in test assets, so 404
    CHECK(res->status == 404);
  }

  svr.stop();
  t.join();
}
