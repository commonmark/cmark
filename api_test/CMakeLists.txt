add_executable(api_test
  cplusplus.cpp
  harness.c
  harness.h
  main.c
)

target_link_libraries(api_test cmark::cmark)
