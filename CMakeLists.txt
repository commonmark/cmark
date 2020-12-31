cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0065 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

project(cmark VERSION 0.29.0)

include(TargetProperties)
include(FindAsan)
include(GNUInstallDirs)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Do not build in-source.\nPlease remove CMakeCache.txt and the CMakeFiles/ directory.\nThen: mkdir build ; cd build ; cmake .. ; make")
endif()

option(CMARK_TESTS "Build cmark tests and enable testing" ON)
option(CMARK_STATIC "Build static libcmark library" ON)
option(CMARK_SHARED "Build shared libcmark library" ON)
option(CMARK_EXE "Build cmark executable" ON)
option(CMARK_LIB_FUZZER "Build libFuzzer fuzzing harness" OFF)

set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES};Release;Profile;Asan;Ubsan" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT MSVC)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED YES)
  set(CMAKE_C_EXTENSIONS NO)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(src)

if(NOT WIN32 AND NOT MINGW)
  add_subdirectory(man)
endif()

if(CMARK_TESTS)
  enable_testing()
  if(CMARK_SHARED OR CMARK_STATIC)
    add_subdirectory(api_test)
  endif()
  add_subdirectory(test testdir)
endif()
