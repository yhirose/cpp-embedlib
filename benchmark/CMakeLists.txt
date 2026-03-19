cpp_embedlib_add(BenchAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/assets
    NAMESPACE Bench
)

add_executable(cpp_embedlib_benchmark main.cpp)
target_link_libraries(cpp_embedlib_benchmark PRIVATE BenchAssets)

# Pass assets directory path for filesystem benchmark
target_compile_definitions(cpp_embedlib_benchmark PRIVATE
    BENCH_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
