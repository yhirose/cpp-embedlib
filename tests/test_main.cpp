#include <ostream>
#include "TestAssets.h"
#include <doctest/doctest.h>

#include <string>
#include <vector>

TEST_CASE("get existing file") {
  auto it = Test::FS.find("hello.txt");
  REQUIRE(it != Test::FS.end());
  auto entry = *it;
  CHECK(entry.is_file());
  CHECK(entry.bytes()->size() > 0);

  SUBCASE("content matches") {
    CHECK(entry.text().has_value());
    CHECK(entry.text().value() == "Hello, World!\n");
  }
}

TEST_CASE("get nonexistent file") {
  CHECK(Test::FS.find("no_such_file.txt") == Test::FS.end());
}

TEST_CASE("get_str for text file") {
  auto it = Test::FS.find("sub/nested.txt");
  REQUIRE(it != Test::FS.end());
  auto str = (*it).text();
  REQUIRE(str.has_value());
  CHECK(str.value() == "nested\n");
}

TEST_CASE("get binary file") {
  auto it = Test::FS.find("binary.bin");
  REQUIRE(it != Test::FS.end());
  auto data = (*it).bytes();
  REQUIRE(data.has_value());
  CHECK(data->size() == 8);
  // Verify known byte values: \x00\x01\x02\xff\xfe\xfd\xab\xcd
  CHECK((*data)[0] == 0x00);
  CHECK((*data)[1] == 0x01);
  CHECK((*data)[2] == 0x02);
  CHECK((*data)[3] == 0xff);
  CHECK((*data)[4] == 0xfe);
  CHECK((*data)[5] == 0xfd);
  CHECK((*data)[6] == 0xab);
  CHECK((*data)[7] == 0xcd);
}

TEST_CASE("path normalization") {
  SUBCASE("strip leading ./") {
    CHECK(Test::FS.find("./hello.txt") != Test::FS.end());
  }
  SUBCASE("strip leading /") {
    CHECK(Test::FS.find("/hello.txt") != Test::FS.end());
  }
  SUBCASE("collapse double slash") {
    CHECK(Test::FS.find("sub//nested.txt") != Test::FS.end());
  }
  SUBCASE("reject traversal") {
    CHECK(Test::FS.find("../etc/passwd") == Test::FS.end());
  }
  SUBCASE("reject mid-path traversal") {
    CHECK(Test::FS.find("sub/../hello.txt") == Test::FS.end());
  }
}

TEST_CASE("empty file") {
  auto it = Test::FS.find("empty.txt");
  REQUIRE(it != Test::FS.end());
  CHECK((*it).bytes()->size() == 0);
}

TEST_CASE("exists via find") {
  CHECK(Test::FS.find("hello.txt") != Test::FS.end());
  CHECK(Test::FS.find("sub/nested.txt") != Test::FS.end());
  CHECK(Test::FS.find("nonexistent.txt") == Test::FS.end());

  SUBCASE("directory entries exist") {
    CHECK(Test::FS.find("sub") != Test::FS.end());
  }
}

TEST_CASE("size returns correct entry count") {
  // Assets: a_b.txt, binary.bin, empty.txt, hello.txt, sub (dir), sub/nested.txt
  CHECK(Test::FS.size() == 6);
}

TEST_CASE("iteration is sorted") {
  std::string prev;
  for (const auto &entry : Test::FS) {
    CHECK(std::string(entry.path()) > prev);
    prev = std::string(entry.path());
  }
}

TEST_CASE("mime_type") {
  // Test via Entry::mime_type()
  CHECK((*Test::FS.find("hello.txt")).mime_type() == "text/plain");

  // Test built-in MIME types via namespace function (falls back to detail::mime_type)
  CHECK(Test::mime_type("hello.txt") == "text/plain");
  CHECK(Test::mime_type("binary.bin") == "application/octet-stream");
  CHECK(Test::mime_type("sub/nested.txt") == "text/plain");

  SUBCASE("common web types") {
    CHECK(Test::mime_type("index.html") == "text/html");
    CHECK(Test::mime_type("page.htm") == "text/html");
    CHECK(Test::mime_type("style.css") == "text/css");
    CHECK(Test::mime_type("app.js") == "text/javascript");
    CHECK(Test::mime_type("mod.mjs") == "text/javascript");
    CHECK(Test::mime_type("data.json") == "application/json");
    CHECK(Test::mime_type("logo.png") == "image/png");
    CHECK(Test::mime_type("photo.jpg") == "image/jpeg");
    CHECK(Test::mime_type("photo.jpeg") == "image/jpeg");
    CHECK(Test::mime_type("icon.svg") == "image/svg+xml");
    CHECK(Test::mime_type("favicon.ico") == "image/x-icon");
    CHECK(Test::mime_type("font.woff2") == "font/woff2");
    CHECK(Test::mime_type("page.xhtml") == "application/xhtml+xml");
  }

  SUBCASE("audio/video types") {
    CHECK(Test::mime_type("song.mp3") == "audio/mp3");
    CHECK(Test::mime_type("clip.mp4") == "video/mp4");
    CHECK(Test::mime_type("movie.webm") == "video/webm");
    CHECK(Test::mime_type("track.wav") == "audio/wave");
    CHECK(Test::mime_type("track.ogg") == "audio/ogg");
    CHECK(Test::mime_type("track.flac") == "audio/flac");
  }

  SUBCASE("archive types") {
    CHECK(Test::mime_type("file.zip") == "application/zip");
    CHECK(Test::mime_type("file.gz") == "application/gzip");
    CHECK(Test::mime_type("file.tar") == "application/x-tar");
    CHECK(Test::mime_type("file.7z") == "application/x-7z-compressed");
  }

  SUBCASE("image types") {
    CHECK(Test::mime_type("pic.webp") == "image/webp");
    CHECK(Test::mime_type("pic.avif") == "image/avif");
    CHECK(Test::mime_type("pic.gif") == "image/gif");
    CHECK(Test::mime_type("pic.bmp") == "image/bmp");
    CHECK(Test::mime_type("pic.tiff") == "image/tiff");
    CHECK(Test::mime_type("anim.apng") == "image/apng");
  }

  SUBCASE("no extension") {
    CHECK(Test::mime_type("Makefile") == "application/octet-stream");
  }

  SUBCASE("custom MIME types") {
    // Custom types defined in CMakeLists.txt MIME_TYPES
    CHECK(Test::mime_type("data.custom") == "application/x-custom");
    CHECK(Test::mime_type("file.mydata") == "application/x-mydata");

    // Falls back to built-in for known types
    CHECK(Test::mime_type("style.css") == "text/css");
    CHECK(Test::mime_type("app.js") == "text/javascript");

    // Falls back to application/octet-stream for unknown
    CHECK(Test::mime_type("unknown.xyz") == "application/octet-stream");
  }
}

