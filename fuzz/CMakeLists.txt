add_executable(cmark-fuzz cmark-fuzz.c)
cmark_add_compile_options(cmark-fuzz)
target_link_libraries(cmark-fuzz cmark)
