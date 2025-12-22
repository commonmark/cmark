add_executable(api_test
  cplusplus.cpp
  delim_test.c
  delim_test.h
  harness.c
  harness.h
  main.c
)
cmark_add_compile_options(api_test)
target_link_libraries(api_test PRIVATE
  cmark)

add_test(NAME api_test COMMAND api_test)
if(WIN32)
  set_tests_properties(api_test PROPERTIES
    ENVIRONMENT "PATH=$<TARGET_FILE_DIR:cmark>$<SEMICOLON>$ENV{PATH}")
endif()

