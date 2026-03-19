cmake_minimum_required(VERSION 3.20)
project(traverse_example CXX)

include(FetchContent)
FetchContent_Declare(cpp-embedlib
    GIT_REPOSITORY https://github.com/yhirose/cpp-embedlib
    GIT_TAG        main
)
FetchContent_MakeAvailable(cpp-embedlib)

cpp_embedlib_add(SiteAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/site
    NAMESPACE Site
)

add_executable(traverse main.cpp)
target_link_libraries(traverse PRIVATE SiteAssets)
