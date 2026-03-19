cpp_embedlib_add(TestAssets
    FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/assets
    NAMESPACE Test
    MIME_TYPES
        .custom "application/x-custom"
        .mydata "application/x-mydata"
)

add_executable(cpp_embedlib_tests test_main.cpp)
target_link_libraries(cpp_embedlib_tests PRIVATE TestAssets doctest_with_main)
add_test(NAME cpp_embedlib_tests COMMAND cpp_embedlib_tests)

add_subdirectory(cmake)
