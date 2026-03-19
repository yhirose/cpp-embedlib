cmake_minimum_required(VERSION 3.20)
project(basic_example CXX)

include(FetchContent)
FetchContent_Declare(cpp-embedlib
    GIT_REPOSITORY https://github.com/yhirose/cpp-embedlib
    GIT_TAG        main
)
FetchContent_MakeAvailable(cpp-embedlib)

cpp_embedlib_add(WebAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../www
    NAMESPACE Web
)

add_executable(basic main.cpp)
target_link_libraries(basic PRIVATE WebAssets)
