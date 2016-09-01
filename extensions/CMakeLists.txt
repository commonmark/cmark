cmake_minimum_required(VERSION 2.8)
set(LIBRARY "libcmarkextensions")
set(STATICLIBRARY "libcmarkextensions_static")
set(LIBRARY_SOURCES
    core-extensions.c
    table.c
    strikethrough.c
    ext_scanners.c
    ext_scanners.h
   )

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
)

# We make LIB_INSTALL_DIR configurable rather than
# hard-coding lib, because on some OSes different locations
# are used for different architectures (e.g. /usr/lib64 on
# 64-bit Fedora).
if(NOT LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR "lib" CACHE STRING
  "Set the installation directory for libraries." FORCE)
endif(NOT LIB_INSTALL_DIR)

include_directories(. ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELEASE} -pg")
set(CMAKE_LINKER_PROFILE "${CMAKE_LINKER_FLAGS_RELEASE} -pg")

add_library(${LIBRARY} SHARED ${LIBRARY_SOURCES})
add_library(${STATICLIBRARY} STATIC ${LIBRARY_SOURCES})

set_property(TARGET ${LIBRARY}
  APPEND PROPERTY MACOSX_RPATH true)

set_target_properties(${STATICLIBRARY} PROPERTIES
  COMPILE_FLAGS -DCMARK_STATIC_DEFINE
  POSITION_INDEPENDENT_CODE ON)

if (MSVC)
  set_target_properties(${STATICLIBRARY} PROPERTIES
    OUTPUT_NAME "cmarkextensions_static"
    VERSION ${PROJECT_VERSION})
else()
  set_target_properties(${STATICLIBRARY} PROPERTIES
    OUTPUT_NAME "cmarkextensions"
    VERSION ${PROJECT_VERSION})
endif(MSVC)

target_link_libraries(${LIBRARY} libcmark)
