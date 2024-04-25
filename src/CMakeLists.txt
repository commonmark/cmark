
configure_file(cmark_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h)
configure_file(libcmark.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc
  @ONLY)

add_library(cmark
  blocks.c
  buffer.c
  cmark.c
  cmark_ctype.c
  commonmark.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  html.c
  inlines.c
  iterator.c
  latex.c
  man.c
  node.c
  references.c
  render.c
  scanners.c
  scanners.re
  utf8.c
  xml.c)
cmark_add_compile_options(cmark)
set_target_properties(cmark PROPERTIES
  OUTPUT_NAME "cmark"
  # Avoid name clash between PROGRAM and LIBRARY pdb files.
  PDB_NAME libcmark
  POSITION_INDEPENDENT_CODE YES
  # Include minor version and patch level in soname for now.
  SOVERSION ${PROJECT_VERSION}
  VERSION ${PROJECT_VERSION})
if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(cmark PUBLIC
    CMARK_STATIC_DEFINE)
endif()
target_include_directories(cmark INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

generate_export_header(cmark
  BASE_NAME ${PROJECT_NAME})

# FIXME(compnerd) this should be removed, but exists solely to allow a migration
# path for OSS Fuzz.
add_custom_target(cmark_static DEPENDS cmark)

add_executable(cmark_exe
  main.c)
cmark_add_compile_options(cmark_exe)
set_target_properties(cmark_exe PROPERTIES
  OUTPUT_NAME "cmark"
  INSTALL_RPATH "${Base_rpath}")
target_link_libraries(cmark_exe PRIVATE
  cmark)

install(TARGETS cmark_exe cmark
  EXPORT cmark-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

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
