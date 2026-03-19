cmake_minimum_required(VERSION 3.20)
project(httplib_example CXX)

include(FetchContent)
FetchContent_Declare(cpp-embedlib
    GIT_REPOSITORY https://github.com/yhirose/cpp-embedlib
    GIT_TAG        main
)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GIT_TAG        v0.38.0
)
FetchContent_MakeAvailable(cpp-embedlib httplib)

cpp_embedlib_add(WebAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../www
    NAMESPACE Web
)

add_executable(httplib_example main.cpp)
target_link_libraries(httplib_example PRIVATE WebAssets httplib::httplib cpp-embedlib-httplib)
