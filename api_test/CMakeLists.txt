add_executable(api_test
  cplusplus.cpp
  harness.c
  harness.h
  main.c
)
if(CMARK_SHARED)
  target_link_libraries(api_test libcmark)
else()
  target_link_libraries(api_test libcmark_static)
endif()
