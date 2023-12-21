add_executable(api_test
  cplusplus.cpp
  harness.c
  harness.h
  main.c
)
cmark_add_compile_options(api_test)
if(CMARK_SHARED)
  target_link_libraries(api_test cmark)
else()
  target_link_libraries(api_test cmark_static)
endif()

add_test(NAME api_test COMMAND api_test)
if(WIN32)
  file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/src WIN_DLL_DIR)
  set(NEWPATH "${WIN_DLL_DIR};$ENV{PATH}")
  string(REPLACE ";" "\\;" NEWPATH "${NEWPATH}")
  set_tests_properties(api_test PROPERTIES ENVIRONMENT "PATH=${NEWPATH}")
endif()

