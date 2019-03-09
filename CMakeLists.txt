cmake_minimum_required(VERSION 2.8.9)

# prevent ugly developer warnings because version is set directly, not through project()
# it should be redone properly by using VERSION in project() if on CMake 3.x
if(CMAKE_MAJOR_VERSION GREATER 2)
    cmake_policy(SET CMP0048 OLD)
endif()

project(cmark)

include("FindAsan.cmake")
include("CheckFileOffsetBits.cmake")

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Do not build in-source.\nPlease remove CMakeCache.txt and the CMakeFiles/ directory.\nThen: mkdir build ; cd build ; cmake .. ; make")
endif()

set(PROJECT_NAME "cmark")

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 28)
set(PROJECT_VERSION_PATCH 3)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

option(CMARK_TESTS "Build cmark tests and enable testing" ON)
option(CMARK_STATIC "Build static libcmark library" ON)
option(CMARK_SHARED "Build shared libcmark library" ON)
option(CMARK_LIB_FUZZER "Build libFuzzer fuzzing harness" OFF)

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
