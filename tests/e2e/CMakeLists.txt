include(FetchContent)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_ZSTD OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_BROTLI OFF CACHE BOOL "" FORCE)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GIT_TAG        v0.38.0
)
FetchContent_MakeAvailable(httplib)

cpp_embedlib_add(E2EAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../assets
    NAMESPACE E2E
)

add_executable(cpp_embedlib_e2e_tests test_httplib.cpp)
target_link_libraries(cpp_embedlib_e2e_tests
    PRIVATE E2EAssets httplib::httplib doctest_with_main cpp-embedlib-httplib
)
add_test(NAME e2e_httplib COMMAND cpp_embedlib_e2e_tests)
