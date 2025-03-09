add_executable(heading_tests
    heading_tests.c
    test_utils.c)
target_link_libraries(heading_tests cmark)
target_include_directories(heading_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src)

add_executable(strikethrough_tests strikethrough_tests.c test_utils.c)
target_link_libraries(strikethrough_tests cmark)
target_include_directories(heading_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src)

add_test(NAME heading_tests COMMAND heading_tests)
add_test(NAME strikethrough_tests COMMAND strikethrough_tests)
