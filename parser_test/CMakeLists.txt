set(TEST_TARGETS
    heading_tests
    strikethrough_tests
    mark_tests)

foreach(TARGET ${TEST_TARGETS})
    add_executable(${TARGET} ${TARGET}.c test_utils.c)
    target_link_libraries(${TARGET} cmark)
    add_test(NAME ${TARGET} COMMAND ${TARGET})

    if(WIN32 AND BUILD_SHARED_LIBS)
        set_tests_properties(${TARGET} PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:cmark>$<SEMICOLON>$ENV{PATH}")
    endif()
endforeach()
