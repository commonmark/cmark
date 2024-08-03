cmake_minimum_required(VERSION 3.7)

if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()
if(POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW)
endif()

project(cmark
  LANGUAGES C CXX
  VERSION 0.31.1)

if(CMAKE_BUILD_TYPE STREQUAL Asan)
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
  include(FindAsan)
endif()

# Include module for functions
# - 'write_basic_package_version_file'
# - 'configure_package_config_file'
include(CMakePackageConfigHelpers)
include(CTest)
include(GenerateExportHeader)
include(GNUInstallDirs)

if(NOT MSVC OR CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
  include(InstallRequiredSystemLibraries)
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Do not build in-source.\nPlease remove CMakeCache.txt and the CMakeFiles/ directory.\nThen: mkdir build ; cd build ; cmake .. ; make")
endif()

# Backwards Compatibility
# TODO: remove this once there has been a period to enable people to migrate
set(_CMARK_BUILD_SHARED_LIBS_DEFAULT NO)
if(DEFINED CMARK_SHARED)
  message(AUTHOR_WARNING [=[
'CMARK_SHARED' has been replaced with the standard 'BUILD_SHARED_LIBS' to control the library type.
]=])
  set(_CMARK_BUILD_SHARED_LIBS_DEFAULT ${CMARK_SHARED})
endif()
if(DEFINED CMARK_STATIC)
  message(AUTHOR_WARNING [=[
'CMARK_STATIC' has been replaced with the standard 'BUILD_SHARED_LIBS' to control the library type.
]=])
  if(NOT CMARK_STATIC)
    set(_CMARK_BUILD_SHARED_LIBS_DEFAULT YES)
  else()
    set(_CMARK_BUILD_SHARED_LIBS_DEFAULT NO)
  endif()
endif()

option(CMARK_LIB_FUZZER "Build libFuzzer fuzzing harness" OFF)
option(BUILD_SHARED_LIBS "Build the CMark library as shared"
  ${_CMARK_BUILD_SHARED_LIBS_DEFAULT})

if(NOT MSVC)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED YES)
  set(CMAKE_C_EXTENSIONS NO)
endif()

# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (DEFINED CMAKE_INSTALL_RPATH)
  set(Base_rpath "${CMAKE_INSTALL_RPATH}")
else()
  if(BUILD_SHARED_LIBS)
    set(p "${CMAKE_INSTALL_FULL_LIBDIR}")
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${p}" i)
    if("${i}" STREQUAL "-1")
      set(Base_rpath "${p}")
    endif()
  endif()
endif()

# Append non-standard external dependency directories, if any.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Check integrity of node structure when compiled as debug
add_compile_options($<$<CONFIG:Debug>:-DCMARK_DEBUG_NODES>)

# In order to maintain compatibility with older platforms which may not have a
# recent version of CMake (i.e. are running CMake <3.3), we cannot simply use
# the `add_compile_options` with a generator expression.  This uses the
# `target_compile_options` with `PRIVATE` to add the flags only to the targets
# so that CMark may be used in projects with non-C languages.
function(cmark_add_compile_options target)
  if(MSVC)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${target} PRIVATE
            -Wall -Wextra -pedantic
            $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Profile)
    target_compile_options(${target} PRIVATE -pg)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Ubsan)
    target_compile_options(${target} PRIVATE -fsanitize=undefined)
  endif()
  if(CMARK_LIB_FUZZER)
    if(target MATCHES fuzz)
      target_compile_options(${target} PRIVATE -fsanitize=fuzzer)
      target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    else()
      target_compile_options(${target} PRIVATE -fsanitize=fuzzer-no-link)
      target_link_options(${target} PRIVATE -fsanitize=fuzzer-no-link)
    endif()
  endif()
endfunction()

add_subdirectory(src)
# TODO(compnerd) should this be enabled for MinGW, which sets CMAKE_SYSTEM_NAME
# to Windows, but defines `MINGW`.
if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  add_subdirectory(man)
endif()
if(BUILD_TESTING)
  add_subdirectory(api_test)
  add_subdirectory(test testdir)
endif()
if(CMARK_LIB_FUZZER)
  add_subdirectory(fuzz)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Profile Release Asan Ubsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
