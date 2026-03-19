cmake_minimum_required(VERSION 3.20)
project(test_empty_folder LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CPP_EMBEDLIB_DIR}/cmake")
include(cpp-embedlib)

# This should fail with FATAL_ERROR because the folder is empty
cpp_embedlib_add(EmptyAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/empty
    NAMESPACE Empty
)
