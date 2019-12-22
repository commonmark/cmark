add_executable(api_test
  cplusplus.cpp
  harness.c
  harness.h
  main.c
)
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
)
if(CMARK_SHARED)
  target_link_libraries(api_test libcmark)
else()
  target_link_libraries(api_test libcmark_static)
endif()
