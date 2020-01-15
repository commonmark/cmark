cmake_minimum_required(VERSION 3.0)
project(cmark VERSION 0.29.0)

include("FindAsan.cmake")
include(GNUInstallDirs)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Do not build in-source.\nPlease remove CMakeCache.txt and the CMakeFiles/ directory.\nThen: mkdir build ; cd build ; cmake .. ; make")
endif()

option(CMARK_TESTS "Build cmark tests and enable testing" ON)
option(CMARK_STATIC "Build static libcmark library" ON)
option(CMARK_SHARED "Build shared libcmark library" ON)
option(CMARK_LIB_FUZZER "Build libFuzzer fuzzing harness" OFF)

if(NOT MSVC)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED YES)
  set(CMAKE_C_EXTENSIONS NO)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# The Linux modules distributed with CMake add "-rdynamic" to the build flags
# which is incompatible with static linking under certain configurations.
# Unsetting CMAKE_SHARED_LIBRARY_LINK_C_FLAGS ensures this does not happen.
if(CMARK_STATIC AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
endif()

# Compiler flags
if(MSVC)
  # Force to always compile with W4
  add_compile_options($<$<COMPILE_LANGUAGE:C>:/W4>)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:/wd4706>)
  # Compile as C++ under MSVC older than 12.0
  if(MSVC_VERSION LESS 1800)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/TP>)
  endif()
  add_compile_options($<$<COMPILE_LANGUAGE:C>:/D_CRT_SECURE_NO_WARNINGS>)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES Clang)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wall>)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wextra>)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-pedantic>)
endif()

# Check integrity of node structure when compiled as debug
add_compile_options($<$<CONFIG:Debug>:-DCMARK_DEBUG_NODES>)

add_compile_options($<$<AND:$<CONFIG:PROFILE>,$<COMPILE_LANGUAGE:C>>:-pg>)

if(CMAKE_BUILD_TYPE STREQUAL Ubsan)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-fsanitize=undefined>)
endif()
if(CMARK_LIB_FUZZER)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-fsanitize-coverage=trace-pc-guard>)
endif()

add_subdirectory(src)
if(CMARK_TESTS AND (CMARK_SHARED OR CMARK_STATIC))
  add_subdirectory(api_test)
endif()
# TODO(compnerd) should this be enabled for MinGW, which sets CMAKE_SYSTEM_NAME
# to Windows, but defines `MINGW`.
if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  add_subdirectory(man)
endif()
if(CMARK_TESTS)
  enable_testing()
  add_subdirectory(test testdir)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Profile Release Asan Ubsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