TEST_CASE("children") {
  SUBCASE("root children") {
    auto kids = Test::FS.root().children();
    // 4 files + 1 directory at root level
    CHECK(kids.size() == 5);

    // "sub" should appear as a directory
    bool found_sub = false;
    for (const auto &c : kids) {
      if (c.name() == "sub") {
        CHECK(c.is_dir());
        CHECK(c.path() == "sub");
        found_sub = true;
      } else {
        CHECK(c.is_file());
      }
    }
    CHECK(found_sub);
  }

  SUBCASE("subdirectory children via find") {
    auto it = Test::FS.find("sub");
    REQUIRE(it != Test::FS.end());
    auto kids = (*it).children();
    CHECK(kids.size() == 1);
    CHECK(kids[0].name() == "nested.txt");
    CHECK(kids[0].path() == "sub/nested.txt");
    CHECK(kids[0].is_file());
  }

  SUBCASE("nonexistent directory") {
    auto it = Test::FS.find("nonexistent");
    CHECK(it == Test::FS.end());
  }
}

TEST_CASE("is_dir via find") {
  auto sub = Test::FS.find("sub");
  REQUIRE(sub != Test::FS.end());
  CHECK((*sub).is_dir());

  auto hello = Test::FS.find("hello.txt");
  REQUIRE(hello != Test::FS.end());
  CHECK_FALSE((*hello).is_dir());

  CHECK(Test::FS.find("nonexistent") == Test::FS.end());

  SUBCASE("root is dir") { CHECK(Test::FS.root().is_dir()); }
}

TEST_CASE("find") {
  SUBCASE("find existing file") {
    auto it = Test::FS.find("hello.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).path() == "hello.txt");
    CHECK((*it).is_file());
  }

  SUBCASE("find existing directory") {
    auto it = Test::FS.find("sub");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).path() == "sub");
    CHECK((*it).is_dir());
  }

  SUBCASE("find nonexistent returns end") {
    CHECK(Test::FS.find("nonexistent") == Test::FS.end());
  }

  SUBCASE("find with path normalization") {
    auto it = Test::FS.find("./hello.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).path() == "hello.txt");
  }
}

TEST_CASE("Entry") {
  SUBCASE("name returns last path component") {
    auto it = Test::FS.find("sub/nested.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).name() == "nested.txt");
  }

  SUBCASE("name for root-level file") {
    auto it = Test::FS.find("hello.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).name() == "hello.txt");
  }

  SUBCASE("file entry has bytes and text") {
    auto it = Test::FS.find("hello.txt");
    REQUIRE(it != Test::FS.end());
    auto entry = *it;
    CHECK(entry.bytes().has_value());
    CHECK(entry.text().has_value());
    CHECK(entry.text().value() == "Hello, World!\n");
    CHECK(entry.size() > 0);
  }

  SUBCASE("directory entry has no bytes or text") {
    auto it = Test::FS.find("sub");
    REQUIRE(it != Test::FS.end());
    auto entry = *it;
    CHECK_FALSE(entry.bytes().has_value());
    CHECK_FALSE(entry.text().has_value());
    CHECK(entry.size() == 0);
  }

  SUBCASE("directory entry children") {
    auto it = Test::FS.find("sub");
    REQUIRE(it != Test::FS.end());
    auto kids = (*it).children();
    CHECK(kids.size() == 1);
    CHECK(kids[0].name() == "nested.txt");
  }

  SUBCASE("file entry children is empty") {
    auto it = Test::FS.find("hello.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).children().empty());
  }

  SUBCASE("mime_type on entry") {
    auto it = Test::FS.find("hello.txt");
    REQUIRE(it != Test::FS.end());
    CHECK((*it).mime_type() == "text/plain");
  }
}

TEST_CASE("root") {
  auto root = Test::FS.root();
  CHECK(root.is_dir());
  CHECK(root.path() == "");

  SUBCASE("root children") {
    auto kids = root.children();
    CHECK(kids.size() == 5);
  }
}
