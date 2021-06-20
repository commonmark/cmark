include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

set(PROGRAM "cmark_exe")
set(FUZZ "cmark-fuzz")
set(LIBRARY "cmark")
if(CMARK_SHARED)
  set(STATICLIBRARY "cmark_static")
else()
  set(STATICLIBRARY "cmark")
endif()

set(LIBRARY_HEADERS
  cmark.h
  parser.h
  buffer.h
  node.h
  iterator.h
  chunk.h
  references.h
  utf8.h
  scanners.h
  inlines.h
  houdini.h
  cmark_ctype.h
  render.h
)
set(LIBRARY_SOURCES
  cmark.c
  node.c
  iterator.c
  blocks.c
  inlines.c
  scanners.c
  scanners.re
  utf8.c
  buffer.c
  references.c
  render.c
  man.c
  xml.c
  html.c
  commonmark.c
  latex.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  cmark_ctype.c
  ${LIBRARY_HEADERS}
)
set(PROGRAM_SOURCES main.c)

# Feature tests
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_c_source_compiles(
  "int main() { __builtin_expect(0,0); return 0; }"
  HAVE___BUILTIN_EXPECT)
check_c_source_compiles("
  int f(void) __attribute__ (());
  int main() { return 0; }
" HAVE___ATTRIBUTE__)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmark_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h)

# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (CMARK_SHARED)
  add_library(${LIBRARY} SHARED ${LIBRARY_SOURCES})
  cmark_target_properties(${LIBRARY})
  set_target_properties(${LIBRARY} PROPERTIES
    MACOSX_RPATH TRUE
    OUTPUT_NAME "cmark"
    # Avoid name clash between PROGRAM and LIBRARY pdb files.
    PDB_NAME cmark_dll
    # Include minor version and patch level in soname for now.
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    VERSION ${PROJECT_VERSION}
  )
  target_include_directories(${LIBRARY} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  add_library(cmark::cmark ALIAS ${LIBRARY})

  generate_export_header(${LIBRARY}
    BASE_NAME ${PROJECT_NAME})

  list(APPEND CMARK_INSTALL ${LIBRARY})
endif()

if (CMARK_STATIC)
  add_library(${STATICLIBRARY} STATIC ${LIBRARY_SOURCES})
  cmark_target_properties(${STATICLIBRARY})
  set_target_properties(${STATICLIBRARY} PROPERTIES
    OUTPUT_NAME "cmark$<$<BOOL:${MSVC}>:_static>"
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION})
  target_include_directories(${STATICLIBRARY} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  add_library(cmark::cmark_static ALIAS ${STATICLIBRARY})

  if (NOT CMARK_SHARED)
    # Linking against cmark::cmark shouldn't cause errors just because shared
    # builds weren't enabled.
    add_library(cmark::cmark ALIAS ${STATICLIBRARY})
    generate_export_header(${STATICLIBRARY}
      BASE_NAME ${PROJECT_NAME})
  endif()

  list(APPEND CMARK_INSTALL ${STATICLIBRARY})
endif()

if(CMARK_EXE)
  add_executable(${PROGRAM} ${PROGRAM_SOURCES})
  cmark_target_properties(${PROGRAM})
  set_target_properties(${PROGRAM} PROPERTIES OUTPUT_NAME "cmark")

  if (CMARK_STATIC)
    target_link_libraries(${PROGRAM} PRIVATE ${STATICLIBRARY})
  elseif (CMARK_SHARED)
    target_link_libraries(${PROGRAM} PRIVATE ${LIBRARY})
  endif()

  if (MSVC)
    set_target_properties(${PROGRAM} PROPERTIES LINK_FLAGS /INCREMENTAL:NO)
  endif()

  # Disables -rdynamic, which is incompatible with static linking under certain
  # configurations.
  set_target_properties(${PROGRAM} PROPERTIES ENABLE_EXPORTS OFF)

  list(APPEND CMARK_INSTALL ${PROGRAM})
endif()

if(NOT MSVC OR CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
  include(InstallRequiredSystemLibraries)
endif()

install(TARGETS ${CMARK_INSTALL}
  EXPORT cmark-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(CMARK_SHARED OR CMARK_STATIC)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libcmark.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  install(FILES
    cmark.h
    ${CMAKE_CURRENT_BINARY_DIR}/cmark_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # `find_package` searches different paths in Windows.
  if(WIN32)
    set(CMAKE_INSTALL_CMAKEDIR CMake
        CACHE STRING "Installation directory for CMake files")
  else()
    set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/cmark
        CACHE STRING "Installation directory for CMake files")
  endif()

  # generate cmark-config.cmake and cmark-config-version.cmake files
  configure_package_config_file(
    "cmarkConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  # install config and version file
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  )
  # install targets file
  install(EXPORT cmark-targets
    NAMESPACE cmark::
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  )
endif()

if(CMARK_LIB_FUZZER)
  add_executable(${FUZZ} ../test/cmark-fuzz.c ${LIBRARY_SOURCES})
  target_link_libraries(${FUZZ} PRIVATE ${CMAKE_LIB_FUZZER_PATH})

  # cmark is written in C but the libFuzzer runtime is written in C++ which
  # needs to link against the C++ runtime.
  set_target_properties(${FUZZ} PROPERTIES LINKER_LANGUAGE CXX)
endif()
