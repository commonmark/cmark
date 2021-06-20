# Defines common cmark properties.
function(cmark_target_properties tgt)
  # Compiler flags
  if(MSVC)
    target_compile_options(${tgt} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:/W4> # Compile with W4
      $<$<COMPILE_LANGUAGE:C>:/wd4706>
      $<$<COMPILE_LANGUAGE:C>:/D_CRT_SECURE_NO_WARNINGS>
      $<$<COMPILE_LANGUAGE:C>:/D_CRT_SECURE_NO_WARNINGS>
      $<$<AND:$<COMPILE_LANGUAGE:C>,$<VERSION_LESS:MSVC_VERSION,1800>>:/TP>
    )
  elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES Clang)
    target_compile_options(${tgt} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:-Wall>
      $<$<COMPILE_LANGUAGE:C>:-Wextra>
      $<$<COMPILE_LANGUAGE:C>:-pedantic>
      $<$<AND:$<COMPILE_LANGUAGE:C>,$<CONFIG:PROFILE>>:-pg>
      $<$<AND:$<COMPILE_LANGUAGE:C>,$<STREQUAL:${CMAKE_BUILD_TYPE},Ubsan>>:-fsanitize=undefined>
      $<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:${CMARK_LIB_FUZZER}>>:-fsanitize-coverage=trace-pc-guard>
    )
  endif()

  # Compiler definitions
  target_compile_definitions(${tgt} PRIVATE
    $<$<CONFIG:Debug>:CMARK_DEBUG_NODES> # Check integrity of node structure
    $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>:CMARK_STATIC_DEFINE> # Disable PUBLIC declarations
  )
endfunction()
