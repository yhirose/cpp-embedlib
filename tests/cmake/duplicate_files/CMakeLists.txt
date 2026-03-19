cmake_minimum_required(VERSION 3.20)
project(test_duplicate_files LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CPP_EMBEDLIB_DIR}/cmake")
include(cpp-embedlib)

# Specify the same file via both FOLDER and FILES — should deduplicate
cpp_embedlib_add(DupAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/assets
    FILES  ${CMAKE_CURRENT_SOURCE_DIR}/assets/dup.txt
    NAMESPACE Dup
)

add_executable(test_dup main.cpp)
target_link_libraries(test_dup PRIVATE DupAssets)
