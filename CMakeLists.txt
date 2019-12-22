cmake_minimum_required(VERSION 3.0)
project(cmark VERSION 0.29.0)

include("FindAsan.cmake")

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

# The Linux modules distributed with CMake add "-rdynamic" to the build flags
# which is incompatible with static linking under certain configurations.
# Unsetting CMAKE_SHARED_LIBRARY_LINK_C_FLAGS ensures this does not happen.
if(CMARK_STATIC AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
endif()

# Compiler flags
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4706 /D_CRT_SECURE_NO_WARNINGS")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES Clang)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
endif()

add_subdirectory(src)
if(CMARK_TESTS AND (CMARK_SHARED OR CMARK_STATIC))
  add_subdirectory(api_test)
endif()
add_subdirectory(man)
if(CMARK_TESTS)
  enable_testing()
  add_subdirectory(test testdir)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Profile Release Asan Ubsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
