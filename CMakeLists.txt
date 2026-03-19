cmake_minimum_required(VERSION 3.20)
project(cpp-embedlib LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cpp-embedlib)

option(CPP_EMBEDLIB_BUILD_TESTS "Build tests" OFF)
option(CPP_EMBEDLIB_BUILD_E2E_TESTS "Build e2e tests (requires network for cpp-httplib)" OFF)
option(CPP_EMBEDLIB_BUILD_BENCHMARK "Build benchmarks" OFF)
option(CPP_EMBEDLIB_BUILD_EXAMPLES "Build examples" OFF)

if(CPP_EMBEDLIB_BUILD_TESTS OR CPP_EMBEDLIB_BUILD_E2E_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest
        GIT_TAG        v2.4.12
    )
    FetchContent_MakeAvailable(doctest)
endif()

if(CPP_EMBEDLIB_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(CPP_EMBEDLIB_BUILD_E2E_TESTS)
    add_subdirectory(tests/e2e)
endif()

if(CPP_EMBEDLIB_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(CPP_EMBEDLIB_BUILD_EXAMPLES)
    # Build examples using local source (no FetchContent needed)
    cpp_embedlib_add(SiteAssets
        FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/examples/traverse/site
        NAMESPACE Site
    )
    add_executable(traverse examples/traverse/main.cpp)
    target_link_libraries(traverse PRIVATE SiteAssets)
endif()
