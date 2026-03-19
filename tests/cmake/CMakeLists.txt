# Edge case tests for cpp-embedlib CMake macro.
# These test configure/build-time behavior using separate CMake projects.

# Empty folder → configure should fail with FATAL_ERROR
add_test(
    NAME cmake_empty_folder
    COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_CURRENT_SOURCE_DIR}/empty_folder
        -B ${CMAKE_CURRENT_BINARY_DIR}/empty_folder
        -DCPP_EMBEDLIB_DIR=${PROJECT_SOURCE_DIR}
)
set_tests_properties(cmake_empty_folder PROPERTIES WILL_FAIL TRUE)

# Duplicate files → should deduplicate and build successfully
set(_dup_bin ${CMAKE_CURRENT_BINARY_DIR}/duplicate_files/bin)
add_test(
    NAME cmake_duplicate_files_configure
    COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_CURRENT_SOURCE_DIR}/duplicate_files
        -B ${CMAKE_CURRENT_BINARY_DIR}/duplicate_files
        -DCPP_EMBEDLIB_DIR=${PROJECT_SOURCE_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${_dup_bin}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${_dup_bin}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${_dup_bin}
)

add_test(
    NAME cmake_duplicate_files_build
    COMMAND ${CMAKE_COMMAND}
        --build ${CMAKE_CURRENT_BINARY_DIR}/duplicate_files
        --config $<CONFIG>
)
set_tests_properties(cmake_duplicate_files_build PROPERTIES
    DEPENDS cmake_duplicate_files_configure
)

add_test(
    NAME cmake_duplicate_files_run
    COMMAND ${_dup_bin}/test_dup
)
set_tests_properties(cmake_duplicate_files_run PROPERTIES
    DEPENDS cmake_duplicate_files_build
)
