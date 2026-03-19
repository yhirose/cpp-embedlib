build_dir := "build"

default: list

list:
    @just --list --unsorted

# Run all tests
test:
    cmake -B {{build_dir}} -DCPP_EMBEDLIB_BUILD_TESTS=ON -DCPP_EMBEDLIB_BUILD_E2E_TESTS=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build {{build_dir}} --target cpp_embedlib_tests --target cpp_embedlib_e2e_tests -j$(sysctl -n hw.ncpu)
    {{build_dir}}/tests/cpp_embedlib_tests
    {{build_dir}}/tests/e2e/cpp_embedlib_e2e_tests

# Configure, build, and run unit tests
test-unit:
    cmake -B {{build_dir}} -DCPP_EMBEDLIB_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build {{build_dir}} --target cpp_embedlib_tests -j$(sysctl -n hw.ncpu)
    {{build_dir}}/tests/cpp_embedlib_tests

# Configure, build, and run e2e tests (requires network for cpp-httplib)
test-e2e:
    cmake -B {{build_dir}} -DCPP_EMBEDLIB_BUILD_E2E_TESTS=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build {{build_dir}} --target cpp_embedlib_e2e_tests -j$(sysctl -n hw.ncpu)
    {{build_dir}}/tests/e2e/cpp_embedlib_e2e_tests

# Run benchmark
bench:
    cmake -B {{build_dir}} -DCPP_EMBEDLIB_BUILD_BENCHMARK=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build {{build_dir}} --target cpp_embedlib_benchmark -j$(sysctl -n hw.ncpu)
    {{build_dir}}/benchmark/cpp_embedlib_benchmark

# Clean build directories
clean:
    rm -rf {{build_dir}} build-*
    find examples -type d -name {{build_dir}} -exec rm -rf {} +
