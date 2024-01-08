set(LIBRARY "cmark")
set(HEADERS
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
  ${HEADERS}
  )

set(PROGRAM "cmark_exe")
set(PROGRAM_SOURCES main.c)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmark_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h)

add_executable(${PROGRAM} ${PROGRAM_SOURCES})
cmark_add_compile_options(${PROGRAM})
set_target_properties(${PROGRAM} PROPERTIES
  OUTPUT_NAME "cmark")
target_link_libraries(${PROGRAM} PRIVATE
  cmark)

add_library(${LIBRARY}
  ${LIBRARY_SOURCES})
cmark_add_compile_options(${LIBRARY})
set_target_properties(${LIBRARY} PROPERTIES
  MACOSX_RPATH TRUE
  OUTPUT_NAME "cmark"
  # Avoid name clash between PROGRAM and LIBRARY pdb files.
  PDB_NAME libcmark
  POSITION_INDEPENDENT_CODE YES
  # Include minor version and patch level in soname for now.
  SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  VERSION ${PROJECT_VERSION})
if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${LIBRARY} PUBLIC
    CMARK_STATIC_DEFINE)
endif()
target_include_directories(${LIBRARY} INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

generate_export_header(${LIBRARY}
  BASE_NAME ${PROJECT_NAME})

list(APPEND CMARK_INSTALL ${LIBRARY})

# FIXME(compnerd) this should be removed, but exists solely to allow a migration
# path for OSS Fuzz.
add_custom_target(cmark_static DEPENDS cmark)

install(TARGETS ${PROGRAM} ${CMARK_INSTALL}
  EXPORT cmark-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

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

# generate cmark-config.cmake and cmark-config-version.cmake files
configure_package_config_file(
  "cmarkConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cmark")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
# install config and version file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/cmark-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cmark"
)
# install targets file
install(
  EXPORT "cmark-targets"
  NAMESPACE "cmark::"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cmark"
)

if(CMARK_LIB_FUZZER)
  add_executable(cmark-fuzz ../test/cmark-fuzz.c ${LIBRARY_SOURCES})
  cmark_add_compile_options(cmark-fuzz)
  target_link_libraries(cmark-fuzz "${CMAKE_LIB_FUZZER_PATH}")

  # cmark is written in C but the libFuzzer runtime is written in C++ which
  # needs to link against the C++ runtime.
  set_target_properties(cmark-fuzz PROPERTIES
    LINKER_LANGUAGE CXX)
endif()
