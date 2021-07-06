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
